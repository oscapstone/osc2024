use std::{
    fs::File,
    io::{Read, Write},
    path::PathBuf,
    time::Duration,
};

use clap::Parser;
use color_eyre::eyre::{eyre, Context, Result};
use indicatif::{ProgressBar, ProgressState, ProgressStyle};
use serial2::SerialPort;

#[derive(Debug, Parser)]
struct Args {
    /// Path to the kernel image to push
    #[clap(short, long, default_value = "kernel8.img")]
    image: PathBuf,

    /// Path to the UART serial device
    #[clap(short, long, default_value = "/dev/ttyUSB0")]
    device: PathBuf,

    /// Baud rate to use for the serial device
    #[clap(short, long, default_value = "115200")]
    baud_rate: u32,
}

fn main() -> Result<()> {
    install_tracing();
    color_eyre::install()?;

    let args = Args::parse();

    let image = match File::open(&args.image) {
        Ok(file) => file,
        Err(e) => {
            tracing::error!(
                "Failed to open kernel image '{}': {}",
                args.image.display(),
                e
            );
            return Err(e.into());
        }
    };
    let image_size = image.metadata()?.len();
    let Ok(image_size) = image_size.try_into() else {
        // Downcast from u64 to u32, the only possible error is if the image is too large
        tracing::error!(
            "Kernel image '{}' is too large to be pushed",
            args.image.display()
        );
        return Err(eyre!(
            "Kernel image '{}' is too large to be pushed",
            args.image.display()
        ));
    };

    tracing::info!(
        "Pushing kernel '{}' of size {} bytes",
        args.image.display(),
        image_size
    );

    let mut device = match SerialPort::open(&args.device, args.baud_rate) {
        Ok(port) => port,
        Err(e) => {
            tracing::error!(
                "Failed to open serial device '{}': {}",
                args.device.display(),
                e
            );
            return Err(e.into());
        }
    };
    device.set_read_timeout(Duration::from_secs(1))?;
    device.set_write_timeout(Duration::from_secs(1))?;

    if let Err(e) = push_kernel(image_size, image, device) {
        tracing::error!("Failed to push kernel: {}", e);
        return Err(e);
    };

    Ok(())
}

fn push_kernel(image_size: u32, mut image: impl Read, mut device: impl Read + Write) -> Result<()> {
    tracing::info!("Writing kernel size to device");
    device.write_all(&image_size.to_le_bytes())?;

    let mut buffer = [0u8; 1024];
    let read = device.read(&mut buffer).wrap_err("Read failed")?;
    let [b'O', b'K'] = &buffer[..read] else {
        tracing::error!("Kernel push failed, did not receive 'OK' from device");
        return Err(eyre!(
            "Kernel push failed, did not receive 'OK' from device"
        ));
    };

    tracing::info!("Writing kernel to device");
    let pb = ProgressBar::new(image_size as u64);

    let style = ProgressStyle::with_template("{spinner:.green} [{elapsed_precise}] [{wide_bar:.cyan/blue}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .with_key("eta", |state: &ProgressState, w: &mut dyn std::fmt::Write| write!(w, "{:.1}s", state.eta().as_secs_f64()).unwrap())
        .progress_chars("#>-");
    pb.set_style(style);

    loop {
        let mut buffer = [0u8; 1024];
        let read = image.read(&mut buffer).wrap_err("Read failed")?;
        if read == 0 {
            break;
        }
        device.write_all(&buffer[..read]).wrap_err("Write failed")?;
        pb.inc(read as u64);
    }

    pb.finish_with_message("Kernel pushed successfully");

    Ok(())
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
