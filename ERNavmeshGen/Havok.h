#pragma once

#include <memory>
#include <string>
#include "GhidraStructs/GhidraStructs.h"

namespace Havok {
    void init();

    std::unique_ptr<hkSerialize::Load> getLoader();

    hkResult loadCompendium(hkSerialize::Load* loader, const std::string& path);

    std::unique_ptr<hkReflect::Var> load(hkSerialize::Load* loader, const std::string& path);

    hkResult save(hkReflect::Var* var, const std::string& path);

    hknpShape* getCollisionShapeFromContainer(const hkRootLevelContainer* container);

    hkResult* getGeometryFromShape(hknpShape* shape, hkGeometry* geomOut);

    void getDefaultNavMeshGenerationSettings(hkaiNavMeshGenerationUtilsSettings& settings);

    std::unique_ptr<hkaiNavMesh> generateNavMesh(hkaiNavMeshGenerationSnapshot* snapshot);

    void* setupNavMeshQueryMediator(hkaiNavMesh* navMesh);

    void setBitInBitfield(hkBitField& hkBitField, int index);

    hkResult addAction(hknpActionManager* actionManager, hknpAction* action);

}