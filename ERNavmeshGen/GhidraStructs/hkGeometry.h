#pragma once

#include "hkReferencedObject.h"
#include "hkArray.h"

struct hkGeometry {
	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	hkArrayGeneric vertices;
	hkArrayGeneric triangles;
};