#pragma once

#include "hkaiNavMeshClearanceCache.h"
#include "hkReferencedObject.h"
#include "hkArray.h"

namespace hkaiNavMeshClearanceCacheSeeding {
	struct CacheDataSet {
		struct vtable;
		vtable* vftable;

		hkReferencedObject_Data super_hkReferencedObject;
		hkArrayGeneric cacheDatas;
	};

	struct CacheData {
		unsigned long long id;
		unsigned int info;
		unsigned int infoMask;
		hkaiNavMeshClearanceCache* initialCache;
	};
}