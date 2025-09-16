#pragma once

#include "hkReferencedObject.h"
#include "hkArray.h"
#include "hkVector4.h"

struct hknpShape {
	struct BuildSurfaceGeometryConfig {
		enum ConvexRadiusDisplayMode {
			CONVEX_RADIUS_DISPLAY_NONE = 0,
			CONVEX_RADIUS_DISPLAY_PLANAR,
			CONVEX_RADIUS_DISPLAY_ROUNDED
		};

		enum ScaleMode : int {
			SCALE_SURFACE = 0,
			SCALE_VERTICES
		};

		ConvexRadiusDisplayMode radiusMode;
		bool buildEdges;
		bool storeShapeKeyInTriangleMaterial;
		void* shapeKeyMask;
		int levelOfDetail;
		hkVector4 scale;
		ScaleMode scaleMode;
		hkVector4 offset;
		float convexRadiusOverride;

		BuildSurfaceGeometryConfig() {
			radiusMode = CONVEX_RADIUS_DISPLAY_ROUNDED;
			storeShapeKeyInTriangleMaterial = false;
			shapeKeyMask = nullptr;
			levelOfDetail = 2;
			scale = hkVector4(1.0f, 1.0f, 1.0f, 1.0f);
			scaleMode = SCALE_SURFACE;
			offset = hkVector4();
			buildEdges = false;
			convexRadiusOverride = -1.0f;
		}
	};


	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;

	unsigned short flags;
	unsigned char type;
	unsigned char numShapeKeyBits;
	int dispatchType;
	float convexRadius;
	unsigned long long userData;
	void* properties;
};