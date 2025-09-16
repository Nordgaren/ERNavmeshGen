#pragma once

namespace hkSerialize {
	struct Load {
		void* typeReg;
		void* format;
		void* patcher;
		void* cloner;
		void* cloneCallback;
		int versionOptions;
		void* noteHandler;
	};
}