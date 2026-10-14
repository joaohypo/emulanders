#include "PortalAlgos.hpp"
#include "3rd_party/md5.h"
#include "3rd_party/rijndael.h"

#include <cstring>
#include <cstdint>
#include <fstream>
#include <random>
#include <chrono>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

// ASCII representation of the string (spaces include)
// " Copyright (C) 2010 Activision. All Rights Reserved. "
const uint8_t kSalt[0x35] = {
	0x20, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67,
	0x68, 0x74, 0x20, 0x28, 0x43, 0x29, 0x20, 0x32,
	0x30, 0x31, 0x30, 0x20, 0x41, 0x63, 0x74, 0x69,
	0x76, 0x69, 0x73, 0x69, 0x6F, 0x6E, 0x2E, 0x20,
	0x41, 0x6C, 0x6C, 0x20, 0x52, 0x69, 0x67, 0x68,
	0x74, 0x73, 0x20, 0x52, 0x65, 0x73, 0x65, 0x72,
	0x76, 0x65, 0x64, 0x2E, 0x20
};

void Runes::computeKey(uint8_t* header, uint8_t blockId, uint8_t key[0x10])
{
	uint8_t keyComponents[0x56];
	memcpy(keyComponents, header, 0x20);
	keyComponents[0x20] = blockId;

	memcpy(keyComponents + 0x21, kSalt, 0x35);

	MD5 md5;
	MD5Open(&md5);
	MD5Digest(&md5, keyComponents, 0x56);
	MD5Close(&md5, key);
}

void Runes::decryptBlock(uint8_t* header, uint8_t* cBlockData, uint8_t* pBlockData, uint8_t blockId)
{
	uint8_t key[0x10];
	Runes::computeKey(header, blockId, key);

	unsigned long rk[RKLENGTH(128)];
	uint32_t rounds = rijndaelSetupDecrypt(rk, key, 128);
	rijndaelDecrypt(rk, rounds, cBlockData, pBlockData);
}

void Runes::encryptBlock(uint8_t* header, uint8_t* pBlockData, uint8_t* cBlockData, uint8_t blockId)
{
	uint8_t key[0x10];
	Runes::computeKey(header, blockId, key);

	unsigned long rk[RKLENGTH(128)];
	uint32_t rounds = rijndaelSetupEncrypt(rk, key, 128);
	rijndaelEncrypt(rk, rounds, pBlockData, cBlockData);
}

void Runes::CalcKeyA(const uint8_t* uid, uint8_t sector, uint8_t* out_key) {
    if (sector == 0) {
        out_key[0] = 0x4B; out_key[1] = 0x0B; out_key[2] = 0x20;
        out_key[3] = 0x10; out_key[4] = 0x7C; out_key[5] = 0xCB;
        return;
    }

    uint64_t crc = 0x9AE903260CC4ULL;
    uint64_t poly = 0x42F0E1EBA9EA3693ULL;

    uint8_t data[5];
    data[0] = uid[0]; data[1] = uid[1]; data[2] = uid[2]; data[3] = uid[3];
    data[4] = sector;

    for (int i = 0; i < 5; i++) {
        crc ^= ((uint64_t)data[i] << 40);
        for (int k = 0; k < 8; k++) {
            if (crc & 0x800000000000ULL) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
            crc &= 0xFFFFFFFFFFFFULL;
        }
    }

    out_key[0] = crc & 0xFF;
    out_key[1] = (crc >> 8) & 0xFF;
    out_key[2] = (crc >> 16) & 0xFF;
    out_key[3] = (crc >> 24) & 0xFF;
    out_key[4] = (crc >> 32) & 0xFF;
    out_key[5] = (crc >> 40) & 0xFF;
}

uint16_t Runes::crc16(const uint8_t* pData, size_t length)
{
	uint16_t wCrc = 0xffff;
	while (length--) {
		wCrc ^= (uint16_t)(*pData++) << 8;
		for (int i = 0; i < 8; i++) {
			wCrc = (wCrc & 0x8000) ? ((wCrc << 1) ^ 0x1021) : (wCrc << 1);
		}
	}
	return wCrc & 0xffff;
}

uint8_t Runes::calculateBCC(uint32_t serial)
{
	return ((serial >> 24) & 0xff) ^ ((serial >> 16) & 0xff) ^ ((serial >> 8) & 0xff) ^ (serial & 0xff);
}

bool Runes::CheckIsImaginatorOrCrystal(const std::string& dump_path)
{
    std::ifstream in(dump_path, std::ios::binary);
    if (!in.is_open()) return false;

    uint8_t hdr[0x20] = {0};
    in.read(reinterpret_cast<char*>(hdr), sizeof(hdr));
    if (in.gcount() < 0x20) return false;

    uint32_t toyType = hdr[0x10] | (hdr[0x11] << 8) | (hdr[0x12] << 16);
    uint16_t subType = hdr[0x1C] | (hdr[0x1D] << 8);

    // Year code from subType (bit 12..15)
    uint8_t yearCode = (subType >> 12) & 0x0F;

    // Year 5 (Skylanders 2016 / Imaginators) or Year 6 (2017)
    if (yearCode >= 5) return true;

    // Creation Crystals range: 0x000E00 to 0x000E1F
    if (toyType >= 0x000E00 && toyType <= 0x000E1F) return true;

    return false;
}

static void CopyAdjacentArtwork(const std::string& src_path, const std::string& dest_path)
{
    size_t src_dot = src_path.find_last_of('.');
    size_t dest_dot = dest_path.find_last_of('.');
    if (src_dot != std::string::npos && dest_dot != std::string::npos) {
        std::string src_img = src_path.substr(0, src_dot) + ".png";
        std::string dest_img = dest_path.substr(0, dest_dot) + ".png";
        std::error_code ec;
        if (fs::exists(src_img, ec)) {
            fs::copy_file(src_img, dest_img, fs::copy_options::overwrite_existing, ec);
            chmod(dest_img.c_str(), 0777);
        }
    }
}

bool Runes::CloneDump1to1(const std::string& src_path, const std::string& dest_path)
{
    std::ifstream in(src_path, std::ios::binary);
    if (!in.is_open()) return false;

    uint8_t buffer[1024];
    in.read(reinterpret_cast<char*>(buffer), 1024);
    if (in.gcount() != 1024) return false;
    in.close();

    std::ofstream out(dest_path, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<char*>(buffer), 1024);
    out.close();

    chmod(dest_path.c_str(), 0777);
    CopyAdjacentArtwork(src_path, dest_path);
    return true;
}

static inline bool IsBlockAllZero(const uint8_t* block) {
    uint64_t w1, w2;
    memcpy(&w1, block, 8);
    memcpy(&w2, block + 8, 8);
    return (w1 | w2) == 0;
}

bool Runes::CloneDumpNewIdentity(const std::string& src_path, const std::string& dest_path, uint32_t forced_new_serial)
{
    std::ifstream in(src_path, std::ios::binary);
    if (!in.is_open()) return false;

    uint8_t dump[1024];
    in.read(reinterpret_cast<char*>(dump), 1024);
    if (in.gcount() != 1024) return false;
    in.close();

    // 1. Capture original header (0x20 bytes)
    uint8_t old_header[0x20];
    memcpy(old_header, dump, 0x20);

    // 2. Decrypt all populated data blocks (blocks 8..63, excluding sector trailers)
    // NOTE: In Skylanders NFC tags, unwritten/inactive blocks are left as ALL ZEROS (0x00).
    // Decrypting 0x00 converts empty space into pseudorandom garbage ciphertext, which
    // corrupts area sequence detection and triggers the in-game data recovery prompt.
    uint8_t plain_blocks[64][16];
    bool block_is_populated[64] = {false};

    for (uint8_t b = 8; b < 64; b++) {
        if ((b % 4) == 3) continue; // Skip access control / sector trailer blocks
        if (!IsBlockAllZero(dump + b * 16)) {
            block_is_populated[b] = true;
            decryptBlock(old_header, dump + b * 16, plain_blocks[b], b);
        }
    }

    // 3. Generate new unique serial / UID
    uint32_t new_serial = forced_new_serial;
    if (new_serial == 0) {
        uint32_t seed = (uint32_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::mt19937 rng(seed);
        new_serial = rng();
        // Prevent identical serial to old
        uint32_t old_serial = *(reinterpret_cast<uint32_t*>(old_header));
        if (new_serial == old_serial || new_serial == 0) new_serial ^= 0xA5C3F019;
    }

    // 4. Update Block 0 (Serial + BCC) and Block 1 Checksum
    memcpy(dump, &new_serial, sizeof(uint32_t));
    dump[4] = calculateBCC(new_serial);

    // Recalculate Header Checksum (CRC16 over first 0x1E bytes)
    uint16_t new_hdr_crc = crc16(dump, 0x1E);
    memcpy(dump + 0x1E, &new_hdr_crc, sizeof(uint16_t));

    // 5. Re-encrypt all populated data blocks with the new header
    uint8_t new_header[0x20];
    memcpy(new_header, dump, 0x20);

    for (uint8_t b = 8; b < 64; b++) {
        if ((b % 4) == 3) continue;
        if (block_is_populated[b]) {
            encryptBlock(new_header, plain_blocks[b], dump + b * 16, b);
        } else {
            memset(dump + b * 16, 0, 16);
        }
    }

    // 6. Update Sector Trailers (Key A) for all 16 sectors
    uint8_t new_uid[4];
    memcpy(new_uid, dump, 4);

    for (uint8_t s = 0; s < 16; s++) {
        uint8_t keyA[6];
        CalcKeyA(new_uid, s, keyA);
        uint8_t* trailer = dump + (s * 4 + 3) * 16;
        memcpy(trailer, keyA, 6);
    }

    // 7. Write out cloned dump
    std::ofstream out(dest_path, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<char*>(dump), 1024);
    out.close();

    chmod(dest_path.c_str(), 0777);
    CopyAdjacentArtwork(src_path, dest_path);
    return true;
}
