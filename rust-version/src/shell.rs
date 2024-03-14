use crate::bcm::mailbox::MailboxTag;

pub fn interactiave_shell() -> ! {
    let mut array : [u8; MAXCHAR] = [0; MAXCHAR];
    let mut cnt = 0;

    loop {
        let c = bcm::UART.get_char();
        array[cnt] = c;
        let c = c as char;
        print!("{}", c);
        if c == '\r' {
            println!();
            match core::str::from_utf8(&array[0..cnt]).unwrap() {
                "help" => {
                    help();
                }
                "hello" => {
                    println!("Hello World!");
                }
                "reboot" => {
                    println!("Rebooting...");
                    reboot();
                }
                "board" => {
                    let (board, _) = bcm::MAILBOX.get(MailboxTag::BoardRevision);
                    let (mem0, mem1) = bcm::MAILBOX.get(MailboxTag::ArmMemory);
                    let (serial0, serial1) = bcm::MAILBOX.get(MailboxTag::BoardSerial);
                    let (vc0, vc1) = bcm::MAILBOX.get(MailboxTag::VcMemory);
                    println!("Board revision: {:x}", board);
                    println!("Board serial: {:x} {:x}", serial0, serial1);
                    println!("Arm memory(base, size): {:x}, {:x}", mem0, mem1);
                    println!("Vc memory(base, size):  {:x}, {:x}", vc0, vc1);
                }
                _ => {
                    if cnt > 0 {
                        println!("Unknown command: {:?}", &core::str::from_utf8(&array[0..cnt]).unwrap());
                        help();
                    }
                }
            }

            print!("\r# ");
            cnt = 0;
        }
        cnt += if c == '\r' { 0 } else { 1 };
    }
}
