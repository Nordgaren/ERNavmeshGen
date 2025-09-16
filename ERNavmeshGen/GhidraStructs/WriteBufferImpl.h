#pragma once

#include "hkReferencedObject.h"

namespace hkIo {
	namespace Detail {
		struct WriteBufferImpl {
			struct vtable;

			vtable* vftable;
			hkReferencedObject_Data super_hkReferencedObject;
		};
	}
}
