#include <Windows.h>
#include "Havok.h"
#include "GhidraStructs/GhidraStructs.h"
#include "HavokFunctions.h"
#include "Util/hkArrayUtil.h"
#include "Util/HookUtil.h"

void* implConstructHook(void* mem, void* typeCopier, bool isPackfile, bool deleteCopier)
{
	return HavokFunctions::hkSerialize::TagfileWriteFormat::Impl::constructor(mem, nullptr, true, false);
}

void Havok::init()
{
	HavokFunctions::init();
}

std::unique_ptr<hkSerialize::Load> Havok::getLoader()
{
	std::unique_ptr<hkSerialize::Load> load = std::make_unique<hkSerialize::Load>();
	HavokFunctions::hkSerialize::Load::constructor(load.get());
	return load;
}

hkResult Havok::loadCompendium(hkSerialize::Load* loader, const std::string& path)
{
	// return HK_FAILURE;
	hkIo::Detail::ReadBufferAdapter readBufferAdapter { };
	readBufferAdapter.impl = HavokFunctions::hkIo::Detail::createReaderImpl(path.c_str());

	hkResult result;
	HavokFunctions::hkSerialize::Load::loadCompendium(loader, &result, &readBufferAdapter);
	return result;
}

std::unique_ptr<hkReflect::Var> Havok::load(hkSerialize::Load* loader, const std::string& path)
{
	hkIo::Detail::ReadBufferAdapter readBufferAdapter { };
	readBufferAdapter.impl = HavokFunctions::hkIo::Detail::createReaderImpl(path.c_str());

	std::unique_ptr<hkReflect::Var> var = std::make_unique<hkReflect::Var>();
	HavokFunctions::hkSerialize::Load::toVar(loader, var.get(), &readBufferAdapter, nullptr);
	return var;
}

hkResult Havok::save(hkReflect::Var* var, const std::string& path)
{
	hkSerialize::Save save { };
	hkOstream stream { };
	HavokFunctions::hkOstream::constructor(&stream, path.c_str());

	hkIo::Detail::WriteBufferAdapter writeBufferAdapter { };
	writeBufferAdapter.impl = (hkIo::Detail::WriteBufferImpl*)HavokFunctions::hkIo::Detail::createWriterImpl(
		stream.writer);

	HavokFunctions::hkOstream::destructor(&stream);

	hkResult result;

	HOOK_SCOPED(HavokFunctions::hkSerialize::TagfileWriteFormat::Impl::constructor, implConstructHook);
	HavokFunctions::hkSerialize::Save::contentsVar(&save, &result, var, &writeBufferAdapter);
	return result;
}

hknpShape* Havok::getCollisionShapeFromContainer(const hkRootLevelContainer* container)
{
	const hkRootLevelContainer::NamedVariant* variant = container->variants.data;

	const hknpPhysicsSceneData* physicsSceneData = (hknpPhysicsSceneData*)variant->variant;
	const hknpPhysicsSystemData* physicsSystemData = *physicsSceneData->systemDatas.data;

	hknpShape* shape = *(hknpShape**)physicsSystemData->bodyCinfos.data;
	return shape;
}

hkResult* Havok::getGeometryFromShape(hknpShape* shape, hkGeometry* geomOut)
{
	hkResult buildGeomResult;
	hknpShape::BuildSurfaceGeometryConfig config;
	return HavokFunctions::hknpShape::buildSurfaceGeometry(shape, &buildGeomResult, &config, geomOut, nullptr);
}

std::unique_ptr<hkaiNavMesh> Havok::generateNavMesh(hkaiNavMeshGenerationSnapshot* snapshot)
{
	std::unique_ptr<hkaiNavMesh> navMesh = std::make_unique<hkaiNavMesh>();
	HavokFunctions::hkaiNavMesh::constructor(navMesh.get());

	hkaiNavMeshGenerationOutputs outputs { };

	outputs.navMesh = navMesh.get();
	HavokFunctions::hkaiNavMeshGenerationUtils::generateNavMesh(&snapshot->settings, snapshot, &outputs, nullptr, nullptr);
	return navMesh;
}

void Havok::getDefaultNavMeshGenerationSettings(hkaiNavMeshGenerationUtilsSettings& settings)
{
	HavokFunctions::hkaiNavMeshGenerationUtilsSettings::constructor(&settings);
}

void* Havok::setupNavMeshQueryMediator(hkaiNavMesh* navMesh)
{
	return HavokFunctions::hkaiNavMeshUtils::setupQueryMediator(navMesh);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
// the contents are not const
void Havok::setBitInBitfield(hkBitField& hkBitField, int index)
{
	const int wordIndex = index >> 5;
	const int bitIndex = index & 31;
	unsigned* data = hkBitField.storage.words.data;
	data[wordIndex] |= 1 << bitIndex;
}

hkResult Havok::addAction(hknpActionManager* actionManager, hknpAction* action)
{
	hknpBodyId* bodyIds = nullptr;
	uint32_t numBodies = 0;
	HavokFunctions::hknpAction::getBodies(action, &bodyIds, &numBodies);
	if (actionManager->bodies.storage.numBits / 32 < bodyIds[numBodies].getIndex())
	{
		PLOG_ERROR << "Body bitfield too small, expansion not implemented";
		return HK_E_NOT_IMPLEMENTED;
	}

	for (int i = 0; i < numBodies; ++i)
	{
		setBitInBitfield(actionManager->bodies, bodyIds[i].getIndex());
	}

	hkArrayManager::PushBack(&actionManager->actions, action);
	HavokFunctions::hknpAction::onActionAdded(action, actionManager);
	HavokFunctions::hkSignal1hknpAction::fire(&actionManager->signals.actionAdded, action);
	return HK_SUCCESS;
}
