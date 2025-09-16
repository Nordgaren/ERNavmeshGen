#pragma once

#include "hkReferencedObject.h"
#include "hkArray.h"

struct hknpPhysicsSystemData {
	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	hkArrayGeneric materials; //hkArray<hknpMaterial>
	hkArrayGeneric motionProperties; //hkArray<hknpMotionProperties>
	hkArrayGeneric bodyCinfos; //hkArray<hknpPhysicsSystemData::bodyCinfo>
	hkArrayGeneric constraintCinfos; //hkArray<hknpConstraintCinfo>
	const char* name;
	unsigned char microStepMultiplier;
};