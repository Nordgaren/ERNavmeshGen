#pragma once

#include "hkArray.h"

namespace hkaiNavMeshSimplificationUtils {
	struct ExtraVertexSettings {
		enum VertexSelectionMethod {
			PROPORTIONAL_TO_AREA = 0,
			PROPORTIONAL_TO_VERTICES = 1
		};

		VertexSelectionMethod vertexSelectionMethod;
		float vertexFraction;
		float areaFraction;
		float minPartitionArea;
		int numSmoothingIterations;
		float iterationDamping;
		bool addVerticesOnBoundaryEdges;
		bool addVerticesOnPartitionBorders;
		float boundaryEdgeSplitLength;
		float partitionBordersSplitLength;
		float userVertexOnBoundaryTolerance;
		hkArrayGeneric userVertices;
	};

	struct Settings {
		float maxBorderSimplifyArea;
		float maxConcaveBorderSimplifyArea;
		float minCorridorWidth;
		float maxCorridorWidth;
		float holeReplacementArea;
		float aabbReplacementAreaFraction;
		float maxLoopShrinkFraction;
		float maxBorderHeightError;
		float maxBorderDistanceError;
		int maxPartitionSize;
		bool useHeightPartitioning;
		float maxPartitionHeightError;
		bool useConservativeHeightPartitioning;
		float hertelMehlhornHeightError;
		float cosPlanarityThreshold;
		float nonconvexityThreshold;
		float boundaryEdgeFilterThreshold;
		float maxSharedVertexHorizontalError;
		float maxSharedVertexVerticalError;
		float maxBoundaryVertexHorizontalError;
		float maxBoundaryVertexVerticalError;
		bool mergeLongestEdgesFirst;
		ExtraVertexSettings extraVertexSettings;
		bool saveInputSnapshot;
		char * snapshotFilename;
	};
}




