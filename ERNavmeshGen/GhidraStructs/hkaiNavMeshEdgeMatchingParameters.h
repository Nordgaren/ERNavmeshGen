#pragma once

struct hkaiNavMeshEdgeMatchingParameters {
	float maxStepHeight;
	float maxSeparation;
	float maxOverhang;
	float behindFaceTolerance;
	float cosPlanarAlignmentAngle;
	float cosVerticalAlignmentAngle;
	float minEdgeOverlap;
	float edgeTraversibilityHorizontalEpsilon;
	float edgeTraversibilityVerticalEpsilon;
	float cosClimbingFaceNormalAlignmentAngle;
	float cosClimbingEdgeAlignmentAngle;
	float minAngleBetweenFaces;
	float edgeParallelTolerance;
	bool useSafeEdgeTraversibilityHorizontalEpsilon;
};