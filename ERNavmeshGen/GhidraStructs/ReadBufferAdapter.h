#pragma once

namespace hkIo {
	namespace Detail {
		struct ReadBufferAdapter {
			void* impl;
			void* data;
			long long dataSize;

			ReadBufferAdapter() : impl(nullptr), data(nullptr), dataSize(0) {}
		};
	}
}