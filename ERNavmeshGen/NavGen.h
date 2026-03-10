#pragma once

#include <memory>

#include "Havok.h"
#include "GhidraStructs/Load.h"
#include "Util/hkArrayUtil.h"
#include <plog/Log.h>
#include <filesystem>
#include "globals.h"


// std::vector<std::unique_ptr<hkaiNavMesh>> keepAliveNavMesh;
// std::vector<std::unique_ptr<hkSerialize::Load>> keepAliveLoad;
// std::vector<std::unique_ptr<hkReflect::Var>> keepAliveVar;

bool GenerateNavMeshFromCollision(const std::string& pathIn, const std::string& pathOut, const std::string& compendiumPathIn)
{
	hkSerialize::Load* load = Havok::getLoader();
	if (!compendiumPathIn.empty() && Havok::loadCompendium(load, compendiumPathIn) == HK_SUCCESS)
	{
		PLOG_VERBOSE << "Loaded compendium from " << compendiumPathIn;
	}

	hkReflect::Var* var = Havok::load(load, pathIn);
	if (!var) return false;
	if (var->addr == nullptr)
	{
		// Anything that is hkx that can't be loaded.
		return true;
	}
	
	PLOG_VERBOSE << "Loaded collision file from " << pathIn;


	hkaiNavMeshGenerationSnapshot snapshot = g_snapshot;
	hknpShape* shape = Havok::getCollisionShapeFromContainer((hkRootLevelContainer*)var->addr);
	if (hkResult geomBuildResult = *Havok::getGeometryFromShape(shape, &snapshot.geometry))
	{
		PLOG_ERROR << "Failed to get geometry. Error code: " << geomBuildResult;
		return false;
	}
	PLOG_VERBOSE << "Geometry built successfully. (vertices: " << snapshot.geometry.vertices.size << " triangles: " << snapshot.geometry.triangles.size << ")";

	hkaiNavMesh* navMesh = Havok::generateNavMesh(&snapshot);
	if (!navMesh) return false;
	PLOG_VERBOSE << "NavMesh generated successfully. (vertices: " << navMesh->vertices.size << " faces: " << navMesh->faces.size << ")";

	void* queryMediator = Havok::setupNavMeshQueryMediator(navMesh);
	if (!queryMediator) return false;
	PLOG_VERBOSE << "QueryMediator set up successfully.";


	hkRootLevelContainer container = { };

	std::vector<hkRootLevelContainer::NamedVariant> v {
		{
			.name = "hkaiNavMesh",
			.className = "hkaiNavMesh",
			.variant = navMesh,
		},
		{
			.name = "hkaiStaticTreeNavMeshQueryMediator",
			.className = "hkaiStaticTreeNavMeshQueryMediator",
			.variant = queryMediator,
		}
	};
	hkArrayManager::CreateManaged(&container.variants, v);

	var->addr = &container;
	

	PLOG_INFO << "Saving navmesh file to " << pathOut;
	const hkResult result = Havok::save(var, pathOut);

	if (result != HK_SUCCESS)
	{
		PLOG_ERROR << "Write failed with error code " << std::hex << result;
		return false;
	}

	PLOG_INFO << "Saved navmesh file to " << pathOut;
	return true;
}

