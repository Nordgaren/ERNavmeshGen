#pragma once

namespace hkaiOverlappingTriangles {

	enum WalkableTriangleSettings {
		ONLY_FIX_WALKABLE = 0,
		PREFER_WALKABLE = 1,
		PREFER_UNWALKABLE = 2
	};

	struct Settings {
		float coplanarityTolerance;
		float raycastLengthMultiplier;
		WalkableTriangleSettings walkableTriangleSettings;
	};
}