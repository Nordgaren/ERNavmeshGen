#pragma once

#include "hkGeometry.h"
#include "hkaiNavMesh.h"

struct hkaiNavMeshGenerationOutputs {
	/// Geometry used for generation, formed by welding nearby vertices and removing duplicate triangles.
	hkGeometry* weldedGeometry;
	/// Bitfield that indicates which of the m_weldedGeometry triangles are walkable
	void* walkableTriangles;
	/// Bitfield that indicates which of the m_weldedGeometry triangles are cutting
	void* cuttingTriangles;
	/// The generated nav mesh, before simplification is applied. This output may only be requested if simplification is enabled in the nav mesh generation settings.
	hkaiNavMesh* unsimplifiedNavMesh;
	/// The final nav mesh.
	hkaiNavMesh* navMesh;
};