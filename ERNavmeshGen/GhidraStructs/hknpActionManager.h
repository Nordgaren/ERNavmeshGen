#pragma once
#include "hkArray.h"
#include "hkBitField.h"
#include "hkReferencedObject.h"

struct hknpActionManager {
	struct vtable;

	struct Signals {
		void* actionAdded;
		void* actionRemoved;
	};

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	void* world;
	hkArray<hknpAction*> actions;
	hkBitField bodies;
	Signals signals;
};
