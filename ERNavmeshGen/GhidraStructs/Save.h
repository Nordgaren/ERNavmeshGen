#pragma once

namespace hkSerialize {
	struct Save {
		void* format;
		void* varCallback;
		void* ptrCallback;
		void* callbackData;

		Save() : format(nullptr), varCallback(nullptr), ptrCallback(nullptr), callbackData(nullptr) {}
	};
}