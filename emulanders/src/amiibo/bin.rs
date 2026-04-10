use aes::cipher::KeyIvInit;
use aes::cipher::StreamCipher;
use alloc::string::String;
use alloc::vec::Vec;
use hmac::digest::FixedOutput;
use nx::service::mii;
use nx::svc::rc::ResultInvalidSize;
use nx::util;
use nx::rand::RngCore;
use nx::result::*;
use nx::util::Uuid;
use crate::fsext;
use crate::miiext;
use super::ntag::Manufacturer1;
use super::ntag::Manufacturer2;
use super::{ntag, fmt};

use sha2::Sha256;
use hmac::{Hmac, Mac};

type Aes128Ctr= ctr::Ctr128LE<aes::Aes128>;
type HmacSha256 = Hmac<Sha256>;

pub trait Buffer: Sized {
    fn get_buf(&self) -> &[u8] {
        unsafe {
            core::slice::from_raw_parts(self as *const _ as *const u8, core::mem::size_of::<Self>())
        }
    }

    fn get_buf_mut(&mut self) -> &mut [u8] {
        unsafe {
            core::slice::from_raw_parts_mut(self as *mut _ as *mut u8, core::mem::size_of::<Self>())
        }
    }
}

pub struct DrbgContext {
    pub hmac_key: Vec<u8>,
    pub iteration: u16,
    pub buf: [u8; core::mem::size_of::<u16>() + Self::MAX_SEED_SIZE],
    pub buf_size: usize
}

impl DrbgContext {
    pub const MAX_SEED_SIZE: usize = 480;

    pub fn new(hmac_key: &[u8], seed: &[u8]) -> Result<Self> {
        let mut ctx = Self {
            hmac_key: hmac_key.to_vec(),
            iteration: 0,
            buf: [0; core::mem::size_of::<u16>() + Self::MAX_SEED_SIZE],
            buf_size: core::mem::size_of::<u16>() + seed.len().min(Self::MAX_SEED_SIZE)
        };

        unsafe {
            core::ptr::copy(seed.as_ptr(), ctx.buf.as_mut_ptr().add(core::mem::size_of::<u16>()), seed.len().min(Self::MAX_SEED_SIZE));
        }

        Ok(ctx)
    }

    pub fn step(&mut self, out_mac: &mut [u8]) -> Result<()> {
        unsafe {
            *(self.buf.as_mut_ptr() as *mut u16) = self.iteration.swap_bytes();
        }
        self.iteration += 1;

        let mut mac = HmacSha256::new_from_slice(&self.hmac_key).expect("HMAC-SHA256 allows keys of any size");
        mac.update(&self.buf[0..self.buf_size]);
        mac.finalize_into(out_mac.into());
        Ok(())
    }

    pub fn gen_bytes(hmac_key: &[u8], seed: &[u8], out_data: &mut [u8]) -> Result<()> {
        let mut ctx = Self::new(hmac_key, seed)?;

        let mut remaining_size = out_data.len();
        let mut cur_buf = out_data.as_mut_ptr();

        let mut tmp_buf = [0u8; 0x20];
        while remaining_size > 0 {
            ctx.step(&mut tmp_buf)?;

            let step_size = remaining_size.min(0x20);
            unsafe {
                core::ptr::copy(tmp_buf.as_ptr(), cur_buf, step_size);
                cur_buf = cur_buf.add(step_size);
            }
            remaining_size -= step_size;
        }

        Ok(())
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct RetailKey {
    pub hmac_key: [u8; 0x10],
    pub phrase: util::ArrayString<0xE>,
    pub reserved: u8,
    pub seed_size: u8,
    pub seed: [u8; Self::MAX_SEED_SIZE],
    pub xor_pad: [u8; 0x20]
}
const_assert!(core::mem::size_of::<RetailKey>() == 80);

impl RetailKey {
    pub const MAX_SEED_SIZE: usize = 0x10;
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C,)]
pub struct DerivedKey {
    aes_key: [u8; 0x10],
    aes_iv: [u8; 0x10],
    hmac_key: [u8; 0x10]
}
const_assert!(core::mem::size_of::<DerivedKey>() == 0x30);

impl Buffer for DerivedKey {}

impl DerivedKey {
    pub fn derive_from(key: &RetailKey, base_seed: &[u8]) -> Result<Self> {
        let mut prepared_seed = [0u8; DrbgContext::MAX_SEED_SIZE];

        let prepared_seed_buf_start = prepared_seed.as_mut_ptr();
        let mut prepared_seed_buf_cursor = prepared_seed_buf_start;

        unsafe {
            let phrase = key.phrase.as_bytes();
            core::ptr::copy(phrase.as_ptr(), prepared_seed_buf_cursor, phrase.len());
            prepared_seed_buf_cursor = prepared_seed_buf_cursor.add(phrase.len());

            let seed_size = key.seed_size as usize;
            let leading_seed_bytes = RetailKey::MAX_SEED_SIZE - seed_size;
            core::ptr::copy(base_seed.as_ptr(), prepared_seed_buf_cursor, leading_seed_bytes);
            prepared_seed_buf_cursor = prepared_seed_buf_cursor.add(leading_seed_bytes);

            core::ptr::copy(key.seed.as_ptr(), prepared_seed_buf_cursor, seed_size);
            prepared_seed_buf_cursor = prepared_seed_buf_cursor.add(seed_size);

            let seed_step_size = 0x10usize;
            core::ptr::copy(base_seed.as_ptr().add(0x10), prepared_seed_buf_cursor, seed_step_size);
            prepared_seed_buf_cursor = prepared_seed_buf_cursor.add(seed_step_size);

            for i in 0..key.xor_pad.len() {
                *prepared_seed_buf_cursor.add(i) = base_seed[0x20 + i] ^ key.xor_pad[i];
            }
            prepared_seed_buf_cursor = prepared_seed_buf_cursor.add(key.xor_pad.len());

            let prepared_seed_size = prepared_seed_buf_cursor.offset_from(prepared_seed_buf_start) as usize;
            let mut derived_key: Self = Default::default();
            DrbgContext::gen_bytes(&key.hmac_key, &prepared_seed[0..prepared_seed_size], derived_key.get_buf_mut())?;

            Ok(derived_key)
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct RetailKeySet {
    pub data_key: RetailKey,
    pub tag_key: RetailKey
}
const_assert!(core::mem::size_of::<RetailKeySet>() == 2 * core::mem::size_of::<RetailKey>());

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct DerivedKeySet {
    pub data_key: DerivedKey,
    pub tag_key: DerivedKey
}
const_assert!(core::mem::size_of::<DerivedKeySet>() == 2 * core::mem::size_of::<DerivedKey>());

impl DerivedKeySet {
    #[inline]
    pub fn derive_from(key_set: &RetailKeySet, base_seed: &[u8]) -> Result<Self> {
        Ok(Self {
            data_key: DerivedKey::derive_from(&key_set.data_key, base_seed)?,
            tag_key: DerivedKey::derive_from(&key_set.tag_key, base_seed)?
        })
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct KeyDerivationSeed {
    pub unk_1: u8,
    pub write_counter: u8,
    pub pad_zero: [u8; 0xE],
    pub man_info_1s: [ntag::Manufacturer1; 2],
    pub unk_maybe_hash: [u8; 0x20]
}
const_assert!(core::mem::size_of::<KeyDerivationSeed>() == 0x40);

impl KeyDerivationSeed {
    pub const fn from_converted(conv: &ConvertedFormat) -> Self {
        Self {
            unk_1: conv.st_2.unk_1,
            write_counter: conv.st_2.write_counter,
            pad_zero: [0; 0xE],
            man_info_1s: [conv.man_info_1; 2],
            unk_maybe_hash: conv.st_1.unk_maybe_hash
        }
    }
}

impl Buffer for KeyDerivationSeed {}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct AmiiboId {
    pub game_character_id: u16,
    pub character_variant: u8,
    pub figure_type: u8,
    pub model_number: u16,
    pub series: u8,
    pub unk_0x2: u8
}
const_assert!(core::mem::size_of::<AmiiboId>() == 0x8);

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct Struct1 {
    pub amiibo_id: AmiiboId,
    pub unk: [u8; 0x4],
    pub unk_maybe_hash: [u8; 0x20]
}
const_assert!(core::mem::size_of::<Struct1>() == 0x2C);

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct Struct2 {
    pub unk_0xa5: u8,
    pub unk_1: u8,
    pub write_counter: u8,
    pub unk_2: u8
}
const_assert!(core::mem::size_of::<Struct2>() == 0x4);

define_bit_set! {
    Flags (u8) {
        None = 0,
        // TODO: more flags here? bits 0-3 are probably for internal use as they get masked out in both 3DS and here...
        Initialized = bit!(4),
        ApplicationAreaUsed = bit!(5)
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(u8)]
#[allow(dead_code)]
pub enum CountryCode {
    #[default]
    Invalid = 0,

    // Note: https://wiibrew.org/wiki/Country_Codes

    // Japan
    Japan = 1,

    // Americas
    Anguilla = 8,
    AntiguaAndBarbuda = 9,
    Argentina = 10,
    Aruba = 11,
    Bahamas = 12,
    Barbados = 13,
    Belize = 14,
    Bolivia = 15,
    Brazil = 16,
    BritishVirginIslands = 17,
    Canada = 18,
    CaymanIslands = 19,
    Chile = 20,
    Colombia = 21,
    CostaRica = 22,
    Dominica = 23,
    DominicanRepublic = 24,
    Ecuador = 25,
    ElSalvador = 26,
    FrenchGuiana = 27,
    Grenada = 28,
    Guadeloupe = 29,
    Guatemala = 30,
    Guyana = 31,
    Haiti = 32,
    Honduras = 33,
    Jamaica = 34,
    Martinique = 35,
    Mexico = 36,
    Monsterrat = 37,
    NetherlandsAntilles = 38,
    Nicaragua = 39,
    Panama = 40,
    Paraguay = 41,
    Peru = 42,
    StKittsAndNevis = 43,
    StLucia = 44,
    StVincentAndTheGrenadines = 45,
    Suriname = 46,
    TrinidadAndTobago = 47,
    TurksAndCaicosIslands = 48,
    UnitedStates = 49,
    Uruguay = 50,
    USVirginIslands = 51,
    Venezuela = 52,

    // Europe & Africa
    Albania = 64,
    Australia = 65,
    Austria = 66,
    Belgium = 67,
    BosniaAndHerzegovina = 68,
    Botswana = 69,
    Bulgaria = 70,
    Croatia = 71,
    Cyprus = 72,
    CzechRepublic = 73,
    Denmark = 74,
    Estonia = 75,
    Finland = 76,
    France = 77,
    Germany = 78,
    Greece = 79,
    Hungary = 80,
    Iceland = 81,
    Ireland = 82,
    Italy = 83,
    Latvia = 84,
    Lesotho = 85,
    Lichtenstein = 86,
    Lithuania = 87,
    Luxembourg = 88,
    NorthMacedonia = 89,
    Malta = 90,
    Montenegro = 91,
    Mozambique = 92,
    Namibia = 93,
    Netherlands = 94,
    NewZealand = 95,
    Norway = 96,
    Poland = 97,
    Portugal = 98,
    Romania = 99,
    Russia = 100,
    Serbia = 101,
    Slovakia = 102,
    Slovenia = 103,
    SouthAfrica = 104,
    Spain = 105,
    Swaziland = 106,
    Sweden = 107,
    Switzerland = 108,
    Turkey = 109,
    UnitedKingdom = 110,
    Zambia = 111,
    Zimbabwe = 112,
    Azerbaijan = 113,
    Mauritania = 114,
    Mali = 115,
    Niger = 116,
    Chad = 117,
    Sudan = 118,
    Eritrea = 119,
    Djibouti = 120,
    Somalia = 121,

    // Southeast Asia
    Taiwan = 128,
    SouthKorea = 136,
    HongKong = 144,
    Macao = 145,
    Indonesia = 152,
    Singapore = 153,
    Thailand = 154,
    Philippines = 155,
    Malaysia = 156,
    China = 160,

    // Middle East
    UnitedArabEmirates = 168,
    India = 169,
    Egypt = 170,
    Oman = 171,
    Qatar = 172,
    Kuwait = 173,
    SaudiArabia = 174,
    Syria = 175,
    Bahrain = 176,
    Jordan = 177
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct Date {
    pub val_be: u16
}

impl Date {
    pub const fn new(year: u16, month: u8, day: u8) -> Self {
        let mut val_be: u16 = 0;
        write_bits!(0, 4, val_be, day as u16);
        write_bits!(5, 8, val_be, month as u16);
        write_bits!(9, 15, val_be, year - 2000);
        Self { val_be }
    }

    pub const fn get_day(&self) -> u8 {
        (read_bits!(0, 4, self.val_be)) as u8
    }

    pub const fn get_month(&self) -> u8 {
        (read_bits!(5, 8, self.val_be)) as u8
    }

    pub const fn get_year(&self) -> u16 {
        read_bits!(9, 15, self.val_be) + 2000
    }

    pub const fn to_virtual_amiibo_date(&self) -> fmt::VirtualAmiiboDate {
        fmt::VirtualAmiiboDate {
            y: self.get_year(),
            m: self.get_month(),
            d: self.get_day()
        }
    }
}
const_assert!(core::mem::size_of::<Date>() == 0x2);

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C, packed)]
pub struct MiiFormat {
    // Note: 3DS mii format (https://www.3dbrew.org/wiki/Mii#Mii_format)

    pub version: u8,
    pub info_1_bf: u8,
    pub info_2_bf: u8,
    pub info_3_bf: u8,
    pub system_id: u64,
    pub mii_id_bf: u32,
    pub mac_addr: [u8; 6],
    pub pad: [u8; 2],
    pub info_4_bf: u16,
    pub name: util::ArrayWideString<10>,
    pub height: u8,
    pub build: u8,
    pub faceline_info_1_bf: u8,
    pub faceline_info_2_bf: u8,
    pub hair_type: u8,
    pub hair_info_bf: u8,
    pub eye_info_bf: u32,
    pub eyebrow_info_bf: u32,
    pub nose_info_bf: u16,
    pub mouth_info_1_bf: u16,
    pub mouth_mustache_info_bf: u16,
    pub mustache_beard_info_bf: u16,
    pub glass_info_bf: u16,
    pub mole_info_bf: u16,
    pub author_name: util::ArrayWideString<10>,
    pub unk: u16,
    pub crc16: u16
}
const_assert!(core::mem::size_of::<MiiFormat>() == 0x60);

#[allow(dead_code)]
impl MiiFormat {
    // Info1
    #[inline]
    pub fn get_character_set(&self) -> u8 {
        read_bits!(2, 3, self.info_1_bf)
    }

    #[inline]
    pub fn set_character_set(&mut self, set: u8) {
        write_bits!(2, 3, self.info_1_bf, set);
    }

    #[inline]
    pub fn get_region_lock(&self) -> u8 {
        read_bits!(4, 5, self.info_1_bf)
    }

    #[inline]
    pub fn set_region_lock(&mut self, lock: u8) {
        write_bits!(4, 5, self.info_1_bf, lock);
    }

    #[inline]
    pub fn get_profanity_flag(&self) -> bool {
        read_bits!(6, 6, self.info_1_bf) != 0
    }

    #[inline]
    pub fn set_profanity_flag(&mut self, profanity: bool) {
        write_bits!(6, 6, self.info_1_bf, profanity as u8);
    }

    #[inline]
    pub fn get_allow_copying(&self) -> bool {
        read_bits!(7, 7, self.info_1_bf) != 0
    }

    #[inline]
    pub fn set_allow_copying(&mut self, allow: bool) {
        write_bits!(7, 7, self.info_1_bf, allow as u8);
    }

    // Info2
    #[inline]
    pub fn get_slot_index(&self) -> u8 {
        read_bits!(0, 3, self.info_2_bf)
    }

    #[inline]
    pub fn set_slot_index(&mut self, idx: u8) {
        write_bits!(0, 3, self.info_2_bf, idx);
    }

    #[inline]
    pub fn get_page_index(&self) -> u8 {
        read_bits!(4, 7, self.info_2_bf)
    }

    #[inline]
    pub fn set_page_index(&mut self, idx: u8) {
        write_bits!(4, 7, self.info_2_bf, idx);
    }

    // Info3
    #[inline]
    pub fn get_console_kind(&self) -> u8 {
        read_bits!(1, 3, self.info_3_bf)
    }

    #[inline]
    pub fn set_console_kind(&mut self, kind: u8) {
        write_bits!(1, 3, self.info_3_bf, kind);
    }

    // Info4
    #[inline]
    pub fn get_favorite_mii_flag(&self) -> bool {
        read_bits!(1, 1, self.info_4_bf) as u8 != 0
    }

    #[inline]
    pub fn set_favorite_mii_flag(&mut self, favorite: bool) {
        write_bits!(1, 1, self.info_4_bf, favorite as u16);
    }

    #[inline]
    pub fn get_favorite_color(&self) -> u8 {
        read_bits!(2, 5, self.info_4_bf) as u8
    }

    #[inline]
    pub fn set_favorite_color(&mut self, color: u8) {
        write_bits!(2, 5, self.info_4_bf, color as u16);
    }

    #[inline]
    pub fn get_birthday_day(&self) -> u8 {
        read_bits!(6, 10, self.info_4_bf) as u8
    }

    #[inline]
    pub fn set_birthday_day(&mut self, day: u8) {
        write_bits!(6, 10, self.info_4_bf, day as u16);
    }

    #[inline]
    pub fn get_birthday_month(&self) -> u8 {
        read_bits!(11, 14, self.info_4_bf) as u8
    }

    #[inline]
    pub fn set_birthday_month(&mut self, month: u8) {
        write_bits!(11, 14, self.info_4_bf, month as u16);
    }

    #[inline]
    pub fn get_gender(&self) -> u8 {
        read_bits!(15, 15, self.info_4_bf) as u8
    }

    #[inline]
    pub fn set_gender(&mut self, gender: u8) {
        write_bits!(15, 15, self.info_4_bf, gender as u16);
    }

    // MiiId
    #[inline]
    pub fn get_not_special_flag(&self) -> bool {
        read_bits!(0, 0, self.mii_id_bf) as u8 != 0
    }

    #[inline]
    pub fn set_not_special_flag(&mut self, not_special: bool) {
        write_bits!(0, 0, self.mii_id_bf, not_special as u32);
    }

    #[inline]
    pub fn get_maybe_dsi_mii_flag(&self) -> bool {
        read_bits!(1, 1, self.mii_id_bf) as u8 != 0
    }

    #[inline]
    pub fn set_maybe_dsi_mii_flag(&mut self, maybe_dsi_mii: bool) {
        write_bits!(1, 1, self.mii_id_bf, maybe_dsi_mii as u32);
    }

    #[inline]
    pub fn get_temp_mii_flag(&self) -> bool {
        read_bits!(2, 2, self.mii_id_bf) as u8 != 0
    }

    #[inline]
    pub fn set_temp_mii_flag(&mut self, temp_mii: bool) {
        write_bits!(2, 2, self.mii_id_bf, temp_mii as u32);
    }

    #[inline]
    pub fn get_unk_flag(&self) -> bool {
        read_bits!(3, 3, self.mii_id_bf) as u8 != 0
    }

    #[inline]
    pub fn set_unk_flag(&mut self, unk: bool) {
        write_bits!(3, 3, self.mii_id_bf, unk as u32);
    }

    #[inline]
    pub fn get_creation_date_timestamp(&self) -> u32 {
        read_bits!(4, 31, self.mii_id_bf)
    }

    #[inline]
    pub fn set_creation_date_timestamp(&mut self, ts: u32) {
        write_bits!(4, 31, self.mii_id_bf, ts);
    }

    // FacelineInfo1
    #[inline]
    pub fn get_faceline_color(&self) -> u8 {
        read_bits!(0, 2, self.faceline_info_1_bf)
    }

    #[inline]
    pub fn set_faceline_color(&mut self, color: u8) {
        write_bits!(0, 2, self.faceline_info_1_bf, color);
    }

    #[inline]
    pub fn get_faceline_type(&self) -> u8 {
        read_bits!(3, 6, self.faceline_info_1_bf)
    }

    #[inline]
    pub fn set_faceline_type(&mut self, f_type: u8) {
        write_bits!(3, 6, self.faceline_info_1_bf, f_type);
    }

    #[inline]
    pub fn get_disable_sharing_flag(&self) -> bool {
        read_bits!(7, 7, self.faceline_info_1_bf) != 0
    }

    #[inline]
    pub fn set_disable_sharing_flag(&mut self, disable_sharing: bool) {
        write_bits!(7, 7, self.faceline_info_1_bf, disable_sharing as u8);
    }

    // FacelineInfo2
    #[inline]
    pub fn get_faceline_make(&self) -> u8 {
        read_bits!(0, 3, self.faceline_info_2_bf)
    }

    #[inline]
    pub fn set_faceline_make(&mut self, make: u8) {
        write_bits!(0, 3, self.faceline_info_2_bf, make);
    }

    #[inline]
    pub fn get_faceline_wrinkle(&self) -> u8 {
        read_bits!(4, 7, self.faceline_info_2_bf)
    }

    #[inline]
    pub fn set_faceline_wrinkle(&mut self, wrinkle: u8) {
        write_bits!(4, 7, self.faceline_info_2_bf, wrinkle);
    }

    // HairInfo
    #[inline]
    pub fn get_hair_flip(&self) -> u8 {
        read_bits!(4, 4, self.hair_info_bf)
    }

    #[inline]
    pub fn set_hair_flip(&mut self, flip: u8) {
        write_bits!(4, 4, self.hair_info_bf, flip);
    }

    #[inline]
    pub fn get_hair_color(&self) -> u8 {
        read_bits!(5, 7, self.hair_info_bf)
    }

    #[inline]
    pub fn set_hair_color(&mut self, color: u8) {
        write_bits!(5, 7, self.hair_info_bf, color);
    }

    // EyeInfo
    #[inline]
    pub fn get_eye_y(&self) -> u8 {
        read_bits!(2, 6, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_y(&mut self, y: u8) {
        write_bits!(2, 6, self.eye_info_bf, y as u32);
    }

    #[inline]
    pub fn get_eye_x(&self) -> u8 {
        read_bits!(7, 10, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_x(&mut self, x: u8) {
        write_bits!(7, 10, self.eye_info_bf, x as u32);
    }

    #[inline]
    pub fn get_eye_rotate(&self) -> u8 {
        read_bits!(11, 15, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_rotate(&mut self, rotate: u8) {
        write_bits!(11, 15, self.eye_info_bf, rotate as u32);
    }

    #[inline]
    pub fn get_eye_aspect(&self) -> u8 {
        read_bits!(16, 18, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_aspect(&mut self, aspect: u8) {
        write_bits!(16, 18, self.eye_info_bf, aspect as u32);
    }

    #[inline]
    pub fn get_eye_scale(&self) -> u8 {
        read_bits!(19, 22, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_scale(&mut self, scale: u8) {
        write_bits!(19, 22, self.eye_info_bf, scale as u32);
    }

    #[inline]
    pub fn get_eye_color(&self) -> u8 {
        read_bits!(23, 25, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_color(&mut self, color: u8) {
        write_bits!(23, 25, self.eye_info_bf, color as u32);
    }

    #[inline]
    pub fn get_eye_type(&self) -> u8 {
        read_bits!(26, 31, self.eye_info_bf) as u8
    }

    #[inline]
    pub fn set_eye_type(&mut self, e_type: u8) {
        write_bits!(26, 31, self.eye_info_bf, e_type as u32);
    }

    // EyebrowInfo
    #[inline]
    pub fn get_eyebrow_y(&self) -> u8 {
        read_bits!(2, 6, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_y(&mut self, y: u8) {
        write_bits!(2, 6, self.eyebrow_info_bf, y as u32);
    }

    #[inline]
    pub fn get_eyebrow_x(&self) -> u8 {
        read_bits!(7, 10, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_x(&mut self, x: u8) {
        write_bits!(7, 10, self.eyebrow_info_bf, x as u32);
    }

    #[inline]
    pub fn get_eyebrow_rotate(&self) -> u8 {
        read_bits!(11, 14, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_rotate(&mut self, rotate: u8) {
        write_bits!(11, 14, self.eyebrow_info_bf, rotate as u32);
    }

    #[inline]
    pub fn get_eyebrow_aspect(&self) -> u8 {
        read_bits!(17, 19, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_aspect(&mut self, aspect: u8) {
        write_bits!(17, 19, self.eyebrow_info_bf, aspect as u32);
    }

    #[inline]
    pub fn get_eyebrow_scale(&self) -> u8 {
        read_bits!(20, 23, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_scale(&mut self, scale: u8) {
        write_bits!(20, 23, self.eyebrow_info_bf, scale as u32);
    }

    #[inline]
    pub fn get_eyebrow_color(&self) -> u8 {
        read_bits!(24, 26, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_color(&mut self, color: u8) {
        write_bits!(24, 26, self.eyebrow_info_bf, color as u32);
    }

    #[inline]
    pub fn get_eyebrow_type(&self) -> u8 {
        read_bits!(27, 31, self.eyebrow_info_bf) as u8
    }

    #[inline]
    pub fn set_eyebrow_type(&mut self, e_type: u8) {
        write_bits!(27, 31, self.eyebrow_info_bf, e_type as u32);
    }

    // NoseInfo
    #[inline]
    pub fn get_nose_y(&self) -> u8 {
        read_bits!(2, 6, self.nose_info_bf) as u8
    }

    #[inline]
    pub fn set_nose_y(&mut self, y: u8) {
        write_bits!(2, 6, self.nose_info_bf, y as u16);
    }

    #[inline]
    pub fn get_nose_scale(&self) -> u8 {
        read_bits!(7, 10, self.nose_info_bf) as u8
    }

    #[inline]
    pub fn set_nose_scale(&mut self, scale: u8) {
        write_bits!(7, 10, self.nose_info_bf, scale as u16);
    }

    #[inline]
    pub fn get_nose_type(&self) -> u8 {
        read_bits!(11, 15, self.nose_info_bf) as u8
    }

    #[inline]
    pub fn set_nose_type(&mut self, n_type: u8) {
        write_bits!(11, 15, self.nose_info_bf, n_type as u16);
    }

    // MouthInfo1
    #[inline]
    pub fn get_mouth_aspect(&self) -> u8 {
        read_bits!(0, 2, self.mouth_info_1_bf) as u8
    }

    #[inline]
    pub fn set_mouth_aspect(&mut self, aspect: u8) {
        write_bits!(0, 2, self.mouth_info_1_bf, aspect as u16);
    }

    #[inline]
    pub fn get_mouth_scale(&self) -> u8 {
        read_bits!(3, 6, self.mouth_info_1_bf) as u8
    }

    #[inline]
    pub fn set_mouth_scale(&mut self, scale: u8) {
        write_bits!(3, 6, self.mouth_info_1_bf, scale as u16);
    }

    #[inline]
    pub fn get_mouth_color(&self) -> u8 {
        read_bits!(7, 9, self.mouth_info_1_bf) as u8
    }

    #[inline]
    pub fn set_mouth_color(&mut self, color: u8) {
        write_bits!(7, 9, self.mouth_info_1_bf, color as u16);
    }

    #[inline]
    pub fn get_mouth_type(&self) -> u8 {
        read_bits!(10, 15, self.mouth_info_1_bf) as u8
    }

    #[inline]
    pub fn set_mouth_type(&mut self, m_type: u8) {
        write_bits!(10, 15, self.mouth_info_1_bf, m_type as u16);
    }

    // MouthMustacheInfo
    #[inline]
    pub fn get_mustache_type(&self) -> u8 {
        read_bits!(8, 10, self.mouth_mustache_info_bf) as u8
    }

    #[inline]
    pub fn set_mustache_type(&mut self, m_type: u8) {
        write_bits!(8, 10, self.mouth_mustache_info_bf, m_type as u16);
    }

    #[inline]
    pub fn get_mouth_y(&self) -> u8 {
        read_bits!(11, 15, self.mouth_mustache_info_bf) as u8
    }

    #[inline]
    pub fn set_mouth_y(&mut self, y: u8) {
        write_bits!(11, 15, self.mouth_mustache_info_bf, y as u16);
    }

    // MustacheBeardInfo
    #[inline]
    pub fn get_mustache_y(&self) -> u8 {
        read_bits!(1, 5, self.mustache_beard_info_bf) as u8
    }

    #[inline]
    pub fn set_mustache_y(&mut self, y: u8) {
        write_bits!(1, 5, self.mustache_beard_info_bf, y as u16);
    }

    #[inline]
    pub fn get_mustache_scale(&self) -> u8 {
        read_bits!(6, 9, self.mustache_beard_info_bf) as u8
    }

    #[inline]
    pub fn set_mustache_scale(&mut self, scale: u8) {
        write_bits!(6, 9, self.mustache_beard_info_bf, scale as u16);
    }

    #[inline]
    pub fn get_beard_color(&self) -> u8 {
        read_bits!(10, 12, self.mustache_beard_info_bf) as u8
    }

    #[inline]
    pub fn set_beard_color(&mut self, color: u8) {
        write_bits!(10, 12, self.mustache_beard_info_bf, color as u16);
    }

    #[inline]
    pub fn get_beard_type(&self) -> u8 {
        read_bits!(13, 15, self.mustache_beard_info_bf) as u8
    }

    #[inline]
    pub fn set_beard_type(&mut self, b_type: u8) {
        write_bits!(13, 15, self.mustache_beard_info_bf, b_type as u16);
    }

    // GlassInfo
    #[inline]
    pub fn get_glass_y(&self) -> u8 {
        read_bits!(0, 4, self.glass_info_bf) as u8
    }

    #[inline]
    pub fn set_glass_y(&mut self, y: u8) {
        write_bits!(0, 4, self.glass_info_bf, y as u16);
    }

    #[inline]
    pub fn get_glass_scale(&self) -> u8 {
        read_bits!(5, 8, self.glass_info_bf) as u8
    }

    #[inline]
    pub fn set_glass_scale(&mut self, scale: u8) {
        write_bits!(5, 8, self.glass_info_bf, scale as u16);
    }

    #[inline]
    pub fn get_glass_color(&self) -> u8 {
        read_bits!(9, 11, self.glass_info_bf) as u8
    }

    #[inline]
    pub fn set_glass_color(&mut self, color: u8) {
        write_bits!(9, 11, self.glass_info_bf, color as u16);
    }

    #[inline]
    pub fn get_glass_type(&self) -> u8 {
        read_bits!(12, 15, self.glass_info_bf) as u8
    }

    #[inline]
    pub fn set_glass_type(&mut self, g_type: u8) {
        write_bits!(12, 15, self.glass_info_bf, g_type as u16);
    }

    // MoleInfo
    #[inline]
    pub fn get_mole_y(&self) -> u8 {
        read_bits!(1, 5, self.mole_info_bf) as u8
    }

    #[inline]
    pub fn set_mole_y(&mut self, y: u8) {
        write_bits!(1, 5, self.mole_info_bf, y as u16);
    }

    #[inline]
    pub fn get_mole_x(&self) -> u8 {
        read_bits!(6, 10, self.mole_info_bf) as u8
    }

    #[inline]
    pub fn set_mole_x(&mut self, x: u8) {
        write_bits!(6, 10, self.mole_info_bf, x as u16);
    }

    #[inline]
    pub fn get_mole_scale(&self) -> u8 {
        read_bits!(11, 14, self.mole_info_bf) as u8
    }

    #[inline]
    pub fn set_mole_scale(&mut self, scale: u8) {
        write_bits!(11, 14, self.mole_info_bf, scale as u16);
    }

    #[inline]
    pub fn get_mole_type(&self) -> u8 {
        read_bits!(15, 15, self.mole_info_bf) as u8
    }

    #[inline]
    pub fn set_mole_type(&mut self, m_type: u8) {
        write_bits!(15, 15, self.mole_info_bf, m_type as u16);
    }

    pub unsafe fn to_charinfo(&self) -> Result<mii::CharInfo> {
        Ok(mii::CharInfo {
            id: {
                let mut uuid: Uuid = Default::default();
                nx::rand::get_rng()?.fill_bytes(&mut uuid.uuid);
                uuid
            },
            name: {
                let name = self.name;
                util::ArrayWideString::from_string(name.get_string()?)
            },
            font_region: mii::FontRegion::Standard,
            faceline_color: unsafe { core::mem::transmute(self.get_favorite_color()) },
            gender: unsafe { core::mem::transmute(self.get_gender()) },
            height: self.height,
            build: self.build,
            type_val: 0,
            region_move: !self.get_disable_sharing_flag() as u8,
            faceline_type: unsafe { core::mem::transmute(self.get_faceline_type()) },
            favorite_color: self.get_faceline_color(),
            faceline_wrinkle: unsafe { core::mem::transmute(self.get_faceline_wrinkle()) },
            faceline_make: unsafe { core::mem::transmute(self.get_faceline_make()) },
            hair_type: unsafe { core::mem::transmute(self.hair_type) },
            hair_color: self.get_hair_color(),
            hair_flip: unsafe { core::mem::transmute(self.get_hair_flip()) },
            eye_type: unsafe { core::mem::transmute(self.get_eye_type()) },
            eye_color: self.get_eye_color(),
            eye_scale: self.get_eye_scale(),
            eye_aspect: self.get_eye_aspect(),
            eye_rotate: self.get_eye_rotate(),
            eye_x: self.get_eye_x(),
            eye_y: self.get_eye_y(),
            eyebrow_type: unsafe { core::mem::transmute(self.get_eyebrow_type()) },
            eyebrow_color: self.get_eyebrow_color(),
            eyebrow_scale: self.get_eyebrow_scale(),
            eyebrow_aspect: self.get_eyebrow_aspect(),
            eyebrow_rotate: self.get_eyebrow_rotate(),
            eyebrow_x: self.get_eyebrow_x(),
            eyebrow_y: self.get_eyebrow_y(),
            nose_type: unsafe { core::mem::transmute(self.get_nose_type()) },
            nose_scale: self.get_nose_scale(),
            nose_y: self.get_nose_y(),
            mouth_type: unsafe { core::mem::transmute(self.get_mouth_type()) },
            mouth_color: self.get_mouth_color(),
            mouth_scale: self.get_mouth_scale(),
            mouth_aspect: self.get_mouth_aspect(),
            mouth_y: self.get_mouth_y(),
            beard_color: self.get_beard_color(),
            beard_type: unsafe { core::mem::transmute(self.get_beard_type()) },
            mustache_type: unsafe { core::mem::transmute(self.get_mustache_type()) },
            mustache_scale: self.get_mustache_scale(),
            mustache_y: self.get_mustache_y(),
            glass_type: unsafe { core::mem::transmute(self.get_glass_type()) },
            glass_color: self.get_glass_color(),
            glass_scale: self.get_glass_scale(),
            glass_y: self.get_glass_y(),
            mole_type: unsafe { core::mem::transmute(self.get_mole_type()) },
            mole_scale: self.get_mole_scale(),
            mole_x: self.get_mole_x(),
            mole_y: self.get_mole_y(),
            reserved: 0
        })
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(C, packed)]
pub struct Settings {
    pub flags: Flags,
    pub country_code: CountryCode,
    pub crc32_write_counter_be: u16,
    pub first_write_date_be: Date,
    pub last_write_date_be: Date,
    pub crc32_be: u32,
    pub name_be: util::ArrayWideString<10>,
    pub mii_be: MiiFormat,
    pub program_id_be: u64,
    pub write_counter_be: u16,
    pub access_id_be: u32,
    pub unk: [u8; 0x2],
    pub unk_maybe_hash: [u8; 0x20]
}
const_assert!(core::mem::size_of::<Settings>() == 0xB0);

// Easter egg default date: amiibo's first release
pub const DEFAULT_SETTINGS_WRITE_DATE: Date = Date::new(2014, 6, 10);

impl Default for Settings {
    fn default() -> Self {
        Self {
            flags: Flags::None(),
            country_code: CountryCode::Spain,
            crc32_write_counter_be: 0,
            first_write_date_be: DEFAULT_SETTINGS_WRITE_DATE,
            last_write_date_be: DEFAULT_SETTINGS_WRITE_DATE,
            crc32_be: 0,
            name_be: util::ArrayWideString::new(),
            mii_be: Default::default(),
            program_id_be: 0,
            write_counter_be: 0,
            access_id_be: 0,
            unk: [0; 0x2],
            unk_maybe_hash: [0; 0x20]
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct EncryptedData {
    pub section_1: [u8; 0x20],
    pub section_2: [u8; 0x114],
    pub section_3: [u8; 0x54]
}
const_assert!(core::mem::size_of::<EncryptedData>() == 0x188);

impl Default for EncryptedData {
    fn default() -> Self {
        Self {
            section_1: [0; 0x20],
            section_2: [0; 0x114],
            section_3: [0; 0x54]
        }
    }
}

impl Buffer for EncryptedData {}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct DecryptedData {
    pub settings: Settings,
    pub app_area: [u8; super::APP_AREA_SIZE]
}
const_assert!(core::mem::size_of::<EncryptedData>() == core::mem::size_of::<DecryptedData>());

impl Default for DecryptedData {
    fn default() -> Self {
        Self {
            settings: Default::default(),
            app_area: [0; super::APP_AREA_SIZE]
        }
    }
}

impl Buffer for DecryptedData {}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct RawFormat {
    pub man_info_1: ntag::Manufacturer1,
    pub man_info_2: ntag::Manufacturer2,
    pub st_2: Struct2,
    pub enc_section_1: [u8; 0x20],
    pub tag_sha256_hmac_hash: [u8; 0x20],
    pub st_1: Struct1,
    pub data_sha256_hmac_hash: [u8; 0x20],
    pub enc_section_2: [u8; 0x114],
    pub enc_section_3: [u8; 0x54],
    pub dyn_lock: ntag::DynamicLock,
    pub cfg: ntag::Config
}
const_assert!(core::mem::size_of::<RawFormat>() == 532);

impl Default for RawFormat {
    fn default() -> Self {
        Self {
            man_info_1: Default::default(),
            man_info_2: Default::default(),
            st_2: Default::default(),
            enc_section_1: Default::default(),
            tag_sha256_hmac_hash: Default::default(),
            st_1: Default::default(),
            data_sha256_hmac_hash: Default::default(),
            enc_section_2: [0; 0x114],
            enc_section_3: [0; 0x54],
            dyn_lock: Default::default(),
            cfg: Default::default()
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct ConvertedFormat {
    pub man_info_2: ntag::Manufacturer2,
    pub data_sha256_hmac_hash: [u8; 0x20],
    pub st_2: Struct2,
    pub enc_data: EncryptedData,
    pub tag_sha256_hmac_hash: [u8; 0x20],
    pub man_info_1: ntag::Manufacturer1,
    pub st_1: Struct1
}
const_assert!(core::mem::size_of::<ConvertedFormat>() == 520);

impl ConvertedFormat {
    pub const fn from_raw(raw: &RawFormat) -> Self {
        Self {
            man_info_2: raw.man_info_2,
            data_sha256_hmac_hash: raw.data_sha256_hmac_hash,
            st_2: raw.st_2,
            enc_data: EncryptedData {
                section_1: raw.enc_section_1,
                section_2: raw.enc_section_2,
                section_3: raw.enc_section_3
            },
            tag_sha256_hmac_hash: raw.tag_sha256_hmac_hash,
            man_info_1: raw.man_info_1,
            st_1: raw.st_1
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct PlainFormat {
    pub man_info_2: ntag::Manufacturer2,
    pub data_sha256_hmac_hash: [u8; 0x20],
    pub st_2: Struct2,
    pub dec_data: DecryptedData,
    pub tag_sha256_hmac_hash: [u8; 0x20],
    pub man_info_1: ntag::Manufacturer1,
    pub st_1: Struct1
}
const_assert!(core::mem::size_of::<ConvertedFormat>() == core::mem::size_of::<PlainFormat>());

impl PlainFormat {
    pub fn from_converted(conv: &ConvertedFormat) -> Self {
        // Sets everything except for encrypted sections
        Self {
            man_info_2: conv.man_info_2,
            data_sha256_hmac_hash: conv.data_sha256_hmac_hash,
            st_2: conv.st_2,
            dec_data: Default::default(),
            tag_sha256_hmac_hash: conv.tag_sha256_hmac_hash,
            man_info_1: conv.man_info_1,
            st_1: conv.st_1
        }
    }

    pub fn decrypt_from_converted(conv: &ConvertedFormat, key_set: &RetailKeySet) -> Result<Self> {
        // Convert non-encrypted parts
        let mut plain_bin = Self::from_converted(conv);

        // Decrypt the rest
        let base_seed = KeyDerivationSeed::from_converted(conv);

        let derived_key_set = DerivedKeySet::derive_from(key_set, base_seed.get_buf())?;

        let mut aes_ctr_ctx = <Aes128Ctr as KeyIvInit>::new(&derived_key_set.data_key.aes_key.into(), &derived_key_set.data_key.aes_iv.into());
        aes_ctr_ctx.apply_keystream_b2b(conv.enc_data.get_buf(), plain_bin.dec_data.get_buf_mut()).map_err(|_| ResultInvalidSize::make())?;

        if plain_bin.dec_data.settings.flags == Flags::None() {
            // Dumps with this set to none might be from retail amiibos with no usage, thus the rest of settings might be garbage data
            plain_bin.dec_data.settings = Default::default();
        }

        let mut mac = HmacSha256::new_from_slice(&key_set.tag_key.hmac_key).unwrap();
        let tag_data_start = &plain_bin.man_info_1 as *const _ as *const u8;
        let tag_data_size = core::mem::size_of::<Manufacturer1>() + core::mem::size_of::<Struct1>();
        let tag_data = unsafe {
            core::slice::from_raw_parts(tag_data_start, tag_data_size)
        };
        mac.update(tag_data);
        mac.finalize_into(&mut plain_bin.tag_sha256_hmac_hash.into());

        let data_data_start = &plain_bin.st_2.unk_1 as *const _ as *const u8;
        let data_data_size = core::mem::size_of::<PlainFormat>() - core::mem::size_of::<Manufacturer2>() - 0x20 - core::mem::size_of::<u8>();
        let data_data = unsafe {
            core::slice::from_raw_parts(data_data_start, data_data_size)
        };

        let mut mac = HmacSha256::new_from_slice(&key_set.tag_key.hmac_key).unwrap();
        mac.update(data_data);
        mac.finalize_into(&mut plain_bin.data_sha256_hmac_hash.into());

        // assert_eq!(plain_bin.tag_sha256_hmac_hash, conv.tag_sha256_hmac_hash);
        // assert_eq!(plain_bin.data_sha256_hmac_hash, conv.data_sha256_hmac_hash);

        Ok(plain_bin)
    }

    pub fn to_virtual_amiibo(&self, path: String, mii_charinfo_file: String) -> Result<fmt::VirtualAmiibo> {
        // Note: doing all the apparently redundant let-s to silence packed-field-access warnings 

        Ok(fmt::VirtualAmiibo {
            info: fmt::VirtualAmiiboInfo {
                first_write_date: {
                    let first_write_date_be = self.dec_data.settings.first_write_date_be;
                    first_write_date_be.to_virtual_amiibo_date()
                },
                id: fmt::VirtualAmiiboId {
                    game_character_id: self.st_1.amiibo_id.game_character_id,
                    character_variant: self.st_1.amiibo_id.character_variant,
                    figure_type: self.st_1.amiibo_id.figure_type,
                    model_number: self.st_1.amiibo_id.model_number.swap_bytes(),
                    series: self.st_1.amiibo_id.series
                },
                last_write_date: {
                    let last_write_date_be = self.dec_data.settings.last_write_date_be;
                    last_write_date_be.to_virtual_amiibo_date()
                },
                mii_charinfo_file,
                name: {
                    let name_be = self.dec_data.settings.name_be;
                    let name_str = name_be.get_string()?;
                    if name_str.is_empty() {
                        fsext::get_path_file_name(&path)
                    }
                    else {
                        name_str
                    }
                },
                uuid: vec! [
                    self.man_info_1.uid_p1[0],
                    self.man_info_1.uid_p1[1],
                    self.man_info_1.uid_p1[2],
                    self.man_info_1.check_byte_1,
                    self.man_info_1.uid_p2[0],
                    self.man_info_1.uid_p2[1],
                    self.man_info_1.uid_p2[2],
                    self.man_info_1.uid_p2[3],
                    self.man_info_2.check_byte_2,
                    self.man_info_2.internal
                ],
                use_random_uuid: false,
                version: 0,
                write_counter: {
                    let write_counter_be = self.dec_data.settings.write_counter_be;
                    write_counter_be.swap_bytes()
                }
            },
            mii_charinfo: miiext::generate_random_mii()?, // TODO: convert from 3DS format! meanwhile set a random mii
            areas: {
                let flags = self.dec_data.settings.flags; 
                let access_id_be = self.dec_data.settings.access_id_be;
                let program_id_be = self.dec_data.settings.program_id_be;

                match flags.contains(Flags::ApplicationAreaUsed()) {
                    true => fmt::VirtualAmiiboAreaInfo {
                        areas: vec! [
                            fmt::VirtualAmiiboAreaEntry {
                                program_id: program_id_be.swap_bytes(),
                                access_id: access_id_be.swap_bytes()
                            }
                        ],
                        current_area_access_id: access_id_be.swap_bytes()
                    },
                    false => fmt::VirtualAmiiboAreaInfo::empty()
                }
            },
            path
        })
    }
}