use crate::{
    console::console,
    fork_test::fork_test,
    print, println,
    string::String,
    system_call::{exit, get_pid, kill, mbox_call},
};

pub struct Shell {
    input: String,
}

impl Default for Shell {
    fn default() -> Self {
        Shell::new()
    }
}

impl Shell {
    pub fn new() -> Self {
        Self {
            input: String::from(""),
        }
    }

    fn shell_hint(&self) {
        print!("# ");
    }

    pub fn run(&mut self) -> ! {
        self.shell_hint();
        loop {
            if let Some(c) = console().read_char() {
                match c {
                    '\r' | '\n' => {
                        self.execute_command();
                        self.shell_hint();
                    }
                    '\x08' | '\x7f' => self.backspace(),
                    ' '..='~' => self.press_key(c),
                    _ => (),
                }
            }
        }
    }

    fn help(&self) {
        println!("help\t: print this help menu");
        println!("info\t: print memory information");
        println!("pid\t: print process PID");
        println!("fork-test\t: run fork test");
        println!("exit\t: exit the process");
        println!("kill <pid>\t: kill a process")
    }

    fn info(&self) {
        #[repr(align(16))]
        struct GetBoardRevisionBuffer {
            inner: [u32; 7],
        }
        let mut buffer = GetBoardRevisionBuffer { inner: [0; 7] };
        // set buffer length (in bytes)
        buffer.inner[0] = 7 * 4;
        // set request code
        buffer.inner[1] = 0;
        // set tag tag identifier
        buffer.inner[2] = 0x00010002;
        // set value buffer length (in bytes)
        buffer.inner[3] = 4;
        // set tag request code for request
        buffer.inner[4] = 0;
        // set value buffer
        buffer.inner[5] = 0;
        // set end tag bits
        buffer.inner[6] = 0;
        mbox_call(8, buffer.inner.as_mut_ptr());
        println!("board revision: {:#08x}", buffer.inner[5]);
        #[repr(align(16))]
        struct GetARMMemoryBuffer {
            inner: [u32; 8],
        }
        let mut buffer = GetARMMemoryBuffer { inner: [0; 8] };
        // set buffer length (in bytes)
        buffer.inner[0] = 8 * 4;
        // set request code
        buffer.inner[1] = 0;
        // set tag tag identifier
        buffer.inner[2] = 0x00010005;
        // set value buffer length (in bytes)
        buffer.inner[3] = 8;
        // set tag request code for request
        buffer.inner[4] = 0;
        // set value buffer
        buffer.inner[5] = 0;
        buffer.inner[6] = 0;
        // set end tag bits
        buffer.inner[7] = 0;
        mbox_call(8, buffer.inner.as_mut_ptr());
        println!("ARM memory base address: {:#08x}", buffer.inner[5]);
        println!("ARM memory size: {} bytes", buffer.inner[6]);
    }

    fn pid(&self) {
        println!("PID: {}", get_pid());
    }

    fn exit(&self) {
        exit(0);
    }

    fn fork_test(&self) {
        fork_test();
    }

    fn kill(&self, pid: usize) {
        kill(pid as i32);
    }

    fn execute_command(&mut self) {
        println!();
        let input = self.input.trim();
        let mut split_result = input.split(" ");
        if let Some(cmd) = split_result.next() {
            match cmd {
                "help" => self.help(),
                "info" => self.info(),
                "pid" => self.pid(),
                "exit" => self.exit(),
                "fork-test" => self.fork_test(),
                "kill" => {
                    if let Some(pid) = split_result.next() {
                        if let Ok(pid) = pid.parse::<usize>() {
                            self.kill(pid);
                        } else {
                            println!("kill: invalid argument");
                        }
                    } else {
                        println!("kill: missing argument");
                    }
                }
                "" => (),
                cmd => println!("{}: command not found", cmd),
            }
        }
        self.input.clear();
    }

    fn press_key(&mut self, key: char) {
        self.input.push(key);
        print!("{}", key);
    }

    fn backspace(&mut self) {
        if self.input.is_empty() {
            return;
        }
        self.input.pop();
        // move the cursor to the previous character and overwrite it with a space
        // then move the cursor back again
        print!("\x08 \x08");
    }
}
