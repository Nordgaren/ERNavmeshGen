#pragma once

#include "hkArray.h"

struct hkRootLevelContainer {
	struct NamedVariant {
		const char* name;
		const char* className;
		void* variant;
	};

	hkArray<NamedVariant> variants;
};