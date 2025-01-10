// TMS9995
// Copyright 2025 © Yasuo Kuwahara
// MIT License

#include "main.h"		// emulator
#include <cstdio>		// emulator
#include <cstdint>

#define TMS9995_TRACE		0

#if TMS9995_TRACE
#define TMS9995_TRACE_LOG(adr, data, type) \
	if (tracep->index < ACSMAX) tracep->acs[tracep->index++] = { adr, data, type }
#else
#define TMS9995_TRACE_LOG(adr, data, type)
#endif

class TMS9995 {
	friend class Insn;
	using s8 = int8_t;
	using u8 = uint8_t;
	using s16 = int16_t;
	using u16 = uint16_t;
	using s32 = int32_t;
	using u32 = uint32_t;
	enum {
		LL, LA, LE, LC, LO, LP, LX, LI = 12
	};
	enum {
		MSB = 0x8000U,
		ML = MSB >> LL, MA = MSB >> LA, ME = MSB >> LE, MC = MSB >> LC, MO = MSB >> LO, MP = MSB >> LP, MX = MSB >> LX,
		MI = 0xf
	};
	enum {
		FDEF = 1, FADD, FSUB, FINC, FDEC, FC_, FS, FSFT, FABS, F32
	};
#define F(flag, type)	flag##type = F##type << (L##flag << 2)
	enum {
		F(L, DEF), F(L, C_), F(L, ABS), F(L, 32),
		F(A, DEF), F(A, C_), F(A, ABS), F(A, 32),
		F(E, DEF), F(E, C_), F(E, ABS), F(E, 32),
		F(C, ADD), F(C, SUB), F(C, SFT), F(C, ABS),
		F(O, ADD), F(O, SUB), F(O, INC), F(O, DEC), F(O, ABS),
		F(P, DEF)
	};
#undef F
public:
	TMS9995();
	void Reset();
	void SetMemoryPtr(u8 *p) { m = p; }
	int GetPC() const { return pc; }
	void INT1() { irq1 = true; }
	int Execute(int n);
#if TMS9995_TRACE
	void StopTrace();
#endif
private:
	void C(int n) { clock += n; }
	u16 &wr(u16 num) { return (u16 &)m[wp + (num << 1)]; } // working register
	void stRb(u16 num, u8 data) {
		wr(num) = data << 8 | (wr(num) & 0xff);
		TMS9995_TRACE_LOG(num, data, acsStoreR);
	}
	void stR(u16 num, u16 data) {
		wr(num) = data;
		TMS9995_TRACE_LOG(num, data, acsStoreR);
	}
	// customized access -- start
	u16 ld1(u16 adr) {
		u16 data = m[adr];
		TMS9995_TRACE_LOG(adr, data, acsLoad8);
		return data;
	}
	u16 ld2(u16 adr) {
		u16 data = __builtin_bswap16((u16 &)m[adr]);
		TMS9995_TRACE_LOG(adr, data, acsLoad16);
		return data;
	}
	void st1(u16 adr, u8 data) {
		switch (adr) {
			case 0xfff9: set_dcsg(data); break;
			case 0xfffa: set_vram(data << 8); break;
			case 0xfffb: putchar(data); break;
			default: m[adr] = data; break;
		}
		TMS9995_TRACE_LOG(adr, data, acsStore8);
	}
	void st2(u16 adr, u16 data) {
		(u16 &)m[adr] = __builtin_bswap16(data);
		TMS9995_TRACE_LOG(adr, data, acsStore16);
	}
	s16 fetch2() {
		s16 v = __builtin_bswap16((u16 &)m[pc]);
		pc += 2;
		return v;
	}
	// customized access -- end
	u8 *m;
	u16 pc, wp, st;
	int clock;
	bool irq1, idlef;
#if TMS9995_TRACE
	static constexpr int TRACEMAX = 10000;
	static constexpr int ACSMAX = 3;
	enum {
		acsStoreR = 1,
		acsStore8, acsStore16, acsLoad8, acsLoad16
	};
	struct Acs {
		u16 adr, data;
		u8 type;
	};
	struct TraceBuffer {
		u16 pc, index, st;
		Acs acs[ACSMAX];
	};
	TraceBuffer tracebuf[TRACEMAX];
	TraceBuffer *tracep;
#endif
	// flags
	// ST0:Lgt ST1:Agt ST2:E ST3:C ST4:O ST5:P ST6:X ST12-15:I
	template<int DM, int S = 0> u16 fset(u16 r, u16 s = 0, u16 d = 0);
	u16 fmov(u16 r) { return fset<LDEF | ADEF | EDEF>(r); }
	u16 fmovb(u16 r) { return fset<LDEF | ADEF | EDEF | PDEF, 1>(r); }
	u16 fadd(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CADD | OADD>(r, s, d); }
	u16 faddb(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CADD | OADD | PDEF>(r, s, d); }
	u16 fsub(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CSUB | OSUB>(r, s, d); }
	u16 fsubb(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CSUB | OSUB | PDEF>(r, s, d); }
	u16 fneg(u16 r, u16 s) { return fset<LDEF | ADEF | EDEF | CSUB | OABS>(r, s); }
	u16 fc(u16 r) { return fset<EDEF>(r); }
	u16 fmpys(u32 r) { return fset<L32 | A32 | E32>(r >> 16, r); }
	u16 fdiv(u16 s) { return fset<OABS>(0, s); }
	u16 fdivs(u16 r) { return fset<LDEF | ADEF | EDEF | OABS>(r); }
	u16 fsft(u16 r, u16 d) { return fset<LDEF | ADEF | EDEF | CSFT>(r, 0, d); }
	u16 fsla(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CSFT | OABS>(r, s, d); }
	u16 finc(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CADD | OINC>(r, s, d); }
	u16 fdec(u16 r, u16 s, u16 d) { return fset<LDEF | ADEF | EDEF | CSUB | ODEC>(r, s, d); }
	u16 fabs(u16 r, u16 s) { return fset<LABS | AABS | EABS | CSUB | OABS>(r, s); }
	u16 fc(u16 s, u16 d) { return fset<LC_ | AC_ | EC_>(0, s, d); }
	u16 fcb(u16 s, u16 d) { return fset<LC_ | AC_ | EC_ | PDEF, 1>(s, s, d); }
	template<int RW, int M, int S = 0, typename F> u16 ea(int reg, F func);
	void bs(u16 sa) { // blwp and interrupt
		u16 oldwp = wp, oldpc = pc;
		wp = ld2(sa); pc = ld2(sa + 2);
		stR(13, oldwp); stR(14, oldpc); stR(15, st);
	}
	// instructions
	// 4.5.1
	template<int MS, int MD> void a(u16 op) {
		C(8); ea<1, MS>(op, [&](u16 v) { return ea<3, MD>(op >> 6, [&](u16 d) { return fadd(d + v, v, d); }); });
	}
	template<int MS, int MD> void ab(u16 op) {
		C(5); ea<1, MS, 1>(op, [&](u8 v) { return ea<3, MD, 1>(op >> 6, [&](u8 d) { return faddb(d + v, v, d); }); });
	}
	template<int MS, int MD> void s(u16 op) {
		C(8); ea<1, MS>(op, [&](u16 v) { return ea<3, MD>(op >> 6, [&](u16 d) { return fsub(d - v, v, d); }); });
	}
	template<int MS, int MD> void sb(u16 op) {
		C(5); ea<1, MS, 1>(op, [&](u8 v) { return ea<3, MD, 1>(op >> 6, [&](u8 d) { return fsubb(d - v, v, d); }); });
	}
	template<int MS, int MD> void c(u16 op) {
		C(7); ea<1, MS>(op, [&](u16 v) { return ea<1, MD>(op >> 6, [&](u16 d) { return fc(v, d); }); });
	}
	template<int MS, int MD> void cb(u16 op) {
		C(5); ea<1, MS, 1>(op, [&](u8 v) { return ea<1, MD, 1>(op >> 6, [&](u8 d) { return fcb(v, d); }); });
	}
	template<int MS, int MD> void soc(u16 op) {
		C(8); ea<1, MS>(op, [&](u16 v) { return ea<3, MD>(op >> 6, [&](u16 d) { return fmov(v | d); }); });
	}
	template<int MS, int MD> void socb(u16 op) {
		C(5); ea<1, MS, 1>(op, [&](u8 v) { return ea<3, MD, 1>(op >> 6, [&](u8 d) { return fmovb(v | d); }); });
	}
	template<int MS, int MD> void mov(u16 op) {
		C(6); ea<1, MS>(op, [&](u16 v) { return ea<2, MD>(op >> 6, [&]{ return fmov(v); }); });
	}
	template<int MS, int MD> void movb(u16 op) {
		C(4); ea<1, MS, 1>(op, [&](u8 v) { return ea<2, MD, 1>(op >> 6, [&]{ return fmovb(v); }); });
	}
	template<int MS, int MD> void szc(u16 op) {
		C(8); ea<1, MS>(op, [&](u16 v) { return ea<3, MD>(op >> 6, [&](u16 d) { return fmov(d & ~v); }); });
	}
	template<int MS, int MD> void szcb(u16 op) {
		C(5); ea<1, MS, 1>(op, [&](u8 v) { return ea<3, MD, 1>(op >> 6, [&](u8 d) { return fmovb(d & ~v); }); });
	}
	// 4.5.2
	template<int M> void coc(u16 op) { C(7); ea<1, M>(op, [&](u16 v) { fc(v & ~wr(op >> 6 & 0xf)); }); }
	template<int M> void czc(u16 op) { C(7); ea<1, M>(op, [&](u16 v) { fc(v & wr(op >> 6 & 0xf)); }); }
	template<int M> void mpy(u16 op) {
		C(29); ea<1, M>(op, [&](u16 v) { u32 x = op >> 6 & 0xf, t = v * wr(x); stR(x, t >> 16); stR(x + 1, t); });
	}
	template<int M> void div(u16 op);
	template<int M> void _xor(u16 op) { C(8); ea<1, M>(op, [&](u16 v) { u16 x = op >> 6 & 0xf; stR(x, fmov(v ^= wr(x))); }); }
	// 4.5.3
	template<int M> void mpys(u16 op) {
		C(30); ea<1, M>(op, [&](s16 v) { s32 t = v * (s16)wr(0); stR(0, t >> 16); stR(1, t); fmpys(t); });
	}
	template<int M> void divs(u16 op);
	// 4.5.4
	template<int M> void xop(u16 op) { C(22); ea<1, M>(op, [&](u16 v) { bs((op >> 4 & 0x3c) | 0x40); stR(11, v); }); }
	// 4.5.5
	template<int M> void b(u16 op) { C(4); pc = ea<0, M>(op, []{}); }
	template<int M> void bl(u16 op) { C(7); u16 t = ea<0, M>(op, []{}); stR(11, pc); pc = t; }
	template<int M> void blwp(u16 op) { C(7); bs(ea<0, M>(op, []{})); }
	template<int M> void clr(u16 op) { C(5); ea<2, M>(op, []{ return 0; }); }
	template<int M> void seto(u16 op) { C(5); ea<2, M>(op, []{ return 0xff; }); }
	template<int M> void inc(u16 op) { C(6); ea<3, M>(op, [&](u16 v) { return finc(v + 1, v, 1); }); }
	template<int M> void inct(u16 op) { C(6); ea<3, M>(op, [&](u16 v) { return finc(v + 2, v, 2); }); }
	template<int M> void dec(u16 op) { C(6); ea<3, M>(op, [&](u16 v) { return fdec(v - 1, v, 1); }); }
	template<int M> void dect(u16 op) { C(6); ea<3, M>(op, [&](u16 v) { return fdec(v - 2, v, 2); }); }
	template<int M> void swpb(u16 op) { C(16); ea<3, M>(op, [](u16 v) { return v << 8 | v >> 8; }); }
	template<int M> void neg(u16 op) { C(6); ea<3, M>(op, [&](u16 v) { return fneg(-v, v); }); }
	template<int M> void inv(u16 op) { C(6); ea<3, M>(op, [&](u16 v) { return fmov(~v); }); }
	template<int M> void abs(u16 op) { C(6); ea<3, M>(op, [&](s16 v) { u16 t = v >= 0 ? v : -v; return fabs(t, v); }); }
	// 4.5.8
	void jeq(u16 op) { C(4); if (st & ME) pc += (s8)op << 1; }
	void jne(u16 op) { C(4); if (!(st & ME)) pc += (s8)op << 1; }
	void jgt(u16 op) { C(4); if (st & MA) pc += (s8)op << 1; }
	void jmp(u16 op) { C(4); pc += (s8)op << 1; }
	void jnc(u16 op) { C(4); if (!(st & MC)) pc += (s8)op << 1; }
	void jno(u16 op) { C(4); if (!(st & MO)) pc += (s8)op << 1; }
	void joc(u16 op) { C(4); if (st & MC) pc += (s8)op << 1; }
	void jop(u16 op) { C(4); if (st & MP) pc += (s8)op << 1; }
	void jh(u16 op)  { C(4); if ((st & (ML | ME)) == ML) pc += (s8)op << 1; }
	void jhe(u16 op) { C(4); if (st & (ML | ME)) pc += (s8)op << 1; }
	void jlt(u16 op) { C(4); if (!(st & (MA | ME))) pc += (s8)op << 1; }
	void jle(u16 op) { C(4); if (!(st & ML) || st & ME) pc += (s8)op << 1; }
	void jl(u16 op)  { C(4); if (!(st & (ML | ME))) pc += (s8)op << 1; }
	// 4.5.9
	int sftcnt(u16 c) {
		C(c & 0xf0 ? 8 + (c = c >> 4 & 0xf) : 11 + (c = wr(0))); // 0<=c<=15 avoid compiler bug
		// c = (wr(0) - 1 & 0xf) + 1 // 1<=c<=16
		return c;
	}
	void sla(u16 op) {
		u16 v = wr(op & 0xf), c = sftcnt(op), l = v & 0x10000 >> c, ref = v ^ s16(v) >> 15, mask = c < 16 ? (1 << (15 - c)) - 1 : 0;
		stR(op & 0xf, fsla(v <<= c, ref & ~mask ? MSB : 0, l));
	}
	template<typename T, bool R> void sr(u16 op);
	void sra(u16 op) { sr<s16, false>(op); }
	void srl(u16 op) { sr<u16, false>(op); }
	void src(u16 op) { sr<u16, true>(op); }
	// 4.5.10
	void ai(u16 op) { C(8); u16 s = fetch2(), d = wr(op & 0xf); stR(op & 0xf, fadd(s + d, s, d)); }
	void andi(u16 op) { C(8); stR(op & 0xf, fmov(wr(op & 0xf) & fetch2())); }
	void ci(u16 op) { C(7); fc(wr(op & 0xf), fetch2()); }
	void li(u16 op) { C(6); stR(op & 0xf, fmov(fetch2())); }
	void ori(u16 op) { C(8); stR(op & 0xf, fmov(wr(op & 0xf) | fetch2())); }
	// 4.5.11
	void lwpi(u16) { C(6); wp = fetch2(); }
	void limi(u16) { C(7); st = (st & 0xfff0) | (fetch2() & 0xf); }
	// 4.5.12
	void stst(u16 op) { C(5); stR(op & 0xf, st); }
	void lst(u16 op) { C(7); st = wr(op & 0xf); }
	void stwp(u16 op) { C(5); stR(op & 0xf, wp); }
	void lwp(u16 op) { C(6); wp = wr(op & 0xf); }
	// 4.5.13
	void rtwp(u16) { C(10); st = wr(15); pc = wr(14); wp = wr(13); }
	// 4.5.14
	void idle(u16) { C(8); idlef = true; }
	void rset(u16) { C(8); Reset(); }
	void ckof(u16) { C(8); idlef = true; st = 0; emu_exit(); }
	void ckon(u16) { C(8); }
	void lrex(u16) { C(8); }
	void undef(u16 op);
};
