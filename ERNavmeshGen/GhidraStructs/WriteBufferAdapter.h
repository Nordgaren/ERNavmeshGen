#pragma once

#include "WriteBufferImpl.h"

namespace hkIo {
	namespace Detail {
		struct WriteBufferAdapter {
			WriteBufferImpl* impl;
			void* data;
			long long dataSize;
			long long* dataUsedSizeOut;
		};
	}
}