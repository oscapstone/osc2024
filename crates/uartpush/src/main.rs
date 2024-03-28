use std::{
    fs::File,
    io::{Read, Write},
    path::{Path, PathBuf},
    sync::Arc,
    time::Duration,
};

use clap::Parser;
use color_eyre::eyre::{eyre, Context, Result};
use indicatif::{ProgressBar, ProgressState, ProgressStyle};
use serial2::SerialPort;

/// uartpush is a utility designed to push the kernel through a UART-connected serial device for loading by uartload.
#[derive(Debug, Parser)]
#[command(version, author, about, long_about = None)]
struct Args {
    /// Path to the kernel image to push
    #[clap(short, long)]
    image: PathBuf,

    /// Path to the UART serial device
    #[clap(short, long)]
    device: PathBuf,

    /// Baud rate to use for the serial device
    #[clap(short, long, default_value = "115200")]
    baud_rate: u32,
}

fn main() -> Result<()> {
    install_tracing();
    color_eyre::install()?;

    let args = Args::parse();

    let mut serial = match open_serial(&args.device, args.baud_rate) {
        Ok(port) => {
            tracing::info!("Serial connected");
            port
        }
        Err(e) => {
            tracing::error!(
                "Failed to open serial device '{}': {}",
                args.device.display(),
                e
            );
            return Err(eyre!("Failed to open serial device: {:?}", e));
        }
    };

    let image = match File::open(&args.image) {
        Ok(file) => file,
        Err(e) => {
            tracing::error!(
                "Failed to open kernel image '{}': {}",
                args.image.display(),
                e
            );
            return Err(eyre!("Failed to open kernel image: {:?}", e));
        }
    };
    let image_size = image.metadata()?.len();
    let Ok(image_size) = image_size.try_into() else {
        // Downcast from u64 to u32, the only possible error is if the image is too large
        tracing::error!(
            "Kernel image '{}' is too large to be pushed",
            args.image.display()
        );
        return Err(eyre!("Kernel image size is too large"));
    };

    tracing::info!(
        "Pushing kernel '{}' of size {} bytes",
        args.image.display(),
        image_size
    );

    if let Err(e) = send_size(image_size, &mut serial) {
        tracing::error!("Failed to send kernel size: {}", e);
        return Result::Err(e).wrap_err("Failed to send kernel size");
    };

    if let Err(e) = push_kernel(image_size, image, &mut serial) {
        tracing::error!("Failed to push kernel: {}", e);
        return Result::Err(e).wrap_err("Failed to push kernel");
    };

    forward_terminal(serial);

    Ok(())
}
fn wait_for_serial(serial: &Path) {
    if serial.exists() {
        return;
    }

    tracing::info!("Waiting for serial device to be connected...");
    loop {
        std::thread::sleep(Duration::from_secs(1));
        if serial.exists() {
            break;
        }
    }
}

fn open_serial(serial: &Path, baud_rate: u32) -> Result<SerialPort> {
    wait_for_serial(serial);

    let mut port = SerialPort::open(serial, baud_rate)?;
    port.set_read_timeout(Duration::from_secs(1))?;
    port.set_write_timeout(Duration::from_secs(1))?;

    Ok(port)
}

fn send_size(image_size: u32, serial: &mut SerialPort) -> Result<()> {
    tracing::info!("Writing kernel size to device");
    serial
        .write_all(&image_size.to_le_bytes())
        .wrap_err("Failed to write image size")?;

    let mut buffer = [0u8; 1024];
    let read = serial
        .read(&mut buffer)
        .wrap_err("Failed to read confirmation from device")?;
    let [b'O', b'K'] = &buffer[..read] else {
        tracing::error!("Kernel push failed, did not receive 'OK' from device");
        return Err(eyre!(
            "Kernel push failed, did not receive 'OK' from device"
        ));
    };

    Ok(())
}

fn push_kernel(image_size: u32, mut image: impl Read, serial: &mut SerialPort) -> Result<()> {
    tracing::info!("Writing kernel to device");
    let pb = ProgressBar::new(image_size as u64);

    let style = ProgressStyle::with_template("{spinner:.green} [{elapsed_precise}] [{wide_bar:.cyan/blue}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .with_key("eta", |state: &ProgressState, w: &mut dyn std::fmt::Write| write!(w, "{:.1}s", state.eta().as_secs_f64()).unwrap())
        .progress_chars("#>-");
    pb.set_style(style);

    loop {
        let mut buffer = [0u8; 1024];
        let read = image
            .read(&mut buffer)
            .wrap_err("Failed to read the kernel image")?;
        if read == 0 {
            break;
        }
        serial
            .write_all(&buffer[..read])
            .wrap_err("Failed to write the kernel image to device")?;
        pb.inc(read as u64);
    }

    pb.finish_with_message("Kernel pushed successfully");

    Ok(())
}

fn forward_terminal(serial: SerialPort) {
    let serial = Arc::new(serial);

    let s = serial.clone();
    let target_to_host = std::thread::spawn(move || {
        let serial = s;
        let mut ch = [0u8];
        loop {
            match serial.read(&mut ch) {
                Ok(1) => print!("{}", ch[0] as char),
                Ok(_) => continue,
                Err(e) => {
                    match e.kind() {
                        std::io::ErrorKind::TimedOut => continue,
                        _ => {
                            tracing::error!("Error reading from serial: {}", e);
                            break;
                        }
                    };
                }
            }
        }
        tracing::warn!("Connection closed");
    });

    // host to target
    loop {
        let mut buffer = [0u8];
        match std::io::stdin().read(&mut buffer) {
            Ok(1) => {
                if buffer[0] == 3 {
                    // Ctrl-C
                    break;
                }
                serial.write_all(&buffer).unwrap();
            }
            _ => break,
        }
    }

    target_to_host.join().unwrap();
}

fn install_tracing() {
    use tracing_subscriber::prelude::*;
    use tracing_subscriber::{fmt, EnvFilter};

    let fmt_layer = fmt::layer();
    let filter_layer = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));

    tracing_subscriber::registry()
        .with(filter_layer)
        .with(fmt_layer)
        .init();
}
