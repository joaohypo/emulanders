use nx::result::*;
use nx::fs;

pub const SKYLANDER_DUMP_SIZE: usize = 1024;

#[derive(Clone, Debug)]
pub struct Skylander {
    pub data: [u8; SKYLANDER_DUMP_SIZE],
    pub path: alloc::string::String,
}

impl Skylander {
    pub fn load(path: alloc::string::String) -> Result<Self> {
        let mut file = fs::open_file(path.as_str(), fs::FileOpenOption::Read())?;
        let mut data = [0u8; SKYLANDER_DUMP_SIZE];
        file.read_array(&mut data)?;
        
        Ok(Self {
            data,
            path,
        })
    }

    pub fn get_uid(&self) -> &[u8] {
        // UID is in Sector 0, Block 0 (first 4 bytes usually for Mifare Classic)
        &self.data[0..4]
    }

    pub fn get_block(&self, sector: u8, block: u8) -> [u8; 16] {
        let idx = (sector as usize * 4 + block as usize) * 16;
        let mut data = [0u8; 16];
        if idx + 16 <= self.data.len() {
            data.copy_from_slice(&self.data[idx..idx+16]);
        }
        data
    }

    pub fn set_block(&mut self, sector: u8, block: u8, data: &[u8; 16]) {
        let idx = (sector as usize * 4 + block as usize) * 16;
        if idx + 16 <= self.data.len() {
            self.data[idx..idx+16].copy_from_slice(data);
        }
    }

    pub fn save(&self) -> Result<()> {
        let mut file = fs::open_file(self.path.as_str(), fs::FileOpenOption::Write())?;
        // We write the array back starting at offset 0, and flush to SD via the const generic parameter True 
        file.write_array::<u8, true>(&self.data)?;
        Ok(())
    }
}
