#pragma once
#include <cstdint>

#include "hkReferencedObject.h"

struct hknpAction {
	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	uint64_t userData;
};
