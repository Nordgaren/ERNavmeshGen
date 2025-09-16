#include "HavokFunctions.h"
#include <Windows.h>
#include <__msvc_ostream.hpp>

#include "Signature.h"


namespace HavokFunctions {
	void init()
	{
		const wchar_t* moduleName = L"eldenring.exe";

		PATTERN_SETMODULE(moduleName)

		// Base
		PATTERN_SCAN(hkReferencedObject::addReference, "66 83 79 10 00 4c 8b c9 74 4c 41 8b 41 10 33 c9 8d 90 00 00 01 00 f0 41 0f b1 51 10 74 38 66 90 b8 01 00 00 00 d3 e0 85 c0 7e 0d 0f 1f 44 00 00")
		PATTERN_SCAN(hkReferencedObject::removeReference, "66 83 79 10 00 4c 8b c1 74 5f 41 8b 40 10 33 c9 44 8d 88 00 00 ff ff f0 45 0f b1 48 10 74 37 90 b8 01 00 00 00 d3 e0 85 c0 7e 0d 0f 1f 44 00 00")

		// Navmesh
		PATTERN_SCAN(hkaiNavMeshGenerationUtils::generateNavMesh, "4c 89 44 24 18 48 89 4c 24 08 55 41 55 48 8d ac 24 08 4f ff ff b8 f8 b1 00 00 e8 ?? ?? ?? ?? 48 2b e0")
		PATTERN_SCAN(hkaiNavMeshGenerationUtilsSettings::constructor, "48 89 5c 24 08 57 48 83 ec 20 33 ff 48 8d 05 ?? ?? ?? ?? 48 89 79 08 48 8b d9 c7 41 10 ff ff 01 00 48 89 01 c7 41 18 00 00 e0 3f c7 41 30 00 00 00 3c")
		PATTERN_SCAN(hkaiNavMeshUtils::setupQueryMediator, "48 89 5c 24 08 57 48 83 ec 20 48 8b f9 b9 74 f6 05 bb e8 ?? ?? ?? ?? b9 98 e4 bd 65 e8 ?? ?? ?? ?? b9 1a 69 1f cd")
		PATTERN_SCAN(hkaiNavMesh::constructor, "45 33 c0 48 8d 05 ?? ?? ?? ?? 4c 89 41 08 0f 57 c9 48 89 01 b8 00 80 00 00 c7 41 10 ff ff 01 00 4c 89 41 18 41 8d 50 01 44 89 41 20 c7 41 24 00 00 00 80 4c 89 41 28 44 89 41 30 c7 41 34 00 00 00 80")

		// hkIo
		PATTERN_SCAN(hkIo::Detail::createReaderImpl, "40 57 48 83 ec 20 4c 8b c1 48 8d 54 24 38 48 8b 0d ?? ?? ?? ?? 41 b9 01 00 00 00 e8 ?? ?? ?? ?? 48 8b 38 48 c7 00 00 00 00 00 48 85 ff 74 26 48 8b cf")
		PATTERN_SCAN(hkIo::Detail::createWriterImpl, "40 53 48 83 ec 20 48 8b d9 48 85 c9 74 5e 8b 0d ?? ?? ?? ?? 48 89 7c 24 30 ff 15 ?? ?? ?? ?? 48 85 c0 ba 38 00 00 00 48 0f 44 05 ?? ?? ?? ?? 48 8b 48 58")
		PATTERN_SCAN(hkOstream::constructor, "48 89 5c 24 10 57 48 83 ec 20 48 c7 41 08 00 00 00 00 48 8d 05 ?? ?? ?? ?? 48 89 01 48 8b d9 c7 41 10 ff ff 01 00 4c 8b c2 48 c7 41 18 00 00 00 00 48 8d 54 24 30")
		PATTERN_SCANREF(hkOstream::destructor, "40 53 48 83 ec 40 48 8b 91 20 02 00 00 48 8b d9 48 83 e2 fe 48 8d 4c 24 20 e8 ?? ?? ?? ?? 48 8d 54 24 50 48 8d 4c 24 20 e8 ?? ?? ?? ?? 80 38 00 74 17 4c 8b 44 24 38", 0x4f, 0x53)

		// hkSerialize
		PATTERN_SCAN(hkSerialize::Load::constructor, "40 53 48 83 ec 20 48 c7 41 08 00 00 00 00 48 8b d9 48 c7 41 10 00 00 00 00 48 83 c1 18 e8 ?? ?? ?? ?? 48 8b cb c7 43 28 00 00 00 00 e8 ?? ?? ?? ?? 48 8b c3 48 83 c4 20 5b c3")
		PATTERN_SCAN(hkSerialize::Load::toVar, "48 89 5c 24 08 48 89 6c 24 10 48 89 74 24 18 4c 89 4c 24 20 57 48 83 ec 30 48 8b f1 49 8b e8 8b 0d ?? ?? ?? ?? 48 8b da")
		PATTERN_SCAN(hkSerialize::Load::loadCompendium, "48 89 5c 24 08 48 89 6c 24 10 48 89 74 24 18 57 48 83 ec 70 48 8b e9 49 8b f0 8b 0d ?? ?? ?? ?? 48 8b fa ff 15 ?? ?? ?? ?? 48 8b d8 48 85 c0")
		PATTERN_SCAN(hkSerialize::Save::contentsVar, "48 89 5c 24 08 48 89 6c 24 10 48 89 74 24 18 57 48 83 ec 60 48 8b f1 49 8b d9 48 8d 4c 24 20 49 8b f8 48 8b ea")
		PATTERN_SCAN(hkSerialize::TagfileWriteFormat::Impl::constructor, "48 89 5c 24 08 48 89 6c 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 ec 20 48 8d 05 ?? ?? ?? ?? 48 8b d9 48 89 01 45 0f b6 f9 48 83 c1 08 41 0f b6 f0 48 8b ea")

		// Physics
		PATTERN_SCAN(hknpWorld::allocateBody, "40 55 53 56 57 41 56 48 8d ac 24 00 ff ff ff 48 81 ec 00 02 00 00 48 83 b9 20 0a 00 00 00 49 8b f0 4c 8b f2 48 8b f9 74 48")
		PATTERN_SCAN(hknpWorld::addBodies, "45 85 c0 0f 8e e5 06 00 00 48 8b c4 44 89 40 18 48 89 50 10 55 53 56 41 54 41 57 48 8d 68 a8 48 81 ec 30 01 00 00 4c 89 68 d0 45 8b f9 44 8b ad 80 00 00 00 4c 8b e2 4c 89 70 c8")
		PATTERN_SCAN(hknpWorld::getActionManager, "40 53 48 83 ec 20 48 83 b9 28 0b 00 00 00 48 8b d9 75 50 8b 0d ?? ?? ?? ?? ff 15 ?? ?? ?? ?? 48 85 c0 ba 58 00 00 00 48 0f 44 05 ?? ?? ?? ?? 48 8b 48 58 48 8b 01 ff 50 08 48 85 c0")
		PATTERN_SCAN(hknpWorld::setBodyMotionType, "48 89 5c 24 20 89 54 24 10 55 56 57 41 55 41 57 48 8d 6c 24 c9 48 81 ec 00 01 00 00 48 8b f9 c7 44 24 22 00 02 11 00 48 8b 89 10 0a 00 00 45 8b f9")
		PATTERN_SCAN(hknpWorld::setBodyActivationState, "48 89 5c 24 18 48 89 74 24 20 89 54 24 10 55 57 41 56 48 8b ec 48 83 ec 70 8b 81 28 0a 00 00 41 8b f0 8b da 4c 8b f1 83 f8 01 74 05 83 f8 40 75 30")
		PATTERN_SCAN(hknpWorld::setBodyActivationControl, "48 89 5c 24 18 89 54 24 10 56 48 83 ec 30 48 8b d9 c7 44 24 22 00 02 20 00 48 8b 89 10 0a 00 00 41 8b f0 89 54 24 28 b8 10 00 00 00 66 89 44 24 20")

		PATTERN_SCAN(hknpWorldEx::unregisterBodyAtActiveList, "f6 42 44 08 48 8b c2 74 15 8b 52 70 48 83 c1 18 39 50 74 0f 84 ?? ?? ?? ?? e9 ?? ?? ?? ?? c3")

		// AOB the entire function
		PATTERN_SCAN(hkSignal1hknpAction::fire, "48 89 6c 24 18 48 89 74 24 20 41 56 48 83 ec 20 48 83 21 fd 48 8b f1 48 83 09 01 48 8b ea 48 8b 09 4c 8b f6 48 83 e1 fc 74 5a 48 89 5c 24 30 48 89 7c 24 38 0f 1f 40 00 0f 1f 84 00 00 00 00 00 48 8b 59 08 48 8d 79 08 48 8b 01 48 83 e3 fc f6 07 03 74 15 ba 01 00 00 00 ff 10 41 8b 06 83 e0 03 48 0b c3 49 89 06 eb 09 48 8b d5 ff 50 10 4c 8b f7 48 8b cb 48 85 db 75 c6 48 8b 7c 24 38 48 8b 5c 24 30 48 83 26 fc 48 8b 74 24 48 48 8b 6c 24 40 48 83 c4 20 41 5e c3")
		const DWORD64 baseAddress = Pattern::BaseAddress(Pattern::defaultModule);
		CSHavokMan::CSHavokManImpPtr = reinterpret_cast<decltype(CSHavokMan::CSHavokManImpPtr)>(baseAddress + 0x3D76060);
		CSHavokMan::HavokTlsValueOne = reinterpret_cast<decltype(CSHavokMan::HavokTlsValueOne)>(baseAddress + 0x47e051c);
		CSHavokMan::HavokTlsValueTwo = reinterpret_cast<decltype(CSHavokMan::HavokTlsValueTwo)>(baseAddress + 0x47DACD0);
		
		PLOG_INFO << "Initialized Havok Functions";
	}


	void hknpAction::getBodies(::hknpAction* instance, hknpBodyId** bodiesOut, uint32_t* numBodiesOut)
	{
		const uintptr_t* vtable = *(uintptr_t**)instance;
		const getBodies_t getBodiesOverride = (getBodies_t)vtable[3];
		return getBodiesOverride(instance, bodiesOut, numBodiesOut);
	}

	void hknpAction::onActionAdded(::hknpAction* instance, ::hknpActionManager* actionManager)
	{
		const uintptr_t* vtable = *(uintptr_t**)instance;
		const onActionAdded_t onActionAddedOverride = (onActionAdded_t)vtable[6];
		return onActionAddedOverride(instance, actionManager);
	}

	hkResult* hknpShape::buildSurfaceGeometry(::hknpShape* instance, hkResult* result, ::hknpShape::BuildSurfaceGeometryConfig* config, hkGeometry* geometry, hkArrayGeneric* edgesOut)
	{
		const uintptr_t* vtable = *(uintptr_t**)instance;
		const buildSurfaceGeometry_t buildSurfaceGeometryOverride = (buildSurfaceGeometry_t)vtable[29];
		return buildSurfaceGeometryOverride(instance, result, config, geometry, edgesOut);
	}


	void hknpVehicleInstance::init(void* instance)
	{
		const uintptr_t* vtable = *(uintptr_t**)instance;
		const init_t initOverride = (init_t)vtable[9];
		return initOverride(instance);
	}
}