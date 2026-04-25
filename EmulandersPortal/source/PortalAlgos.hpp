#pragma once

#include <cstdint>

namespace Runes
{
	void computeKey(uint8_t* header, uint8_t blockId, uint8_t key[0x10]);
	void decryptBlock(uint8_t* header, uint8_t* cBlockData, uint8_t* pBlockData, uint8_t blockId);
	void encryptBlock(uint8_t* header, uint8_t* pBlockData, uint8_t* cBlockData, uint8_t blockId);
    
    void CalcKeyA(const uint8_t* uid, uint8_t sector, uint8_t* out_key);
}