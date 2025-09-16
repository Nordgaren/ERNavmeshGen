#pragma once

enum hkArrayFlags : unsigned int {
	DONT_DEALLOCATE = 0x80000000
};

template <typename T>
struct hkArray {
	T* data;
	unsigned int size;
	unsigned int capacityAndFlags;

	hkArray() : data(nullptr), size(0), capacityAndFlags(DONT_DEALLOCATE) { }
};

struct hkArrayGeneric : hkArray<void> {};
