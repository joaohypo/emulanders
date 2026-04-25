#include "PortalAlgos.hpp"
#include "3rd_party/md5.h"
#include "3rd_party/rijndael.h"

#include <memory>
#include <iostream>
#include <cstring>

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

	MD5* md5 = new MD5();
	MD5Open(md5);
	MD5Digest(md5, keyComponents, 0x56);
	MD5Close(md5, key);
	delete md5;
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
