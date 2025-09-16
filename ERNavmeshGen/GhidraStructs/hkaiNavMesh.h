#pragma once

#include "hkReferencedObject.h"
#include "hkArray.h"
#include "BuiltInTypes.h"
#include "hkAabb.h"
#include "hkaiNavMeshClearanceCacheSeeding.h"

struct hkaiNavMesh {
	struct vtable;

	vtable* vftable;
    hkReferencedObject_Data super_hkReferencedObject;
    hkArrayGeneric faces; /* hkArray<hkaiNavMesh::Face, hkContainerHeapAllocator> */
    hkArrayGeneric edges; /* hkArray<hkaiNavMesh::Edge, hkContainerHeapAllocator> */
    hkArrayGeneric vertices; /* hkArray<hkVector4, hkContainerHeapAllocator> */
    hkArrayGeneric streamingSets; /* hkArray<hkaiAnnotatedStreamingSet, hkContainerHeapAllocator> */
    hkArrayGeneric faceData; /* hkArray<hkInt32, hkContainerHeapAllocator> */
    hkArrayGeneric edgeData; /* hkArray<hkInt32, hkContainerHeapAllocator> */
    int faceDataStriding;
    int edgeDataStriding;
    uchar flags;
    hkAabb aabb;
    float erosionRadius;
    ulong userData;
	hkaiNavMeshClearanceCacheSeeding::CacheDataSet clearanceCacheSeedingDataSet;
};

