#pragma once

struct alignas(0x10) hkVector4 {
	float x;
	float y;
	float z;
	float w;

	hkVector4() {
		x = 0.0;
		y = 0.0;
		z = 0.0;
		w = 0.0;
	}

	hkVector4(float _x, float _y, float _z, float _w) {
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
};