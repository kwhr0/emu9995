// TMS9995
// Copyright 2025 © Yasuo Kuwahara
// MIT License

#include "TMS9995.h"
#include <cstdlib>
#include <cstring>

// Unimplemented: x,ldcr,stcr,sbo,sbz,tb

#define P(x)			(cnv.pmf = &TMS9995::x, cnv.p)
#define PI(x, i)		(cnv.pmf = &TMS9995::x<i>, cnv.p)
#define PM(x, i, j)		(cnv.pmf = &TMS9995::x<i, j>, cnv.p)

#define S5(op, mask, x) {\
a(op | 0x00, mask | 0x30, PI(x, 0));\
a(op | 0x10, mask | 0x30, PI(x, 1));\
a(op | 0x20, mask | 0x30, PI(x, 2));\
a(op | 0x30, mask | 0x30, PI(x, 3));\
a(op | 0x20, mask | 0x3f, PI(x, 4));\
}
#define ME(op, mask, x, j) {\
a(op | 0x00, mask | 0x30, PM(x, 0, j));\
a(op | 0x10, mask | 0x30, PM(x, 1, j));\
a(op | 0x20, mask | 0x30, PM(x, 2, j));\
a(op | 0x30, mask | 0x30, PM(x, 3, j));\
a(op | 0x20, mask | 0x3f, PM(x, 4, j));\
}
#define MEE(op, mask, x) {\
ME(op | 0x000, mask | 0xc00, x, 0);\
ME(op | 0x400, mask | 0xc00, x, 1);\
ME(op | 0x800, mask | 0xc00, x, 2);\
ME(op | 0xc00, mask | 0xc00, x, 3);\
ME(op | 0x800, mask | 0xfc0, x, 4);\
}

static struct Insn {
	using pmf_t = void (TMS9995::*)(uint16_t);
	using pf_t = void (*)(TMS9995 *, uint16_t);
	Insn() {
		union { pmf_t pmf; pf_t p; } cnv; // not portable
		for (int i = 0; i < 0x10000; i++) fn[i] = P(undef);
		MEE(0x4000, 0xf000, szc);
		MEE(0x5000, 0xf000, szcb);
		MEE(0x6000, 0xf000, s);
		MEE(0x7000, 0xf000, sb);
		MEE(0x8000, 0xf000, c);
		MEE(0x9000, 0xf000, cb);
		MEE(0xa000, 0xf000, a);
		MEE(0xb000, 0xf000, ab);
		MEE(0xc000, 0xf000, mov);
		MEE(0xd000, 0xf000, movb);
		MEE(0xe000, 0xf000, soc);
		MEE(0xf000, 0xf000, socb);
		S5(0x2000, 0xfc00, coc);
		S5(0x2400, 0xfc00, czc);
		S5(0x2800, 0xfc00, _xor);
		S5(0x3800, 0xfc00, mpy);
		S5(0x3c00, 0xfc00, div);
		S5(0x0180, 0xffc0, divs);
		S5(0x01c0, 0xffc0, mpys);
		S5(0x2c00, 0xfc00, xop);
		S5(0x0400, 0xffc0, blwp);
		S5(0x0440, 0xffc0, b);
		S5(0x04c0, 0xffc0, clr);
		S5(0x0500, 0xffc0, neg);
		S5(0x0540, 0xffc0, inv);
		S5(0x0580, 0xffc0, inc);
		S5(0x05c0, 0xffc0, inct);
		S5(0x0600, 0xffc0, dec);
		S5(0x0640, 0xffc0, dect);
		S5(0x0680, 0xffc0, bl);
		S5(0x06c0, 0xffc0, swpb);
		S5(0x0700, 0xffc0, seto);
		S5(0x0740, 0xffc0, abs);
		a(0x1000, 0xff00, P(jmp));
		a(0x1100, 0xff00, P(jlt));
		a(0x1200, 0xff00, P(jle));
		a(0x1300, 0xff00, P(jeq));
		a(0x1400, 0xff00, P(jhe));
		a(0x1500, 0xff00, P(jgt));
		a(0x1600, 0xff00, P(jne));
		a(0x1700, 0xff00, P(jnc));
		a(0x1800, 0xff00, P(joc));
		a(0x1900, 0xff00, P(jno));
		a(0x1a00, 0xff00, P(jl));
		a(0x1b00, 0xff00, P(jh));
		a(0x1c00, 0xff00, P(jop));
		a(0x0800, 0xff00, P(sra));
		a(0x0900, 0xff00, P(srl));
		a(0x0a00, 0xff00, P(sla));
		a(0x0b00, 0xff00, P(src));
		a(0x0200, 0xffe0, P(li));
		a(0x0220, 0xffe0, P(ai));
		a(0x0240, 0xffe0, P(andi));
		a(0x0260, 0xffe0, P(ori));
		a(0x0280, 0xffe0, P(ci));
		a(0x02c0, 0xfff0, P(stst));
		a(0x02e0, 0xffe0, P(lwpi));
		a(0x0300, 0xffe0, P(limi));
		a(0x0080, 0xfff0, P(lst));
		a(0x0090, 0xfff0, P(lwp));
		a(0x02a0, 0xfff0, P(stwp));
		a(0x0380, 0xffe0, P(rtwp));
		a(0x0340, 0xffe0, P(idle));
		a(0x0360, 0xffe0, P(rset));
		a(0x03a0, 0xffe0, P(ckon));
		a(0x03c0, 0xffe0, P(ckof));
		a(0x03e0, 0xffe0, P(lrex));
	}
	void a(uint16_t op, uint16_t mask, pf_t f) {
		int lim = (op & 0xf000) + 0x1000;
		for (int i = op & 0xf000; i < lim; i++)
			if ((i & mask) == op) fn[i] = f;
	}
	static void exec1(TMS9995 *mpu, uint16_t op) { fn[op](mpu, op); }
	static inline pf_t fn[0x10000];
} insn;

TMS9995::TMS9995() : m(nullptr) {
#if TMS9995_TRACE
	memset(tracebuf, 0, sizeof(tracebuf));
	tracep = tracebuf;
#endif
}

void TMS9995::Reset() {
	irq1 = idlef = false;
	st = 0;
	wp = ld2(0);
	pc = ld2(2);
}

// RW: 0...address only 1...read 2...write 3...modify
// M: 0...register 1...indirect 2...indexed 3...autoincrement 4...symbolic
// S: 0...word 1...byte
template<int RW, int M, int S, typename F> TMS9995::u16 TMS9995::ea(int reg, F func) {
	u16 adr = 0, data = 0;
	reg &= 0xf;
	if constexpr (M == 1) { C(2); adr = wr(reg); }
	if constexpr (M == 2) { C(5); adr = wr(reg); adr += fetch2(); }
	if constexpr (M == 3) { C(5); adr = wr(reg); wr(reg) += 2 >> S; }
	if constexpr (M == 4) { C(2); adr = fetch2(); }
	if constexpr ((RW & 1) != 0) {
		if constexpr (!M)
			if constexpr (S) data = wr(reg) >> 8;
			else data = wr(reg);
		else if constexpr (S) data = ld1(adr);
		else data = ld2(adr);
	}
	if constexpr (RW == 1) func(data);
	if constexpr (RW == 2) data = func();
	if constexpr (RW == 3) data = func(data);
	if constexpr ((RW & 2) != 0) {
		if constexpr (!M)
			if constexpr (S) stRb(reg, data);
			else stR(reg, data);
		else if constexpr (S) st1(adr, data);
		else st2(adr, data);
	}
	return adr;
}

template<typename T, bool R> void TMS9995::sr(u16 op) {
	T v = wr(op & 0xf);
	u16 c = sftcnt(op), l = c ? v & 1 << (c - 1) : 0;
	v >>= c;
	if constexpr (R) v |= l ? MSB : 0;
	stR(op & 0xf, fsft(v, l));
}

template<int M> void TMS9995::div(u16 op) {
	ea<1, M>(op, [&](u16 v) {
		if (u16 x = op >> 6 & 0xf, u = wr(x); u < v) {
			C(34);
			u32 t = u << 16 | wr(x + 1); stR(x, t / v); stR(x + 1, t % v); fdiv(0);
		}
		else { C(10); fdiv(MSB); }
	});
}

template<int M> void TMS9995::divs(u16 op) {
	ea<1, M>(op, [&](s16 v) {
		if (v)
			if (s32 t = wr(0) << 16 | wr(1), q = t / v; q == (s16)q) {
				C(39);
				stR(0, q); stR(1, t % v); fdivs(0);
				return;
			}
		C(36); fdivs(MSB);
	});
}

int TMS9995::Execute(int n) {
	clock = 0;
	do {
#if TMS9995_TRACE
		tracep->pc = pc;
		tracep->index = 0;
#endif
		if (irq1 && st & 0xf) {
			bs(4);
			st &= 0xfff0;
			irq1 = idlef = false;
		}
		if (idlef) return 0;
		Insn::exec1(this, fetch2());
#if TMS9995_TRACE
		tracep->st = st;
#if TMS9995_TRACE > 1
		if (++tracep >= tracebuf + TRACEMAX - 1) StopTrace();
#else
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
#endif
#endif
	} while (!idlef && clock < n);
	return idlef ? 0 : clock - n;
}

// ST0:Lgt ST1:Agt ST2:E ST3:C ST4:O ST5:P ST6:X ST12-15:I
template<int DM, int S> TMS9995::u16 TMS9995::fset(u16 r, u16 s, u16 d) {
	if constexpr ((DM & 0xf) == LDEF) {
		if (r) st |= ML;
		else st &= ~ML;
	}
	if constexpr ((DM & 0xf) == LABS) {
		if (s) st |= ML;
		else st &= ~ML;
	}
	if constexpr ((DM & 0xf) == LC_) { // s:(SA)or(W) d:(DA)orIOP
		if constexpr (S)
			if (((s & ~d) | (~(s ^ d) & d - s)) & 0x80) st |= ML;
			else st &= ~ML;
		else if (((s & ~d) | (~(s ^ d) & d - s)) & MSB) st |= ML;
		else st &= ~ML;
	}
	if constexpr ((DM & 0xf) == L32) {
		if (r | s) st |= ML;
		else st &= ~ML;
	}
	if constexpr ((DM & 0xf0) == ADEF) {
		if (r && !(r & MSB)) st |= MA;
		else st &= ~MA;
	}
	if constexpr ((DM & 0xf0) == AABS) {
		if (s && !(s & MSB)) st |= MA;
		else st &= ~MA;
	}
	if constexpr ((DM & 0xf0) == AC_) { // s:(SA)or(W) d:(DA)orIOP
		if constexpr (S)
			if (((~s & d & MSB) | (~(s ^ d) & d - s)) & 0x80) st |= MA;
			else st &= ~MA;
		else if (((~s & d & MSB) | (~(s ^ d) & d - s)) & MSB) st |= MA;
		else st &= ~MA;
	}
	if constexpr ((DM & 0xf0) == A32) {
		if (r | s && !(r & MSB)) st |= MA;
		else st &= ~MA;
	}
	if constexpr ((DM & 0xf00) == EDEF) {
		if (!r) st |= ME;
		else st &= ~ME;
	}
	if constexpr ((DM & 0xf00) == EABS) {
		if (!s) st |= ME;
		else st &= ~ME;
	}
	if constexpr ((DM & 0xf00) == EC_) {
		if (s == d) st |= ME;
		else st &= ~ME;
	}
	if constexpr ((DM & 0xf00) == E32) {
		if (!(r | s)) st |= ME;
		else st &= ~ME;
	}
	if constexpr ((DM & 0xf000) == CADD) {
		if (((s & d) | (~r & d) | (s & ~r)) & MSB) st |= MC;
		else st &= ~MC;
	}
	if constexpr ((DM & 0xf000) == CSUB) {
		if (((s & ~d) | (r & ~d) | (s & r)) & MSB) st |= MC;
		else st &= ~MC;
	}
	if constexpr ((DM & 0xf000) == CSFT) {
		if (d) st |= MC;
		else st &= ~MC;
	}
	if constexpr ((DM & 0xf0000) == OADD) {
		if (~(s ^ d) & (r ^ d) & MSB) st |= MO;
		else st &= ~MO;
	}
	if constexpr ((DM & 0xf0000) == OSUB) {
		if ((s ^ d) & (r ^ d) & MSB) st |= MO;
		else st &= ~MO;
	}
	if constexpr ((DM & 0xf0000) == OINC) {
		if (~s & r & MSB) st |= MO;
		else st &= ~MO;
	}
	if constexpr ((DM & 0xf0000) == ODEC) {
		if (s & ~r & MSB) st |= MO;
		else st &= ~MO;
	}
	if constexpr ((DM & 0xf0000) == OABS) {
		if (s == MSB) st |= MO;
		else st &= ~MO;
	}
	if constexpr ((DM & 0xf00000) == PDEF) {
		u8 t = r;
		t ^= t >> 4;
		t ^= t >> 2;
		t ^= t >> 1;
		if (t & 1) st |= MP;
		else st &= ~MP;
	}
	return r;
}

#if TMS9995_TRACE
#include <string>
void TMS9995::StopTrace() {
	TraceBuffer *endp = tracep;
	int i = 0;
	FILE *fo;
	if (!(fo = fopen((std::string(getenv("HOME")) + "/Desktop/trace.txt").c_str(), "w"))) exit(1);
	do {
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
		fprintf(fo, "%4d %04x %04x ", i++, tracep->pc, __builtin_bswap16((u16 &)m[tracep->pc]));
		fprintf(fo, "%c%c%c%c%c%c%c ",
				tracep->st & 0x8000 ? 'L' : '-',
				tracep->st & 0x4000 ? 'A' : '-',
				tracep->st & 0x2000 ? 'E' : '-',
				tracep->st & 0x1000 ? 'C' : '-',
				tracep->st & 0x0800 ? 'O' : '-',
				tracep->st & 0x0400 ? 'P' : '-',
				tracep->st & 0x0200 ? 'X' : '-');
		for (Acs *p = tracep->acs; p < tracep->acs + tracep->index; p++) {
			switch (p->type) {
				case acsLoad8:
					fprintf(fo, "L %04x %02x ", p->adr, p->data & 0xff);
					break;
				case acsLoad16:
					fprintf(fo, "L %04x %04x ", p->adr, p->data);
					break;
				case acsStore8:
					fprintf(fo, "S %04x %02x ", p->adr, p->data & 0xff);
					break;
				case acsStore16:
					fprintf(fo, "S %04x %04x ", p->adr, p->data);
					break;
				case acsStoreR:
					fprintf(fo, "%04x->R%d ", p->data, p->adr);
					break;
			}
		}
		fprintf(fo, "\n");
	} while (tracep != endp);
	fclose(fo);
	fprintf(stderr, "trace dumped.\n");
	exit(1);
}
#endif

void TMS9995::undef(u16 op) {
	fprintf(stderr, "undefined instruction: PC=%04x OP=%04x\n", pc - 2, op);
#if TMS9995_TRACE
	StopTrace();
#endif
	exit(1);
}
