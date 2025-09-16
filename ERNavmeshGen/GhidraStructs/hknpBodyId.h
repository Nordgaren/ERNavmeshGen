#pragma once
#include <cstdint>

struct hknpBodyId {
	[[nodiscard]] uint32_t getIndex() const
	{
		return serialAndIndex & 0xffffff;
	}

	[[nodiscard]] uint32_t getSerial() const
	{
		return serialAndIndex >> 24;
	}

	uint32_t serialAndIndex;
};
