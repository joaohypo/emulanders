use nx::result::*;
use nx::ipc::sf;
use nx::service;
use nx::service::mii;
use nx::service::mii::{MiiDatabase, IMiiDatabaseClient, IStaticClient};
use nx::sync::sys::mutex::Mutex;
use nx::fs;
use alloc::vec::Vec;

pub use generic_once_cell::OnceCell;

static G_STATIC_SRV: OnceCell<Mutex, mii::StaticService> = OnceCell::new();
static G_DB_SRV: OnceCell<Mutex, MiiDatabase> = OnceCell::new();

#[inline]
fn get_database_service() -> Result<&'static MiiDatabase> {
    G_DB_SRV.get().ok_or(nx::rc::ResultNotInitialized::make())
}

pub fn initialize() -> Result<()> {
    let static_service = service::new_service_object::<mii::StaticService>()?;
    let db_service = static_service.get_database_service(mii::SpecialKeyCode::Normal)?;
    let _ = G_STATIC_SRV.set(static_service);
    let _ = G_DB_SRV.set(db_service);
    Ok(())
}

const DEFAULT_MII_NAME: &'static str = "emulande";

#[inline]
pub fn generate_random_mii() -> Result<mii::CharInfo> {
    let mut char_info = get_database_service()?.build_random(sf::EnumAsPrimitiveType::from(mii::Age::All), sf::EnumAsPrimitiveType::from(mii::Gender::All), sf::EnumAsPrimitiveType::from(mii::FaceColor::All))?;
    // Default name is "no name", use our own default instead
    char_info.name.set_str(DEFAULT_MII_NAME);
    Ok(char_info)
}

const MII_SOURCE_FLAG: mii::SourceFlag = mii::SourceFlag::Database();
pub const EXPORTED_MIIS_DIR: &'static str = "sdmc:/emulanders/miis";

pub fn export_miis() -> Result<()> {
    let mii_count = get_database_service()?.get_count(MII_SOURCE_FLAG)?;
    let mut miis: Vec<mii::CharInfo> = vec![Default::default(); mii_count as usize];

    let mii_total = get_database_service()?.get_one(MII_SOURCE_FLAG, sf::Buffer::from_mut_array(&mut miis))?;
    for i in 0..mii_total {
        let mii = miis[i as usize];

        let mii_dir_path = format!("{}/{}", EXPORTED_MIIS_DIR, i);
        fs::create_directory(mii_dir_path.as_str())?;

        let mii_path = format!("{}/mii-charinfo.bin", mii_dir_path);
        let mut mii_file = fs::open_file(mii_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
        mii_file.write_val::<_, true>(&mii)?;

        let mii_name = format!("{}/name.txt", mii_dir_path);
        let mut mii_name_file = fs::open_file(mii_name.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
        let actual_name = mii.name.get_string()?;
        mii_name_file.write_array::<_, true>(actual_name.as_bytes())?;
    }
    
    Ok(())
}