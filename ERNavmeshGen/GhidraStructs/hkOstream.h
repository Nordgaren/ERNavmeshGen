#pragma once

#include "hkReferencedObject.h"

struct hkOstream {
	struct vtable;
	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	void* writer;
};