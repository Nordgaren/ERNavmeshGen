#pragma once

namespace hkReflect {
	struct Var {
		void* addr;
		void* type;
		void* impl;
	};
}