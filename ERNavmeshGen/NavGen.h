#pragma once

#include <memory>

#include "Havok.h"
#include "GhidraStructs/Load.h"
#include "Util/hkArrayUtil.h"
#include <plog/Log.h>
#include <filesystem>


bool GenerateNavMeshFromCollision(const std::string& pathIn, const std::string& compendiumPathIn)
{
	std::unique_ptr<hkSerialize::Load> load = Havok::getLoader();
	if (!compendiumPathIn.empty() && Havok::loadCompendium(load.get(), compendiumPathIn) == HK_SUCCESS)
	{
		PLOG_VERBOSE << "Loaded compendium from " << compendiumPathIn;
	}

	std::unique_ptr<hkReflect::Var> var = Havok::load(load.get(), pathIn);
	if (!var) return false;
	PLOG_VERBOSE << "Loaded collision file from " << pathIn;


	hkaiNavMeshGenerationSnapshot snapshot { };
	hknpShape* shape = Havok::getCollisionShapeFromContainer((hkRootLevelContainer*)var->addr);
	if (hkResult geomBuildResult = *Havok::getGeometryFromShape(shape, &snapshot.geometry))
	{
		PLOG_ERROR << "Failed to get geometry. Error code: " << geomBuildResult;
		return false;
	}
	PLOG_VERBOSE << "Geometry built successfully. (vertices: " << snapshot.geometry.vertices.size << " triangles: " << snapshot.geometry.triangles.size << ")";

	Havok::getDefaultNavMeshGenerationSettings(snapshot.settings);
	snapshot.settings.up = hkVector4(0, 1, 0, 0);
	snapshot.settings.precalculateClearanceSeedingData = true;

	std::filesystem::path snapshotPath { pathIn };
	const std::string snapshotFilename = "s" + snapshotPath.filename().string().substr(1);
	snapshotPath.replace_filename(snapshotFilename);
	snapshotPath.replace_extension("hkt");
	snapshot.settings.snapshotFilename = snapshotPath.string().c_str();
	snapshot.settings.saveInputSnapshot = true;

	std::unique_ptr<hkaiNavMesh> navMesh = Havok::generateNavMesh(&snapshot);
	if (!navMesh) return false;
	PLOG_VERBOSE << "NavMesh generated successfully. (vertices: " << navMesh->vertices.size << " faces: " << navMesh->faces.size << ")";

	void* queryMediator = Havok::setupNavMeshQueryMediator(navMesh.get());
	if (!queryMediator) return false;
	PLOG_VERBOSE << "QueryMediator set up successfully.";


	hkRootLevelContainer container = { };

	std::vector<hkRootLevelContainer::NamedVariant> v {
		{
			.name = "hkaiNavMesh",
			.className = "hkaiNavMesh",
			.variant = navMesh.get(),
		},
		{
			.name = "hkaiStaticTreeNavMeshQueryMediator",
			.className = "hkaiStaticTreeNavMeshQueryMediator",
			.variant = queryMediator,
		}
	};
	hkArrayManager::CreateManaged(&container.variants, v);

	var->addr = &container;
	
	std::filesystem::path pathOut { pathIn };
	const std::string inFilename = "q" + pathOut.filename().string().substr(1);
	pathOut.replace_filename(inFilename);
	PLOG_INFO << "Saving navmesh file to " << pathOut;
	const hkResult result = Havok::save(var.get(), pathOut.string());

	if (result != HK_SUCCESS)
	{
		PLOG_ERROR << "Write failed with error code " << result;
		return false;
	}

	PLOG_INFO << "Saved navmesh file to " << pathOut;
	return true;
}

