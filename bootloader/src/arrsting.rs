// #[derive(PartialEq)]
const MAXIUM_LEN: usize = 256;
pub struct ArrString {
    data: [u8; MAXIUM_LEN], // maxium size MAXIUM_LEN
    len: usize,
}

#[allow(dead_code)]
impl ArrString {
    pub fn new(s: &str) -> Self {
        let mut data: [u8; MAXIUM_LEN] = [0; MAXIUM_LEN];
        for (index, c) in s.chars().enumerate() {
            data[index] = c as u8;
        }
        ArrString {
            data: data,
            len: s.len(),
        }
    }

    pub fn get_len(&self) -> usize {
        self.len
    }

    pub fn get_data(&self) -> [u8 ;MAXIUM_LEN] {
        self.data
    }

    pub fn clean_buf(&mut self) {
        for x in self.data.as_mut() {
            *x = 0 as u8;
        }
        self.len = 0;
    }

    pub fn push_char(&mut self, c: char) {
        self.data[self.len] = c as u8;
        self.len += 1;
    }

    fn compare(&self, other: &Self) -> bool {
        self.data.starts_with(&other.data)
    }
}

#[allow(dead_code)]
impl PartialEq for ArrString {
    fn eq(&self, other: &Self) -> bool {
        self.compare(other)
    }
}
