use crate::kernel::{get_line, print};

const MAX_MODULES: usize = 16;

#[derive(Copy, Clone)] // Derive Copy and Clone traits
pub struct Command {
    pub name: &'static str,
    pub description: &'static str,
    pub func: fn(),
}

pub struct Console {
    commands: [Option<Command>; MAX_MODULES], // 假設MAX_MODULES已經定義
}

impl Console {
    pub fn new() -> Console {
        Console {
            commands: [None; MAX_MODULES],
        }
    }

    pub fn run(&self) {
        self.print_help();
        // print("Available commands:");
        print("simple-shell > ");
        let res = get_line();
        // let mut tbuffer: [u8; 5] = [1, 2, 3, 4, 5];
        // tbuffer[0] = b'a';
        // tbuffer[1] = b'b';
        // tbuffer[2] = b'c';
        // tbuffer[3] = b'd';
        // tbuffer[4] = b'e';
        // if tbuffer[0] == b'a' {
        //     // print(core::str::from_utf8(&tbuffer).unwrap());
        //     print("Hello, world!");
        //     // let res = get_line();
        // }
        // let len = get_line(&mut tbuffer);
        // let res = get_line();
        // if res.1[0] == b'a' {
        // print("Hello, world!");
        // }
        // let string = core::str::from_utf8(&res.1).unwrap();
        // print(string);
        // let _input = getstr();
        // match input {
        //     Ok(command_name) => match command_name {
        //         "help" => self.print_help(),
        //         command => self.execute_command(command),
        //     },
        //     Err(_) => {}
        // }
    }

    pub fn register_command(&mut self, command: Command) -> Result<(), &'static str> {
        for slot in self.commands.iter_mut() {
            if slot.is_none() {
                *slot = Some(command);
                return Ok(());
            }
        }
        Err("No space to register the module")
    }

    fn execute_command(&self, command_name: &str) {
        for slot in self.commands.iter() {
            if let Some(command) = slot {
                if command.name == command_name {
                    (command.func)();
                    return;
                }
            }
        }
        print("Command not found: ");
        print(command_name);
        print("\n");
    }

    fn print_help(&self) {
        // print("Available commands:");
        // for slot in self.commands.iter() {
        //     if let Some(command) = slot {
        //         print(command.name);
        //         print(": ");
        //         print(command.description);
        //         print("\n");
        //     }
        // }
    }
}
