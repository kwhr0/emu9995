#include "TMS9919.h"

TMS9919::TMS9919() : rng(1 << 16), white(0), nf(0), osc(0), tmp(0) {
	for (int i = 0; i < 4; i++) {
		freq[i] = cnt[i] = v[i] = 0;
		att[i] = 0.f;
	}
}

void TMS9919::Mute() {
	for (int i = 0; i < 4; i++) att[i] = 0.f;
}

void TMS9919::Set(int data) {
	static const float tbl[] = {
		0.250000f, 0.198582f, 0.157739f, 0.125297f, 0.099527f, 0.079057f, 0.062797f, 0.049882f,
		0.039622f, 0.031473f, 0.025000f, 0.019858f, 0.015774f, 0.012530f, 0.009953f, 0.000000f
	};
	if (data & 0x80) {
		int regnum = data >> 4 & 7;
		osc = regnum >> 1;
		switch (regnum) {
			case 0: case 2: case 4:
				tmp = data & 0xf;
				break;
			case 1: case 3: case 5: case 7:
				att[osc] = tbl[data & 0xf];
				break;
			case 6:
				nf = (data & 3) == 3;
				freq[3] = nf ? freq[2] << 1 : 0x20 << (data & 3);
				white = (data & 4) != 0;
				break;
		}
	}
	else if (osc != 3) {
		freq[osc] = (data & 0x3f) << 4 | tmp;
		if (!freq[osc]) freq[osc] = 1024;
		if (nf && osc == 2) freq[3] = freq[2] << 1;
	}
}

float TMS9919::Update() {
	float r = 0;
	for (int i = 0; i < 3; i++) {
		if (++cnt[i] >= freq[i]) {
			cnt[i] = 0;
			v[i] = !v[i];
		}
		r += v[i] ? att[i] : -att[i];
	}
	if (++cnt[3] >= freq[3]) {
		cnt[3] = 0;
		rng = rng >> 1 | ((rng >> 2 ^ (rng >> 3 & white)) & 1) << 16;
	}
	return r + (rng & 1 ? att[3] : -att[3]);
}
