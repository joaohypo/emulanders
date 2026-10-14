#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace Runes
{
	void computeKey(uint8_t* header, uint8_t blockId, uint8_t key[0x10]);
	void decryptBlock(uint8_t* header, uint8_t* cBlockData, uint8_t* pBlockData, uint8_t blockId);
	void encryptBlock(uint8_t* header, uint8_t* pBlockData, uint8_t* cBlockData, uint8_t blockId);
    
    void CalcKeyA(const uint8_t* uid, uint8_t sector, uint8_t* out_key);

    uint16_t crc16(const uint8_t* pData, size_t length);
    uint8_t calculateBCC(uint32_t serial);

    bool CheckIsImaginatorOrCrystal(const std::string& dump_path);
    bool CloneDump1to1(const std::string& src_path, const std::string& dest_path);
    bool CloneDumpNewIdentity(const std::string& src_path, const std::string& dest_path, uint32_t forced_new_serial = 0);
}