# Mifare Classic 1K & Skylander Cryptography Specification

See also: [`wiki/index.md`](index.md) | [`wiki/architecture.md`](architecture.md) | [`wiki/nfc-ipc-protocol.md`](nfc-ipc-protocol.md)

---

## 1. Physical Memory Layout (Mifare Classic 1K)

Every Skylander figure uses a **Mifare Classic 1K** (ISO/IEC 14443 Type A) tag containing **1,024 bytes** organized into **16 sectors** (`0..15`), each containing **4 blocks** (`0..3`) of **16 bytes** (`64` blocks total, `0x00..0x3F`).

| Sector | Blocks | Byte Offset | Encryption | Role & Contents |
| :---: | :---: | :---: | :--- | :--- |
| **Sector 0** | Block 0 (`0x00`) | `0x000..0x00F` | Plaintext | 4-byte NUID (`0x00..0x03`), BCC (`0x04`, XOR of UID bytes), SAK (`0x05` = `0x81` or `0x08`), ATQA (`0x06..0x07` = `0x04 0x00`), Manufacturer data. |
| **Sector 0** | Block 1 (`0x01`) | `0x010..0x01F` | Plaintext | **Toy Header:** `u16` Character ID (`0x10..0x11`),Trading Card/Web Code (`0x14..0x1B`), `u16` Variant ID (`0x1C..0x1D`), **CRC-16 Type 0** (`0x1E..0x1F` over bytes `0x00..0x1D`). |
| **Sector 0** | Block 2 (`0x02`) | `0x020..0x02F` | Plaintext | All zeroes (`0x00 * 16`). |
| **Sector 0** | Block 3 (`0x03`) | `0x030..0x03F` | Plaintext | **Sector 0 Trailer:** Derived `Key A` (`6B`), Access Bits (`0x7F 0x0F 0x08 0x69`), `Key B` (`6B`, zeroed). |
| **Sectors 1–4** | Blocks `0x04..0x13` | `0x040..0x13F` | **AES-128-ECB** (except Block 3 of each sector) | **Save Area 0** (Primary Data Header at Block `0x08`, Hero Stats, Gold, XP lower bits, Hat, Nickname, Heroic Challenges, Owner ID). |
| **Sectors 5–7** | Blocks `0x14..0x1F` | `0x140..0x1FF` | **AES-128-ECB** | Giant / Swap Force / Trap Team / SuperChargers / Imaginators extended save blocks (Area 0). |
| **Sectors 8–11** | Blocks `0x20..0x2F` | `0x200..0x2FF` | **AES-128-ECB** | **Save Area 1** (Redundant double-buffered save slot; active slot selected by sequence counter at offset `+0x09` of Block `0x08` vs. Block `0x24`). |
| **Sectors 12–14** | Blocks `0x30..0x3B` | `0x300..0x3BF` | **AES-128-ECB** | Extended save blocks (Area 1). |
| **Sector 15** | Blocks `0x3C..0x3F` | `0x3C0..0x3FF` | **AES-128-ECB** | Imaginators Creation Crystal / Sensei extended data. |

---

## 2. Mifare Sector Key A Derivation (CRC-48)

Physical Skylanders protect every sector with a unique 6-byte `Key A` calculated deterministically from the tag's 4-byte UID and the sector index (`0..15`). Implemented in [`Runes::CalcKeyA()`](../EmulandersPortal/source/crypto/runes.cpp#L41-L67):

- **Sector 0 Constant Key A:** `0x4B 0x0B 0x20 0x10 0x7C 0xCB` (`73298015072107` in little-endian 48-bit representation).
- **Sectors 1–15 Key A:** Computed via a 48-bit CRC (`POLY = 0x42F0E1EBA9EA3693`, initial state `2 * 2 * 3 * 1103 * 12868356821`) over the 5-byte buffer `[uid[0], uid[1], uid[2], uid[3], sector_index]`, stored in little-endian byte order.

---

## 3. AES-128-ECB Block Encryption

Implemented in [`Crypt::ComputeAESKey()`](../EmulandersPortal/source/crypto/crypt.cpp) and [`SkylanderFigure`](../EmulandersPortal/source/crypto/skylander_crypt.cpp):

### 3.1 Per-Block Key Derivation (`MD5`)
To encrypt or decrypt a 16-byte block at index `block_idx` (`0x04..0x3F`, where `block_idx % 4 != 3`):
1. Construct an `86`-byte preimage buffer:
   - Bytes `0x00..0x1F` (`32` bytes): The first two blocks of Sector 0 (`Block 0` + `Block 1`, containing the UID, Toy ID, Variant ID, and Type 0 CRC).
   - Byte `0x20` (`1` byte): `block_idx` (`uint8_t`).
   - Bytes `0x21..0x55` (`53` bytes): The Activision copyright salt:
     ```text
     " Copyright (C) 2010 Activision. All Rights Reserved. "
     ```
2. Compute the 16-byte **MD5 digest** of this 86-byte buffer.
3. Use that 16-byte MD5 digest as the **128-bit AES-ECB key** (10 rounds) to encrypt or decrypt the 16-byte block.

### 3.2 Critical Hardware & Protocol Invariants

> [!IMPORTANT]
> **Invariant 1 — AArch64 32-Bit Integer Width (`aes.cpp` / `aes.h`):**
> On 64-bit ARM (`aarch64-none-elf` on Nintendo Switch), `unsigned long` is **8 bytes (64-bit)**, whereas on 32-bit x86/MSVC it is 4 bytes. All Rijndael lookup tables (`Te0..Te4`, `Td0..Td4`, `rcon`) and round key arrays in [`aes.cpp`](../EmulandersPortal/source/crypto/aes.cpp) and [`aes.h`](../EmulandersPortal/source/crypto/aes.h) MUST use `uint32_t`. Using `unsigned long` shifts round keys by 64 bits and produces garbage ciphertext/plaintext on Switch hardware.

> [!IMPORTANT]
> **Invariant 2 — Zero-Block Preservation (`ShouldEncryptBlock`):**
> In [`SkylanderFigure::ShouldEncryptBlock()`](../EmulandersPortal/source/crypto/skylander_crypt.cpp#L48-L62), if a data block in an uninitialized/blank `.dump` consists entirely of `0x00` bytes (`16 × 0x00`), it MUST NOT be encrypted or decrypted until non-zero data is written to it. *Skylanders Imaginators* checks whether save blocks are all-zero to detect fresh figures and initialize them; encrypting an all-zero block causes the game to reject the figure as corrupted.

---

## 4. Save Area Checksums (CRC-16-CCITT)

Each Save Area (`Area 0` starting at Block `0x08`, `Area 1` starting at Block `0x24`) is protected by four cascading **CRC-16-CCITT** (`poly = 0x1021`, `init = 0xFFFF`) checksums stored in the decrypted Area Header block (`h = Block 0x08` or `Block 0x24`). Implemented in [`SkylanderFigure::UpdateSaveCRCs()`](../EmulandersPortal/source/crypto/skylander_crypt.cpp#L105-L185):

| Checksum | Header Offset | Computed In Order | Scope Covered |
| :--- | :---: | :---: | :--- |
| **Type IV** *(Extended)* | `ext[0x00..0x01]` (`Block 0x11` or `0x2D`) | **1st** | Extended save blocks (Sectors 5–6 or 12–13) when present. |
| **Type III** | `h[0x0A..0x0B]` | **2nd** | Relative blocks `5, 6, 8` (`+0x50..+0x6F`, `+0x80..+0x8F`) padded with `14 × 0x00` blocks (`288` bytes total). |
| **Type II** | `h[0x0C..0x0D]` | **3rd** | Relative blocks `1, 2, 4` (`+0x10..+0x2F`, `+0x40..+0x4F` — `48` bytes total). |
| **Type I** | `h[0x0E..0x0F]` | **4th** | The Area Header block itself (`h[0x00..0x0D]` with `h[0x0E..0x0F]` temporarily set to `0x05, 0x00`). |

> [!CAUTION]
> Because **Type I** hashes bytes `0x00..0x0D` of the Area Header—which includes the **Type III** (`0x0A..0x0B`) and **Type II** (`0x0C..0x0D`) CRC fields—checksums MUST always be recomputed in strict descending order: **Type IV $\rightarrow$ Type III $\rightarrow$ Type II $\rightarrow$ Type I**.

---

## 5. Surgical Figure Operations (`SkylanderFigure`)

All figure modifications in Emulanders Portal operate on the decrypted Mifare image via [`SkylanderFigure`](../EmulandersPortal/source/crypto/skylander_crypt.cpp) and re-sign all CRCs before re-encrypting:

1. **Reset Figure to Level 1 (`ResetFigureToLevel1`):** Zeroes Save Areas 0 and 1 (`Sectors 1..15` data blocks), preserves Sector 0 (`UID`, `Toy ID`, `Variant ID`, `Type 0 CRC`), restores valid Sector Trailers (`0x7F 0x0F 0x08 0x69`), and leaves data blocks as raw `0x00` so the game initializes the figure cleanly on next mount.
2. **Reset Creation Crystal (`ResetCrystal`):** Zeroes the locked Battle Class and custom parts in the Save Areas and Sector 15 so *Skylanders Imaginators* prompts the player to select a new Battle Class.
3. **Clear Owner ID (`ClearOwner`):** Zeroes the 9-byte console ownership record at relative block `+0x20` (`offset +0x06..+0x0E`) in both Save Areas and updates `Type II` and `Type I` CRCs.
4. **Clone with New UID (`CloneWithNewUID`):** Generates 4 fresh pseudo-random UID bytes (`uid[0..3]`), updates `BCC` (`uid[0] ^ uid[1] ^ uid[2] ^ uid[3]`), recomputes **Sector 0 Type 0 CRC** (`offset 0x1E..0x1F`), derives new `Key A` trailers for Sectors `1..15`, and re-encrypts all non-zero data blocks under the new Sector 0 MD5 key schedule.
