#pragma once
#include <cstdint>

#include "hkArray.h"
#include "hkBitFieldStorage.h"

struct hkBitField {
	hkBitFieldStorage<hkArray<uint32_t>> storage;
};
