#pragma once
#include <cstdint>

struct hkPropertyBag {
	void* bag; /* actual type is hkDefaultPropertyBag* */
};

struct hkReferencedObject {
	struct vtable;
	vtable* vftable;
	hkPropertyBag propertyBag;
	uint16_t memSizeAndFlags;
	uint16_t refCount;
};

/// data struct which allows for overriding of the vftable when embedding
struct hkReferencedObject_Data {
	hkPropertyBag propertyBag;
	uint16_t memSizeAndFlags;
	uint16_t refCount;
};
