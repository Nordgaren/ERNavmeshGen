#pragma once

struct hknpVehicleDriverInputAnalogStatus {
	struct vtable;

	vtable* vftable;
	hkReferencedObject_Data super_hkReferencedObject;
	float positionX;
	float positionY;
	bool handbrakeButtonPressed;
	bool reverseButtonPressed;
};
