pub mod commands;

use small_std::{fmt::print::console::console, print, println, string::String};

pub trait ShellCommand {
    fn name(&self) -> &str;
    fn help(&self) -> &str;
    fn execute(&self, args: &str);
}

pub struct Shell<'a> {
    buf: String,
    commands: alloc::vec::Vec<&'a dyn ShellCommand>,
}

impl<'a> Shell<'a> {
    pub fn new() -> Self {
        Self {
            buf: String::new(),
            commands: Default::default(),
        }
    }

    pub fn register(&mut self, command: &'a dyn ShellCommand) {
        self.commands.push(command);
    }

    pub fn run_loop(&mut self) -> ! {
        loop {
            self.show_prompt();
            self.read_input();
            self.handle_input();
        }
    }

    fn show_prompt(&self) {
        print!("$ ");
    }

    fn read_input(&mut self) {
        loop {
            let c = console().read_char();

            match c {
                '\r' | '\n' => {
                    println!();
                    return;
                }
                '\x08' | '\x7f' => self.on_backspace(),
                ' '..='~' => self.on_input(c),
                _ => continue,
            }
        }
    }

    fn on_backspace(&mut self) {
        if self.buf.is_empty() {
            return;
        }
        self.buf.pop();
        // go back, write space, then go back again
        print!("\x08 \x08");
    }

    fn on_input(&mut self, c: char) {
        self.buf.push(c);
        print!("{}", c);
    }

    fn handle_input(&mut self) {
        let input = self.buf.as_str().trim();
        let whitespace_idx = input.find(' ').unwrap_or(input.len());
        match input.split_at(whitespace_idx) {
            // Help is a special case, we can handle it here
            ("help", _) => self.help(),
            ("", _) => {}
            (cmd, args) => {
                let command = self.commands.iter().filter(|c| c.name() == cmd).next();
                command.map_or_else(
                    || println!("{}: command not found", cmd),
                    |c| c.execute(args),
                );
            }
        }
        self.buf.clear()
    }

    fn help(&self) {
        println!("{: <10}: {}", "help", "print this help menu");
        for cmd in self.commands.iter() {
            println!("{: <10}: {}", cmd.name(), cmd.help());
        }
    }
}
