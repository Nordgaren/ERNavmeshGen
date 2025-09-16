#pragma once

#include "hkaiOverlappingTriangles.h"
#include "hkReferencedObject.h"
#include "hkaiNavMeshEdgeMatchingParameters.h"
#include "hkaiNavMeshSimplificationUtils.h"
#include "hkAabb.h"

struct hkaiNavMeshGenerationUtilsSettings {

	enum TriangleWinding {
		WINDING_CCW = 0,
		WINDING_CW = 1
	};

	enum EdgeMatchingMetric {
		ORDER_BY_OVERLAP = 1,
		ORDER_BY_DISTANCE = 2
	};

	enum ConstructionFlagsBits {
		MATERIAL_WALKABLE = 1,
		MATERIAL_CUTTING = 2,
		MATERIAL_WALKABLE_AND_CUTTING = 3
	};

	enum CharacterWidthUsage {
		NONE = 0,
		BLOCK_EDGES = 1,
		SHRINK_NAV_MESH = 2
	};

	struct RegionPruningSettings {
		float minRegionArea;
		float minDistanceToSeedPoints;
		float borderPreservationTolerance;
		bool preserveVerticalBorderRegions;
		bool pruneBeforeTriangulation;
		struct hkArrayGeneric regionSeedPoints;
		struct hkArrayGeneric regionConnections; /* actual type is hkArray<hkaiNavMeshGenerationUtilsSettings::RegionPruningSettings::RegionConnection, hkContainerHeapAllocator> */
	};

	struct WallClimbingSettings {
		bool enableWallClimbing;
		bool excludeWalkableFaces;
	};

	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	float characterHeight;
	hkVector4 up;
	float quantizationGridSize;
	float maxWalkableSlope;
	TriangleWinding triangleWinding;
	float degenerateAreaThreshold;
	float degenerateWidthThreshold;
	float convexThreshold;
	int maxNumEdgesPerFace;
	hkaiNavMeshEdgeMatchingParameters edgeMatchingParams;
	EdgeMatchingMetric edgeMatchingMetric;
	int edgeConnectionIterations;
	bool smallBoundaryEdgeGroupRemoval;
	RegionPruningSettings regionPruningSettings;
	WallClimbingSettings wallClimbingSettings;
	hkAabb boundsAabb;
	hkArrayGeneric carvers; /* hkArray<hkaiCarver, hkContainerHeapAllocator> */
	hkArrayGeneric painters; /* hkArray<hkaiMaterialPainter, hkContainerHeapAllocator> */
	void * painterOverlapCallback;
	ConstructionFlagsBits defaultConstructionProperties;
	hkArrayGeneric materialMap; /* hkArray<hkaiNavMeshGenerationUtilsSettings::MaterialConstructionPair, hkContainerHeapAllocator> */
	bool fixupOverlappingTriangles;
	hkaiOverlappingTriangles::Settings overlappingTrianglesSettings;
	bool swapOverlappingAndQuantization;
	bool weldInputVertices;
	float weldThreshold;
	float minCharacterWidth;
	CharacterWidthUsage characterWidthUsage;
	float maxCharacterWidth;
	bool precalculateClearanceSeedingData;
	bool enableSimplification;
	hkaiNavMeshSimplificationUtils::Settings simplificationSettings;
	int carvedMaterialDeprecated;
	int carvedCuttingMaterialDeprecated;
	bool checkEdgeGeometryConsistency;
	bool saveInputSnapshot;
	const char * snapshotFilename;
	hkArrayGeneric overrideSettings; /* hkArray<hkaiNavMeshGenerationUtilsSettings::OverrideSettings, hkContainerHeapAllocator> */
};