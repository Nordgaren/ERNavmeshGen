#pragma once

#include "GhidraStructs/GhidraStructs.h"
#include <cstdint>
#include "Pattern.h"

namespace HavokFunctions {
	extern void init();

	namespace hkReferencedObject {
		FUNC_DEF(void, addReference, (::hkReferencedObject* instance))
		FUNC_DEF(void, removeReference, (::hkReferencedObject* instance))
	}

	namespace hkaiNavMeshGenerationUtils {
		FUNC_DEF(void, generateNavMesh, (::hkaiNavMeshGenerationUtilsSettings* settings, ::hkaiNavMeshGenerationSnapshot* generationSnapshot, hkaiNavMeshGenerationOutputs* outputs, void* progressCallback, void* unk))
	}

	namespace hkaiNavMeshGenerationUtilsSettings {
		FUNC_DEF(::hkaiNavMeshGenerationUtilsSettings*, constructor, (::hkaiNavMeshGenerationUtilsSettings* mem))
	};

	namespace hkaiNavMeshUtils {
		FUNC_DEF(void*, setupQueryMediator, (::hkaiNavMesh* navMesh))
	}

	namespace hkaiNavMesh {
		FUNC_DEF(::hkaiNavMesh*, constructor, (::hkaiNavMesh* mem))
	};

	namespace hknpWorld {
		FUNC_DEF(hknpBodyId*, allocateBody, (void* instance, hknpBodyId* bodyIdOut, void* bodyCinfo))
		FUNC_DEF(hknpBodyId*, addBodies, (void* instance, hknpBodyId* bodyIds, int numIds, int additionMode, int activationMode))
		FUNC_DEF(::hknpActionManager*, getActionManager, (void* instance))
		FUNC_DEF(void, setBodyMotionType, (void* instance, hknpBodyId bodyId, int motionType, int updateCachesMode))
		FUNC_DEF(bool, setBodyActivationState, (void* instance, hknpBodyId bodyId, int state))
		FUNC_DEF(void, setBodyActivationControl, (void* instance, hknpBodyId bodyId, int activationControl))
	}

	namespace hknpWorldEx {
		FUNC_DEF(void, unregisterBodyAtActiveList, (void* world, hknpBodyId* bodyId))
	}
	
	namespace CSHavokMan {
		typedef struct {
			void* vtable;
			void* hkLifoAllocator;
		} CSHavokManImp;
		VAR_DEF(CSHavokManImp*, CSHavokManImpPtr)
		VAR_DEF(DWORD, HavokTlsValueOne)
		VAR_DEF(DWORD, HavokTlsValueTwo)
	}

	namespace hkSignal1hknpAction {
		FUNC_DEF(void, fire, (void* signal, ::hknpAction* action))
	}

	namespace hknpAction {
		typedef void (__fastcall* getBodies_t)(::hknpAction* instance, hknpBodyId** bodiesOut, uint32_t* numBodiesOut);
		void getBodies(::hknpAction* instance, hknpBodyId** bodiesOut, uint32_t* numBodiesOut);


		typedef void (__fastcall* onActionAdded_t)(::hknpAction* instance, ::hknpActionManager* actionManager);
		void onActionAdded(::hknpAction* instance, ::hknpActionManager* actionManager);
	}

	namespace hknpShape {
		typedef hkResult* (__fastcall *buildSurfaceGeometry_t)(::hknpShape* instance, hkResult* result, ::hknpShape::BuildSurfaceGeometryConfig* config, hkGeometry* geometry, hkArrayGeneric* edgesOut);
		hkResult* buildSurfaceGeometry(::hknpShape* instance, hkResult* result, ::hknpShape::BuildSurfaceGeometryConfig* config, hkGeometry* geometry, hkArrayGeneric* edgesOut);
	}

	namespace hknpVehicleInstance {
		typedef void (__fastcall* init_t)(void* instance);
		void init(void* instance);
	}

	namespace hkIo::Detail {
		FUNC_DEF(void*, createReaderImpl, (const char* filename))
		FUNC_DEF(void*, createWriterImpl, (void* streamWriter))
	}

	namespace hkSerialize {
		namespace Load {
			FUNC_DEF(::hkSerialize::Load*, constructor, (::hkSerialize::Load* mem))
			FUNC_DEF(hkReflect::Var*, toVar, (::hkSerialize::Load* load, hkReflect::Var* var, ::hkIo::Detail::ReadBufferAdapter* readBufferAdapter, void* type))
			FUNC_DEF(hkResult*, loadCompendium, (::hkSerialize::Load* load, hkResult* result, ::hkIo::Detail::ReadBufferAdapter* readBufferAdapter))
		}

		namespace Save {
			FUNC_DEF(hkResult*, contentsVar, (::hkSerialize::Save* save, hkResult* result, hkReflect::Var* var, ::hkIo::Detail::WriteBufferAdapter* writeBufferAdapter))
		}

		namespace TagfileWriteFormat {
			namespace Impl {
				FUNC_DEF(void*, constructor, (void* mem, void* typeCopier, bool isPackfile, bool deleteCopier))
			}
		}
	}

	namespace hkOstream {
		FUNC_DEF(::hkOstream*, constructor, (::hkOstream* mem, const char* filename))
		FUNC_DEF(void, destructor, (::hkOstream* mem))
	}
}