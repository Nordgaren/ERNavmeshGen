#pragma once

#include "hkReferencedObject.h"
#include "hkArray.h"
#include "hknpPhysicsSytemData.h"

struct hknpPhysicsSceneData {
	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	hkArray<hknpPhysicsSystemData*> systemDatas;
	void* worldCinfo;
};
