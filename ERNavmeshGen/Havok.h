#pragma once

#include <memory>
#include <string>
#include "GhidraStructs/GhidraStructs.h"

namespace Havok {
    
    void* implConstructHook(void* mem, void* typeCopier, bool isPackfile, bool deleteCopier);
    bool init(std::string& gamePath);

    hkSerialize::Load* getLoader();

    hkResult loadCompendium(hkSerialize::Load* loader, const std::string& path);

    hkReflect::Var* load(hkSerialize::Load* loader, const std::string& path);

    hkResult save(hkReflect::Var* var, const std::string& path);

    hknpShape* getCollisionShapeFromContainer(const hkRootLevelContainer* container);

    hkResult* getGeometryFromShape(hknpShape* shape, hkGeometry* geomOut);

    void getDefaultNavMeshGenerationSettings(hkaiNavMeshGenerationUtilsSettings& settings);

    hkaiNavMesh* generateNavMesh(hkaiNavMeshGenerationSnapshot* snapshot);

    void* setupNavMeshQueryMediator(hkaiNavMesh* navMesh);

    void setBitInBitfield(hkBitField& hkBitField, int index);

    hkResult addAction(hknpActionManager* actionManager, hknpAction* action);

}