/* ============================================================================== //
// This file is apart of the Cobalt Programming Language. Cobalt is under the MIT //
// License. Read `cobalt.h` for license information.                              //
// ============================================================================== */

#define VM_CASE(name) vmcase(OP_##name) // just a macro to make the code more readable
#include <stdio.h>

#ifdef JIT
/* Include libjit */
#include <jit/jit.h>
#endif





// Interpreter Executer
static CallInfo *luaV_execute_(lua_State *L, CallInfo *ci)
{
  LClosure *cl;
  TValue *k;
  StkId base;
  const Instruction *pc;
  int trap;
#if LUA_USE_JUMPTABLE
#include "ljumptab.h"
#endif
 startfunc:
  trap = L->hookmask;
 returning:  /* trap already set */
  cl = clLvalue(s2v(ci->func));
#if AOT
  if (cl->p->aot_implementation) {
      return ci;
  }
#endif
  k = cl->p->k;
  pc = ci->u.l.savedpc;
  if (l_unlikely(trap)) {
    if (pc == cl->p->code) {  /* first instruction (not resuming)? */
      if (cl->p->is_vararg)
        trap = 0;  /* hooks will start after VARARGPREP instruction */
      else  /* check 'call' hook */
        luaD_hookcall(L, ci);
    }
    ci->u.l.trap = 1;  /* assume trap is on, for now */
  }
  base = ci->func + 1;
  #ifdef JIT
  /* JIT initialization */
  #endif
  /* main loop of interpreter */
  for (;;) {
    Instruction i;  /* instruction being executed */
    StkId ra;  /* instruction's A register */
    vmfetch();
    // low-level line tracing for debugging Cobalt
    //printf("line: %d\n", luaG_getfuncline(cl->p, pcRel(pc, cl->p)));
    lua_assert(base == ci->func + 1);
    lua_assert(base <= L->top && L->top < L->stack_last);
    /* invalidate top for instructions not expecting it */
    lua_assert(isIT(i) || (cast_void(L->top = base), 1));
    #ifdef JIT 
    // JIT
    if (jit == 1){
      vmdispatch (i) {
      VM_CASE(MOVE) { printf("MOVE\n");
        setobjs2s(L, ra, RB(i));
        vmbreak;
      }
      VM_CASE(LOADI) { printf("LOADI\n");
        lua_Integer b = GETARG_sBx(i);
        setivalue(s2v(ra), b);
        vmbreak;
      }
      VM_CASE(LOADF) { printf("LOADF\n");
        int b = GETARG_sBx(i);
        setfltvalue(s2v(ra), cast_num(b));
        vmbreak;
      }
      VM_CASE(LOADK) { printf("LOADK\n");
        TValue *rb = k + GETARG_Bx(i);
        setobj2s(L, ra, rb);
        vmbreak;
      }
      VM_CASE(LOADKX) { printf("LOADKX\n");
        TValue *rb;
        rb = k + GETARG_Ax(*pc); pc++;
        setobj2s(L, ra, rb);
        vmbreak;
      }
      VM_CASE(LOADFALSE) { printf("LOADFALSE\n");
        setbfvalue(s2v(ra));
        vmbreak;
      }
      VM_CASE(LFALSESKIP) { printf("LFALSESKIP\n");
        setbfvalue(s2v(ra));
        pc++;  /* skip next instruction */
        vmbreak;
      }
      VM_CASE(LOADTRUE) { printf("LOADTRUE\n");
        setbtvalue(s2v(ra));
        vmbreak;
      }
      VM_CASE(LOADNIL) { printf("LOADNIL\n");
        int b = GETARG_B(i);
        do {
          setnilvalue(s2v(ra++));
        } while (b--);
        vmbreak;
      }
      VM_CASE(GETUPVAL) { printf("GETUPVAL\n");
        int b = GETARG_B(i);
        setobj2s(L, ra, cl->upvals[b]->v);
        vmbreak;
      }
      VM_CASE(SETUPVAL) { printf("SETUPVAL\n");
        UpVal *uv = cl->upvals[GETARG_B(i)];
        setobj(L, uv->v, s2v(ra));
        luaC_barrier(L, uv, s2v(ra));
        vmbreak;
      }
      VM_CASE(GETTABUP) { printf("GETTABUP\n");
        const TValue *slot;
        TValue *upval = cl->upvals[GETARG_B(i)]->v;
        TValue *rc = KC(i);
        TString *key = tsvalue(rc);  /* key must be a string */
        if (luaV_fastget(L, upval, key, slot, luaH_getshortstr)) {
          setobj2s(L, ra, slot);
        }
        else
          Protect(luaV_finishget(L, upval, rc, ra, slot));
        vmbreak;
      }
      VM_CASE(GETTABLE) { printf("GETTABLE\n");
        const TValue *slot;
        TValue *rb = vRB(i);
        TValue *rc = vRC(i);
        lua_Unsigned n;
        if (ttisinteger(rc)  /* fast track for integers? */
            ? (cast_void(n = ivalue(rc)), luaV_fastgeti(L, rb, n, slot))
            : luaV_fastget(L, rb, rc, slot, luaH_get)) {
          setobj2s(L, ra, slot);
        }
        else
          Protect(luaV_finishget(L, rb, rc, ra, slot));
        vmbreak;
      }
      VM_CASE(GETI) { printf("GETI\n");
        const TValue *slot;
        TValue *rb = vRB(i);
        int c = GETARG_C(i);
        if (luaV_fastgeti(L, rb, c, slot)) {
          setobj2s(L, ra, slot);
        }
        else {
          TValue key;
          setivalue(&key, c);
          Protect(luaV_finishget(L, rb, &key, ra, slot));
        }
        vmbreak;
      }
      VM_CASE(GETFIELD) { printf("GETFIELD\n");
        const TValue *slot;
        TValue *rb = vRB(i);
        TValue *rc = KC(i);
        TString *key = tsvalue(rc);  /* key must be a string */
        if (luaV_fastget(L, rb, key, slot, luaH_getshortstr)) {
          setobj2s(L, ra, slot);
        }
        else
          Protect(luaV_finishget(L, rb, rc, ra, slot));
        vmbreak;
      }
      VM_CASE(SETTABUP) { printf("SETTABUP\n");
        const TValue *slot;
        TValue *upval = cl->upvals[GETARG_A(i)]->v;
        TValue *rb = KB(i);
        TValue *rc = RKC(i);
        TString *key = tsvalue(rb);  /* key must be a string */
        if (luaV_fastget(L, upval, key, slot, luaH_getshortstr)) {
          luaV_finishfastset(L, upval, slot, rc);
        }
        else
          Protect(luaV_finishset(L, upval, rb, rc, slot));
        vmbreak;
      }
      VM_CASE(SETTABLE) { printf("SETTABLE\n");
        const TValue *slot;
        TValue *rb = vRB(i);  /* key (table is in 'ra') */
        TValue *rc = RKC(i);  /* value */
        lua_Unsigned n;
        if (ttisinteger(rb)  /* fast track for integers? */
            ? (cast_void(n = ivalue(rb)), luaV_fastgeti(L, s2v(ra), n, slot))
            : luaV_fastget(L, s2v(ra), rb, slot, luaH_get)) {
          luaV_finishfastset(L, s2v(ra), slot, rc);
        }
        else
          Protect(luaV_finishset(L, s2v(ra), rb, rc, slot));
        vmbreak;
      }
      VM_CASE(SETI) { printf("SETI\n");
        const TValue *slot;
        int c = GETARG_B(i);
        TValue *rc = RKC(i);
        if (luaV_fastgeti(L, s2v(ra), c, slot)) {
          luaV_finishfastset(L, s2v(ra), slot, rc);
        }
        else {
          TValue key;
          setivalue(&key, c);
          Protect(luaV_finishset(L, s2v(ra), &key, rc, slot));
        }
        vmbreak;
      }
      VM_CASE(SETFIELD) { printf("SETFIELD\n");
        const TValue *slot;
        TValue *rb = KB(i);
        TValue *rc = RKC(i);
        TString *key = tsvalue(rb);  /* key must be a string */
        if (luaV_fastget(L, s2v(ra), key, slot, luaH_getshortstr)) {
          luaV_finishfastset(L, s2v(ra), slot, rc);
        }
        else
          Protect(luaV_finishset(L, s2v(ra), rb, rc, slot));
        vmbreak;
      }
      VM_CASE(NEWTABLE) { printf("NEWTABLE\n");
        int b = GETARG_B(i);  /* log2(hash size) + 1 */
        int c = GETARG_C(i);  /* array size */
        Table *t;
        if (b > 0)
          b = 1 << (b - 1);  /* size is 2^(b - 1) */
        lua_assert((!TESTARG_k(i)) == (GETARG_Ax(*pc) == 0));
        if (TESTARG_k(i))  /* non-zero extra argument? */
          c += GETARG_Ax(*pc) * (MAXARG_C + 1);  /* add it to size */
        pc++;  /* skip extra argument */
        L->top = ra + 1;  /* correct top in case of emergency GC */
        t = luaH_new(L);  /* memory allocation */
        sethvalue2s(L, ra, t);
        if (b != 0 || c != 0)
          luaH_resize(L, t, c, b);  /* idem */
        checkGC(L, ra + 1);
        vmbreak;
      }
      VM_CASE(SELF) { printf("SELF\n");
        const TValue *slot;
        TValue *rb = vRB(i);
        TValue *rc = RKC(i);
        TString *key = tsvalue(rc);  /* key must be a string */
        setobj2s(L, ra + 1, rb);
        if (luaV_fastget(L, rb, key, slot, luaH_getstr)) {
          setobj2s(L, ra, slot);
        }
        else
          Protect(luaV_finishget(L, rb, rc, ra, slot));
        vmbreak;
      }
      VM_CASE(ADDI) { printf("ADDI\n");
        op_arithI(L, l_addi, luai_numadd);
        vmbreak;
      }
      VM_CASE(ADDK) { printf("ADDK\n");
        op_arithK(L, l_addi, luai_numadd);
        vmbreak;
      }
      VM_CASE(SUBK) { printf("SUBK\n");
        op_arithK(L, l_subi, luai_numsub);
        vmbreak;
      }
      VM_CASE(MULK) { printf("MULK\n");
        op_arithK(L, l_muli, luai_nummul);
        vmbreak;
      }
      VM_CASE(MODK) { printf("MODK\n");
        op_arithK(L, luaV_mod, luaV_modf);
        vmbreak;
      }
      VM_CASE(POWK) {  printf("POWK\n");
        op_arithfK(L, luai_numpow);
        vmbreak;
      }
      VM_CASE(DIVK) { printf("DIVK\n");
        op_arithfK(L, luai_numdiv);
        vmbreak;
      }
      VM_CASE(IDIVK) { printf("IDIVK\n");
        op_arithK(L, luaV_idiv, luai_numidiv);
        vmbreak;
      }
      VM_CASE(BANDK) { printf("BANDK\n");
        op_bitwiseK(L, l_band);
        vmbreak;
      }
      VM_CASE(BORK) { printf("BORK\n");
        op_bitwiseK(L, l_bor);
        vmbreak;
      }
      VM_CASE(BXORK) { printf("BXORK\n");
        op_bitwiseK(L, l_bxor);
        vmbreak;
      }
      VM_CASE(SHRI) { printf("SHRI\n");
        TValue *rb = vRB(i);
        int ic = GETARG_sC(i);
        lua_Integer ib;
        if (tointegerns(rb, &ib)) {
          pc++; setivalue(s2v(ra), luaV_shiftl(ib, -ic));
        }
        vmbreak;
      }
      VM_CASE(SHLI) { printf("SHLI\n");
        TValue *rb = vRB(i);
        int ic = GETARG_sC(i);
        lua_Integer ib;
        if (tointegerns(rb, &ib)) {
          pc++; setivalue(s2v(ra), luaV_shiftl(ic, ib));
        }
        vmbreak;
      }
      VM_CASE(ADD) { printf("ADD\n");
        op_arith(L, l_addi, luai_numadd);
        vmbreak;
      }
      VM_CASE(SUB) { printf("SUB\n");
        op_arith(L, l_subi, luai_numsub);
        vmbreak;
      }
      VM_CASE(MUL) { printf("MUL\n");
        op_arith(L, l_muli, luai_nummul);
        vmbreak;
      }
      VM_CASE(MOD) { printf("MOD\n");
        op_arith(L, luaV_mod, luaV_modf);
        vmbreak;
      }
      VM_CASE(POW) { printf("POW\n");
        op_arithf(L, luai_numpow);
        vmbreak;
      }
      VM_CASE(DIV) {  /* float division (always with floats) */ printf("DIV\n");
        op_arithf(L, luai_numdiv);
        vmbreak;
      }
      VM_CASE(IDIV) {  /* floor division */ printf("IDIV\n");
        op_arith(L, luaV_idiv, luai_numidiv);
        vmbreak;
      }
      VM_CASE(BAND) { printf("BAND\n");
        op_bitwise(L, l_band);
        vmbreak;
      }
      VM_CASE(BOR) { printf("BOR\n");
        op_bitwise(L, l_bor);
        vmbreak;
      }
      VM_CASE(BXOR) { printf("BXOR\n");
        op_bitwise(L, l_bxor);
        vmbreak;
      }
      VM_CASE(SHR) { printf("SHR\n");
        op_bitwise(L, luaV_shiftr);
        vmbreak;
      }
      VM_CASE(SHL) { printf("SHL\n");
        op_bitwise(L, luaV_shiftl);
        vmbreak;
      }
      VM_CASE(MMBIN) { printf("MMBIN\n");
        Instruction pi = *(pc - 2);  /* original arith. expression */
        TValue *rb = vRB(i);
        TMS tm = (TMS)GETARG_C(i);
        StkId result = RA(pi);
        lua_assert(OP_ADD <= GET_OPCODE(pi) && GET_OPCODE(pi) <= OP_SHR);
        Protect(luaT_trybinTM(L, s2v(ra), rb, result, tm));
        vmbreak;
      }
      VM_CASE(MMBINI) { printf("MMBINI\n");
        Instruction pi = *(pc - 2);  /* original arith. expression */
        int imm = GETARG_sB(i);
        TMS tm = (TMS)GETARG_C(i);
        int flip = GETARG_k(i);
        StkId result = RA(pi);
        Protect(luaT_trybiniTM(L, s2v(ra), imm, flip, result, tm));
        vmbreak;
      }
      VM_CASE(MMBINK) { printf("MMBINK\n");
        Instruction pi = *(pc - 2);  /* original arith. expression */
        TValue *imm = KB(i);
        TMS tm = (TMS)GETARG_C(i);
        int flip = GETARG_k(i);
        StkId result = RA(pi);
        Protect(luaT_trybinassocTM(L, s2v(ra), imm, flip, result, tm));
        vmbreak;
      }
      VM_CASE(UNM) {  printf("UNM\n");
        TValue *rb = vRB(i);
        lua_Number nb;
        if (ttisinteger(rb)) {
          lua_Integer ib = ivalue(rb);
          setivalue(s2v(ra), intop(-, 0, ib));
        }
        else if (tonumberns(rb, nb)) {
          setfltvalue(s2v(ra), luai_numunm(L, nb));
        }
        else
          Protect(luaT_trybinTM(L, rb, rb, ra, TM_UNM));
        vmbreak;
      }
      VM_CASE(BNOT) { printf("BNOT\n");
        TValue *rb = vRB(i);
        lua_Integer ib;
        if (tointegerns(rb, &ib)) {
          setivalue(s2v(ra), intop(^, ~l_castS2U(0), ib));
        }
        else
          Protect(luaT_trybinTM(L, rb, rb, ra, TM_BNOT));
        vmbreak;
      }
      VM_CASE(NOT) { printf("NOT\n");
        TValue *rb = vRB(i);
        if (l_isfalse(rb))
          setbtvalue(s2v(ra));
        else
          setbfvalue(s2v(ra));
        vmbreak;
      }
      VM_CASE(LEN) { printf("LEN\n");
        Protect(luaV_objlen(L, ra, vRB(i)));
        vmbreak;
      }
      VM_CASE(CONCAT) { printf("CONCAT\n");
        int n = GETARG_B(i);  /* number of elements to concatenate */
        L->top = ra + n;  /* mark the end of concat operands */
        ProtectNT(luaV_concat(L, n));
        checkGC(L, L->top); /* 'luaV_concat' ensures correct top */
        vmbreak;
      }
      VM_CASE(CLOSE) { printf("CLOSE\n");
        Protect(luaF_close(L, ra, LUA_OK, 1));
        vmbreak;
      }
      VM_CASE(TBC) { printf("TBC\n");
        /* create new to-be-closed upvalue */
        halfProtect(luaF_newtbcupval(L, ra));
        vmbreak;
      }
      VM_CASE(JMP) { printf("JMP\n");
        dojump(ci, i, 0);
        vmbreak;
      }
      VM_CASE(EQ) { printf("EQ\n");
        int cond;
        TValue *rb = vRB(i);
        Protect(cond = luaV_equalobj(L, s2v(ra), rb));
        docondjump();
        vmbreak;
      }
      VM_CASE(LT) { printf("LT\n");
        op_order(L, l_lti, LTnum, lessthanothers);
        vmbreak;
      }
      VM_CASE(LE) { printf("LE\n");
        op_order(L, l_lei, LEnum, lessequalothers);
        vmbreak;
      }
      VM_CASE(EQK) { printf("EQK\n");
        TValue *rb = KB(i);
        /* basic types do not use '__eq'; we can use raw equality */
        int cond = luaV_rawequalobj(s2v(ra), rb);
        docondjump();
        vmbreak;
      }
      VM_CASE(EQI) { printf("EQI\n");
        int cond;
        int im = GETARG_sB(i);
        if (ttisinteger(s2v(ra)))
          cond = (ivalue(s2v(ra)) == im);
        else if (ttisfloat(s2v(ra)))
          cond = luai_numeq(fltvalue(s2v(ra)), cast_num(im));
        else
          cond = 0;  /* other types cannot be equal to a number */
        docondjump();
        vmbreak;
      }
      VM_CASE(LTI) { printf("LTI\n");
        op_orderI(L, l_lti, luai_numlt, 0, TM_LT);
        vmbreak;
      }
      VM_CASE(LEI) { printf("LEI\n");
        op_orderI(L, l_lei, luai_numle, 0, TM_LE);
        vmbreak;
      }
      VM_CASE(GTI) { printf("GTI\n");
        op_orderI(L, l_gti, luai_numgt, 1, TM_LT);
        vmbreak;
      }
      VM_CASE(GEI) { printf("GEI\n");
        op_orderI(L, l_gei, luai_numge, 1, TM_LE);
        vmbreak;
      }
      VM_CASE(TEST) { printf("TEST\n");
        int cond = !l_isfalse(s2v(ra));
        docondjump();
        vmbreak;
      }
      VM_CASE(TESTSET) { printf("TESTSET\n");
        TValue *rb = vRB(i);
        if (l_isfalse(rb) == GETARG_k(i))
          pc++;
        else {
          setobj2s(L, ra, rb);
          donextjump(ci);
        }
        vmbreak;
      }
      VM_CASE(CALL) { printf("CALL\n");
        CallInfo *newci;
        int b = GETARG_B(i);
        int nresults = GETARG_C(i) - 1;
        if (b != 0)  /* fixed number of arguments? */
          L->top = ra + b;  /* top signals number of arguments */
        /* else previous instruction set top */
        savepc(L);  /* in case of errors */
        if ((newci = luaD_precall(L, ra, nresults)) == NULL)
          updatetrap(ci);  /* C call; nothing else to be done */
        else {  /* Lua call: run function in this same C frame */
          ci = newci;
          ci->callstatus = 0;  /* call re-uses 'luaV_execute' */
          goto startfunc;
        }
        vmbreak;
      }
      VM_CASE(TAILCALL) { printf("TAILCALL\n");
        int b = GETARG_B(i);  /* number of arguments + 1 (function) */
        int nparams1 = GETARG_C(i);
        /* delta is virtual 'func' - real 'func' (vararg functions) */
        int delta = (nparams1) ? ci->u.l.nextraargs + nparams1 : 0;
        if (b != 0)
          L->top = ra + b;
        else  /* previous instruction set top */
          b = cast_int(L->top - ra);
        savepc(ci);  /* several calls here can raise errors */
        if (TESTARG_k(i)) {
          luaF_closeupval(L, base);  /* close upvalues from current call */
          lua_assert(L->tbclist < base);  /* no pending tbc variables */
          lua_assert(base == ci->func + 1);
        }
        while (!ttisfunction(s2v(ra))) {  /* not a function? */
          luaD_tryfuncTM(L, ra);  /* try '__call' metamethod */
          b++;  /* there is now one extra argument */
          checkstackGCp(L, 1, ra);
        }
        if (!ttisLclosure(s2v(ra))) {  /* C function? */
          luaD_precall(L, ra, LUA_MULTRET);  /* call it */
          updatetrap(ci);
          updatestack(ci);  /* stack may have been relocated */
          ci->func -= delta;  /* restore 'func' (if vararg) */
          luaD_poscall(L, ci, cast_int(L->top - ra));  /* finish caller */
          updatetrap(ci);  /* 'luaD_poscall' can change hooks */
          goto ret;  /* caller returns after the tail call */
        }
        ci->func -= delta;  /* restore 'func' (if vararg) */
        luaD_pretailcall(L, ci, ra, b, 0);  /* prepare call frame */

        goto startfunc;  /* execute the callee */
      }
      VM_CASE(RETURN) { printf("RETURN\n");
        int n = GETARG_B(i) - 1;  /* number of results */
        int nparams1 = GETARG_C(i);
        if (n < 0)  /* not fixed? */
          n = cast_int(L->top - ra);  /* get what is available */
        savepc(ci);
        if (TESTARG_k(i)) {  /* may there be open upvalues? */
          if (L->top < ci->top)
            L->top = ci->top;
          luaF_close(L, base, CLOSEKTOP, 1);
          updatetrap(ci);
          updatestack(ci);
        }
        if (nparams1)  /* vararg function? */
          ci->func -= ci->u.l.nextraargs + nparams1;
        L->top = ra + n;  /* set call for 'luaD_poscall' */
        luaD_poscall(L, ci, n);
        updatetrap(ci);  /* 'luaD_poscall' can change hooks */
        goto ret;
      }
      VM_CASE(RETURN0) { printf("RETURN0\n");
        if (l_unlikely(L->hookmask)) {
          L->top = ra;
          savepc(ci);
          luaD_poscall(L, ci, 0);  /* no hurry... */
          trap = 1;
        }
        else {  /* do the 'poscall' here */
          int nres;
          L->ci = ci->previous;  /* back to caller */
          L->top = base - 1;
          for (nres = ci->nresults; l_unlikely(nres > 0); nres--)
            setnilvalue(s2v(L->top++));  /* all results are nil */
        }
        goto ret;
      }
      VM_CASE(RETURN1) { printf("RETURN1\n");
        if (l_unlikely(L->hookmask)) {
          L->top = ra + 1;
          savepc(ci);
          luaD_poscall(L, ci, 1);  /* no hurry... */
          trap = 1;
        }
        else {  /* do the 'poscall' here */
          int nres = ci->nresults;
          L->ci = ci->previous;  /* back to caller */
          if (nres == 0)
            L->top = base - 1;  /* asked for no results */
          else {
            setobjs2s(L, base - 1, ra);  /* at least this result */
            L->top = base;
            for (; l_unlikely(nres > 1); nres--)
              setnilvalue(s2v(L->top++));  /* complete missing results */
          }
        }
       ret:  /* return from a Lua function */
        if (ci->callstatus & CIST_FRESH)
          return NULL;  /* end this frame */
        else {
          ci = ci->previous;
          goto returning;  /* continue running caller in this frame */
        }
      }
      VM_CASE(FORLOOP) { printf("FORLOOP\n");
        if (ttisinteger(s2v(ra + 2))) {  /* integer loop? */
          lua_Unsigned count = l_castS2U(ivalue(s2v(ra + 1)));
          if (count > 0) {  /* still more iterations? */
            lua_Integer step = ivalue(s2v(ra + 2));
            lua_Integer idx = ivalue(s2v(ra));  /* internal index */
            chgivalue(s2v(ra + 1), count - 1);  /* update counter */
            idx = intop(+, idx, step);  /* add step to index */
            chgivalue(s2v(ra), idx);  /* update internal index */
            setivalue(s2v(ra + 3), idx);  /* and control variable */
            pc -= GETARG_Bx(i);  /* jump back */
          }
        }
        else if (floatforloop(ra))  /* float loop */
          pc -= GETARG_Bx(i);  /* jump back */
        updatetrap(ci);  /* allows a signal to break the loop */
        vmbreak;
      }
      VM_CASE(FORPREP) { printf("FORPREP\n");
        savestate(L, ci);  /* in case of errors */
        if (forprep(L, ra))
          pc += GETARG_Bx(i) + 1;  /* skip the loop */
        vmbreak;
      }
      VM_CASE(TFORPREP) { printf("TFORPREP\n");
        /* create to-be-closed upvalue (if needed) */
        halfProtect(luaF_newtbcupval(L, ra + 3));
        pc += GETARG_Bx(i);
        i = *(pc++);  /* go to next instruction */
        lua_assert(GET_OPCODE(i) == OP_TFORCALL && ra == RA(i));
        goto l_tforcall;
      }
      VM_CASE(TFORCALL) { printf("TFORCALL\n");
       l_tforcall:
        /* 'ra' has the iterator function, 'ra + 1' has the state,
           'ra + 2' has the control variable, and 'ra + 3' has the
           to-be-closed variable. The call will use the stack after
           these values (starting at 'ra + 4')
        */
        /* push function, state, and control variable */
        memcpy(ra + 4, ra, 3 * sizeof(*ra));
        L->top = ra + 4 + 3;
        ProtectNT(luaD_call(L, ra + 4, GETARG_C(i)));  /* do the call */
        updatestack(ci);  /* stack may have changed */
        i = *(pc++);  /* go to next instruction */
        lua_assert(GET_OPCODE(i) == OP_TFORLOOP && ra == RA(i));
        goto l_tforloop;
      }
      VM_CASE(TFORLOOP) { printf("TFORLOOP\n");
        l_tforloop:
        if (!ttisnil(s2v(ra + 4))) {  /* continue loop? */
          setobjs2s(L, ra + 2, ra + 4);  /* save control variable */
          pc -= GETARG_Bx(i);  /* jump back */
        }
        vmbreak;
      }
      VM_CASE(SETLIST) { printf("SETLIST\n");
        int n = GETARG_B(i);
        unsigned int last = GETARG_C(i);
        Table *h = hvalue(s2v(ra));
        if (n == 0)
          n = cast_int(L->top - ra) - 1;  /* get up to the top */
        else
          L->top = ci->top;  /* correct top in case of emergency GC */
        last += n;
        if (TESTARG_k(i)) { 
          last += GETARG_Ax(*pc) * (MAXARG_C + 1);
          pc++;
        }
        if (last > luaH_realasize(h))  /* needs more space? */
          luaH_resizearray(L, h, last);  /* preallocate it at once */
        for (; n > 0; n--) {
          TValue *val = s2v(ra + n);
          setobj2t(L, &h->array[last - 1], val);
          last--;
          luaC_barrierback(L, obj2gco(h), val);
        }
        vmbreak;
      }
      VM_CASE(CLOSURE) { printf("CLOSURE\n");
        Proto *p = cl->p->p[GETARG_Bx(i)];
        halfProtect(pushclosure(L, p, cl->upvals, base, ra));
        checkGC(L, ra + 1);
        vmbreak;
      }
      VM_CASE(VARARG) { printf("VARARG\n");
        int n = GETARG_C(i) - 1;  /* required results */
        Protect(luaT_getvarargs(L, ci, ra, n));
        vmbreak;
      }
      VM_CASE(VARARGPREP) { printf("VARARGPREP\n");
        ProtectNT(luaT_adjustvarargs(L, GETARG_A(i), ci, cl->p));
        if (l_unlikely(trap)) {  /* previous "Protect" updated trap */
          luaD_hookcall(L, ci);
          L->oldpc = 1;  /* next opcode will be seen as a "new" line */
        }
        updatebase(ci);  /* function has new base after adjustment */
        vmbreak;
      }
      VM_CASE(EXTRAARG) { printf("EXTRAARG\n");
        lua_assert(0);
        vmbreak;
      }
    }
    } else {
    #endif
      // Interpreted
      vmdispatch (GET_OPCODE(i)) {
        VM_CASE(MOVE) {
          setobjs2s(L, ra, RB(i));
          vmbreak;
        }
        VM_CASE(LOADI) {
          lua_Integer b = GETARG_sBx(i);
          setivalue(s2v(ra), b);
          vmbreak;
        }
        VM_CASE(LOADF) {
          int b = GETARG_sBx(i);
          setfltvalue(s2v(ra), cast_num(b));
          vmbreak;
        }
        VM_CASE(LOADK) {
          TValue *rb = k + GETARG_Bx(i);
          setobj2s(L, ra, rb);
          vmbreak;
        }
        VM_CASE(LOADKX) {
          TValue *rb;
          rb = k + GETARG_Ax(*pc); pc++;
          setobj2s(L, ra, rb);
          vmbreak;
        }
        VM_CASE(LOADFALSE) {
          setbfvalue(s2v(ra));
          vmbreak;
        }
        VM_CASE(LFALSESKIP) {
          setbfvalue(s2v(ra));
          pc++;  /* skip next instruction */
          vmbreak;
        }
        VM_CASE(LOADTRUE) {
          setbtvalue(s2v(ra));
          vmbreak;
        }
        VM_CASE(LOADNIL) {
          int b = GETARG_B(i);
          do {
            setnilvalue(s2v(ra++));
          } while (b--);
          vmbreak;
        }
        VM_CASE(GETUPVAL) {
          int b = GETARG_B(i);
          setobj2s(L, ra, cl->upvals[b]->v);
          vmbreak;
        }
        VM_CASE(SETUPVAL) {
          UpVal *uv = cl->upvals[GETARG_B(i)];
          setobj(L, uv->v, s2v(ra));
          luaC_barrier(L, uv, s2v(ra));
          vmbreak;
        }
        VM_CASE(GETTABUP) {
          const TValue *slot;
          TValue *upval = cl->upvals[GETARG_B(i)]->v;
          TValue *rc = KC(i);
          TString *key = tsvalue(rc);  /* key must be a string */
          if (luaV_fastget(L, upval, key, slot, luaH_getshortstr)) {
            setobj2s(L, ra, slot);
          }
          else
            Protect(luaV_finishget(L, upval, rc, ra, slot));
          vmbreak;
        }
        VM_CASE(GETTABLE) {
          const TValue *slot;
          TValue *rb = vRB(i);
          TValue *rc = vRC(i);
          lua_Unsigned n;
          if (ttisinteger(rc)  /* fast track for integers? */
              ? (cast_void(n = ivalue(rc)), luaV_fastgeti(L, rb, n, slot))
              : luaV_fastget(L, rb, rc, slot, luaH_get)) {
            setobj2s(L, ra, slot);
          }
          else
            Protect(luaV_finishget(L, rb, rc, ra, slot));
          vmbreak;
        }
        VM_CASE(GETI) {
          const TValue *slot;
          TValue *rb = vRB(i);
          int c = GETARG_C(i);
          if (luaV_fastgeti(L, rb, c, slot)) {
            setobj2s(L, ra, slot);
          }
          else {
            TValue key;
            setivalue(&key, c);
            Protect(luaV_finishget(L, rb, &key, ra, slot));
          }
          vmbreak;
        }
        VM_CASE(GETFIELD) {
          const TValue *slot;
          TValue *rb = vRB(i);
          TValue *rc = KC(i);
          TString *key = tsvalue(rc);  /* key must be a string */
          if (luaV_fastget(L, rb, key, slot, luaH_getshortstr)) {
            setobj2s(L, ra, slot);
          }
          else
            Protect(luaV_finishget(L, rb, rc, ra, slot));
          vmbreak;
        }
        VM_CASE(SETTABUP) {
          const TValue *slot;
          TValue *upval = cl->upvals[GETARG_A(i)]->v;
          TValue *rb = KB(i);
          TValue *rc = RKC(i);
          TString *key = tsvalue(rb);  /* key must be a string */
          if (luaV_fastget(L, upval, key, slot, luaH_getshortstr)) {
            luaV_finishfastset(L, upval, slot, rc);
          }
          else
            Protect(luaV_finishset(L, upval, rb, rc, slot));
          vmbreak;
        }
        VM_CASE(SETTABLE) {
          const TValue *slot;
          TValue *rb = vRB(i);  /* key (table is in 'ra') */
          TValue *rc = RKC(i);  /* value */
          lua_Unsigned n;
          if (ttisinteger(rb)  /* fast track for integers? */
              ? (cast_void(n = ivalue(rb)), luaV_fastgeti(L, s2v(ra), n, slot))
              : luaV_fastget(L, s2v(ra), rb, slot, luaH_get)) {
            luaV_finishfastset(L, s2v(ra), slot, rc);
          }
          else
            Protect(luaV_finishset(L, s2v(ra), rb, rc, slot));
          vmbreak;
        }
        VM_CASE(SETI) {
          const TValue *slot;
          int c = GETARG_B(i);
          TValue *rc = RKC(i);
          if (luaV_fastgeti(L, s2v(ra), c, slot)) {
            luaV_finishfastset(L, s2v(ra), slot, rc);
          }
          else {
            TValue key;
            setivalue(&key, c);
            Protect(luaV_finishset(L, s2v(ra), &key, rc, slot));
          }
          vmbreak;
        }
        VM_CASE(SETFIELD) {
          const TValue *slot;
          TValue *rb = KB(i);
          TValue *rc = RKC(i);
          TString *key = tsvalue(rb);  /* key must be a string */
          if (luaV_fastget(L, s2v(ra), key, slot, luaH_getshortstr)) {
            luaV_finishfastset(L, s2v(ra), slot, rc);
          }
          else
            Protect(luaV_finishset(L, s2v(ra), rb, rc, slot));
          vmbreak;
        }
        VM_CASE(NEWTABLE) {
          int b = GETARG_B(i);  /* log2(hash size) + 1 */
          int c = GETARG_C(i);  /* array size */
          Table *t;
          if (b > 0)
            b = 1 << (b - 1);  /* size is 2^(b - 1) */
          lua_assert((!TESTARG_k(i)) == (GETARG_Ax(*pc) == 0));
          if (TESTARG_k(i))  /* non-zero extra argument? */
            c += GETARG_Ax(*pc) * (MAXARG_C + 1);  /* add it to size */
          pc++;  /* skip extra argument */
          L->top = ra + 1;  /* correct top in case of emergency GC */
          t = luaH_new(L);  /* memory allocation */
          sethvalue2s(L, ra, t);
          if (b != 0 || c != 0)
            luaH_resize(L, t, c, b);  /* idem */
          checkGC(L, ra + 1);
          vmbreak;
        }
        VM_CASE(SELF) {
          const TValue *slot;
          TValue *rb = vRB(i);
          TValue *rc = RKC(i);
          TString *key = tsvalue(rc);  /* key must be a string */
          setobj2s(L, ra + 1, rb);
          if (luaV_fastget(L, rb, key, slot, luaH_getstr)) {
            setobj2s(L, ra, slot);
          }
          else
            Protect(luaV_finishget(L, rb, rc, ra, slot));
          vmbreak;
        }
        VM_CASE(ADDI) {
          op_arithI(L, l_addi, luai_numadd);
          vmbreak;
        }
        VM_CASE(ADDK) {
          op_arithK(L, l_addi, luai_numadd);
          vmbreak;
        }
        VM_CASE(SUBK) {
          op_arithK(L, l_subi, luai_numsub);
          vmbreak;
        }
        VM_CASE(MULK) {
          op_arithK(L, l_muli, luai_nummul);
          vmbreak;
        }
        VM_CASE(MODK) {
          op_arithK(L, luaV_mod, luaV_modf);
          vmbreak;
        }
        VM_CASE(POWK) {
          op_arithfK(L, luai_numpow);
          vmbreak;
        }
        VM_CASE(DIVK) {
          op_arithfK(L, luai_numdiv);
          vmbreak;
        }
        VM_CASE(IDIVK) {
          op_arithK(L, luaV_idiv, luai_numidiv);
          vmbreak;
        }
        VM_CASE(BANDK) {
          op_bitwiseK(L, l_band);
          vmbreak;
        }
        VM_CASE(BORK) {
          op_bitwiseK(L, l_bor);
          vmbreak;
        }
        VM_CASE(BXORK) {
          op_bitwiseK(L, l_bxor);
          vmbreak;
        }
        VM_CASE(SHRI) {
          TValue *rb = vRB(i);
          int ic = GETARG_sC(i);
          lua_Integer ib;
          if (tointegerns(rb, &ib)) {
            pc++; setivalue(s2v(ra), luaV_shiftl(ib, -ic));
          }
          vmbreak;
        }
        VM_CASE(SHLI) {
          TValue *rb = vRB(i);
          int ic = GETARG_sC(i);
          lua_Integer ib;
          if (tointegerns(rb, &ib)) {
            pc++; setivalue(s2v(ra), luaV_shiftl(ic, ib));
          }
          vmbreak;
        }
        VM_CASE(ADD) {
          op_arith(L, l_addi, luai_numadd);
          vmbreak;
        }
        VM_CASE(SUB) {
          op_arith(L, l_subi, luai_numsub);
          vmbreak;
        }
        VM_CASE(MUL) {
          op_arith(L, l_muli, luai_nummul);
          vmbreak;
        }
        VM_CASE(MOD) {
          op_arith(L, luaV_mod, luaV_modf);
          vmbreak;
        }
        VM_CASE(POW) {
          op_arithf(L, luai_numpow);
          vmbreak;
        }
        VM_CASE(DIV) {  /* float division (always with floats) */
          op_arithf(L, luai_numdiv);
          vmbreak;
        }
        VM_CASE(IDIV) {  /* floor division */
          op_arith(L, luaV_idiv, luai_numidiv);
          vmbreak;
        }
        VM_CASE(BAND) {
          op_bitwise(L, l_band);
          vmbreak;
        }
        VM_CASE(BOR) {
          op_bitwise(L, l_bor);
          vmbreak;
        }
        VM_CASE(BXOR) {
          op_bitwise(L, l_bxor);
          vmbreak;
        }
        VM_CASE(SHR) {
          op_bitwise(L, luaV_shiftr);
          vmbreak;
        }
        VM_CASE(SHL) {
          op_bitwise(L, luaV_shiftl);
          vmbreak;
        }
        VM_CASE(MMBIN) {
          Instruction pi = *(pc - 2);  /* original arith. expression */
          TValue *rb = vRB(i);
          TMS tm = (TMS)GETARG_C(i);
          StkId result = RA(pi);
          lua_assert(OP_ADD <= GET_OPCODE(pi) && GET_OPCODE(pi) <= OP_SHR);
          Protect(luaT_trybinTM(L, s2v(ra), rb, result, tm));
          vmbreak;
        }
        VM_CASE(MMBINI) {
          Instruction pi = *(pc - 2);  /* original arith. expression */
          int imm = GETARG_sB(i);
          TMS tm = (TMS)GETARG_C(i);
          int flip = GETARG_k(i);
          StkId result = RA(pi);
          Protect(luaT_trybiniTM(L, s2v(ra), imm, flip, result, tm));
          vmbreak;
        }
        VM_CASE(MMBINK) {
          Instruction pi = *(pc - 2);  /* original arith. expression */
          TValue *imm = KB(i);
          TMS tm = (TMS)GETARG_C(i);
          int flip = GETARG_k(i);
          StkId result = RA(pi);
          Protect(luaT_trybinassocTM(L, s2v(ra), imm, flip, result, tm));
          vmbreak;
        }
        VM_CASE(UNM) {
          TValue *rb = vRB(i);
          lua_Number nb;
          if (ttisinteger(rb)) {
            lua_Integer ib = ivalue(rb);
            setivalue(s2v(ra), intop(-, 0, ib));
          }
          else if (tonumberns(rb, nb)) {
            setfltvalue(s2v(ra), luai_numunm(L, nb));
          }
          else
            Protect(luaT_trybinTM(L, rb, rb, ra, TM_UNM));
          vmbreak;
        }
        VM_CASE(BNOT) {
          TValue *rb = vRB(i);
          lua_Integer ib;
          if (tointegerns(rb, &ib)) {
            setivalue(s2v(ra), intop(^, ~l_castS2U(0), ib));
          }
          else
            Protect(luaT_trybinTM(L, rb, rb, ra, TM_BNOT));
          vmbreak;
        }
        VM_CASE(NOT) {
          TValue *rb = vRB(i);
          if (l_isfalse(rb))
            setbtvalue(s2v(ra));
          else
            setbfvalue(s2v(ra));
          vmbreak;
        }
        VM_CASE(LEN) {
          Protect(luaV_objlen(L, ra, vRB(i)));
          vmbreak;
        }
        VM_CASE(CONCAT) {
          int n = GETARG_B(i);  /* number of elements to concatenate */
          L->top = ra + n;  /* mark the end of concat operands */
          ProtectNT(luaV_concat(L, n));
          checkGC(L, L->top); /* 'luaV_concat' ensures correct top */
          vmbreak;
        }
        VM_CASE(CLOSE) {
          Protect(luaF_close(L, ra, LUA_OK, 1));
          vmbreak;
        }
        VM_CASE(TBC) {
          /* create new to-be-closed upvalue */
          halfProtect(luaF_newtbcupval(L, ra));
          vmbreak;
        }
        VM_CASE(JMP) {
          dojump(ci, i, 0);
          vmbreak;
        }
        VM_CASE(EQ) {
          int cond;
          TValue *rb = vRB(i);
          Protect(cond = luaV_equalobj(L, s2v(ra), rb));
          docondjump();
          vmbreak;
        }
        VM_CASE(LT) {
          op_order(L, l_lti, LTnum, lessthanothers);
          vmbreak;
        }
        VM_CASE(LE) {
          op_order(L, l_lei, LEnum, lessequalothers);
          vmbreak;
        }
        VM_CASE(EQK) {
          TValue *rb = KB(i);
          /* basic types do not use '__eq'; we can use raw equality */
          int cond = luaV_rawequalobj(s2v(ra), rb);
          docondjump();
          vmbreak;
        }
        VM_CASE(EQI) {
          int cond;
          int im = GETARG_sB(i);
          if (ttisinteger(s2v(ra)))
            cond = (ivalue(s2v(ra)) == im);
          else if (ttisfloat(s2v(ra)))
            cond = luai_numeq(fltvalue(s2v(ra)), cast_num(im));
          else
            cond = 0;  /* other types cannot be equal to a number */
          docondjump();
          vmbreak;
        }
        VM_CASE(LTI) {
          op_orderI(L, l_lti, luai_numlt, 0, TM_LT);
          vmbreak;
        }
        VM_CASE(LEI) {
          op_orderI(L, l_lei, luai_numle, 0, TM_LE);
          vmbreak;
        }
        VM_CASE(GTI) {
          op_orderI(L, l_gti, luai_numgt, 1, TM_LT);
          vmbreak;
        }
        VM_CASE(GEI) {
          op_orderI(L, l_gei, luai_numge, 1, TM_LE);
          vmbreak;
        }
        VM_CASE(TEST) {
          int cond = !l_isfalse(s2v(ra));
          docondjump();
          vmbreak;
        }
        VM_CASE(TESTSET) {
          TValue *rb = vRB(i);
          if (l_isfalse(rb) == GETARG_k(i))
            pc++;
          else {
            setobj2s(L, ra, rb);
            donextjump(ci);
          }
          vmbreak;
        }
        VM_CASE(CALL) {
          CallInfo *newci;
          int b = GETARG_B(i);
          int nresults = GETARG_C(i) - 1;
          if (b != 0)  /* fixed number of arguments? */
            L->top = ra + b;  /* top signals number of arguments */
          /* else previous instruction set top */
          savepc(L);  /* in case of errors */
          if ((newci = luaD_precall(L, ra, nresults)) == NULL)
            updatetrap(ci);  /* C call; nothing else to be done */
          else {  /* Lua call: run function in this same C frame */
            ci = newci;
            ci->callstatus = 0;  /* call re-uses 'luaV_execute' */
            goto startfunc;
          }
          vmbreak;
        }
        VM_CASE(TAILCALL) {
          int b = GETARG_B(i);  /* number of arguments + 1 (function) */
          int nparams1 = GETARG_C(i);
          /* delta is virtual 'func' - real 'func' (vararg functions) */
          int delta = (nparams1) ? ci->u.l.nextraargs + nparams1 : 0;
          if (b != 0)
            L->top = ra + b;
          else  /* previous instruction set top */
            b = cast_int(L->top - ra);
          savepc(ci);  /* several calls here can raise errors */
          if (TESTARG_k(i)) {
            luaF_closeupval(L, base);  /* close upvalues from current call */
            lua_assert(L->tbclist < base);  /* no pending tbc variables */
            lua_assert(base == ci->func + 1);
          }
          while (!ttisfunction(s2v(ra))) {  /* not a function? */
            luaD_tryfuncTM(L, ra);  /* try '__call' metamethod */
            b++;  /* there is now one extra argument */
            checkstackGCp(L, 1, ra);
          }
          if (!ttisLclosure(s2v(ra))) {  /* C function? */
            luaD_precall(L, ra, LUA_MULTRET);  /* call it */
            updatetrap(ci);
            updatestack(ci);  /* stack may have been relocated */
            ci->func -= delta;  /* restore 'func' (if vararg) */
            luaD_poscall(L, ci, cast_int(L->top - ra));  /* finish caller */
            updatetrap(ci);  /* 'luaD_poscall' can change hooks */
            goto ret;  /* caller returns after the tail call */
          }
          ci->func -= delta;  /* restore 'func' (if vararg) */
          luaD_pretailcall(L, ci, ra, b, 0);  /* prepare call frame */

          goto startfunc;  /* execute the callee */
        }
        VM_CASE(RETURN) {
          int n = GETARG_B(i) - 1;  /* number of results */
          int nparams1 = GETARG_C(i);
          if (n < 0)  /* not fixed? */
            n = cast_int(L->top - ra);  /* get what is available */
          savepc(ci);
          if (TESTARG_k(i)) {  /* may there be open upvalues? */
            if (L->top < ci->top)
              L->top = ci->top;
            luaF_close(L, base, CLOSEKTOP, 1);
            updatetrap(ci);
            updatestack(ci);
          }
          if (nparams1)  /* vararg function? */
            ci->func -= ci->u.l.nextraargs + nparams1;
          L->top = ra + n;  /* set call for 'luaD_poscall' */
          luaD_poscall(L, ci, n);
          updatetrap(ci);  /* 'luaD_poscall' can change hooks */
          goto ret;
        }
        VM_CASE(RETURN0) {
          if (l_unlikely(L->hookmask)) {
            L->top = ra;
            savepc(ci);
            luaD_poscall(L, ci, 0);  /* no hurry... */
            trap = 1;
          }
          else {  /* do the 'poscall' here */
            int nres;
            L->ci = ci->previous;  /* back to caller */
            L->top = base - 1;
            for (nres = ci->nresults; l_unlikely(nres > 0); nres--)
              setnilvalue(s2v(L->top++));  /* all results are nil */
          }
          goto ret;
        }
        VM_CASE(RETURN1) {
          if (l_unlikely(L->hookmask)) {
            L->top = ra + 1;
            savepc(ci);
            luaD_poscall(L, ci, 1);  /* no hurry... */
            trap = 1;
          }
          else {  /* do the 'poscall' here */
            int nres = ci->nresults;
            L->ci = ci->previous;  /* back to caller */
            if (nres == 0)
              L->top = base - 1;  /* asked for no results */
            else {
              setobjs2s(L, base - 1, ra);  /* at least this result */
              L->top = base;
              for (; l_unlikely(nres > 1); nres--)
                setnilvalue(s2v(L->top++));  /* complete missing results */
            }
          }
        ret:  /* return from a Lua function */
          if (ci->callstatus & CIST_FRESH)
            return NULL;  /* end this frame */
          else {
            ci = ci->previous;
            goto returning;  /* continue running caller in this frame */
          }
        }
        VM_CASE(FORLOOP) {
          if (ttisinteger(s2v(ra + 2))) {  /* integer loop? */
            lua_Unsigned count = l_castS2U(ivalue(s2v(ra + 1)));
            if (count > 0) {  /* still more iterations? */
              lua_Integer step = ivalue(s2v(ra + 2));
              lua_Integer idx = ivalue(s2v(ra));  /* internal index */
              chgivalue(s2v(ra + 1), count - 1);  /* update counter */
              idx = intop(+, idx, step);  /* add step to index */
              chgivalue(s2v(ra), idx);  /* update internal index */
              setivalue(s2v(ra + 3), idx);  /* and control variable */
              pc -= GETARG_Bx(i);  /* jump back */
            }
          }
          else if (floatforloop(ra))  /* float loop */
            pc -= GETARG_Bx(i);  /* jump back */
          updatetrap(ci);  /* allows a signal to break the loop */
          vmbreak;
        }
        VM_CASE(FORPREP) {
          savestate(L, ci);  /* in case of errors */
          if (forprep(L, ra))
            pc += GETARG_Bx(i) + 1;  /* skip the loop */
          vmbreak;
        }
        VM_CASE(TFORPREP) {
          /* create to-be-closed upvalue (if needed) */
          halfProtect(luaF_newtbcupval(L, ra + 3));
          pc += GETARG_Bx(i);
          i = *(pc++);  /* go to next instruction */
          lua_assert(GET_OPCODE(i) == OP_TFORCALL && ra == RA(i));
          goto l_tforcall;
        }
        VM_CASE(TFORCALL) {
        l_tforcall:
          /* 'ra' has the iterator function, 'ra + 1' has the state,
            'ra + 2' has the control variable, and 'ra + 3' has the
            to-be-closed variable. The call will use the stack after
            these values (starting at 'ra + 4')
          */
          /* push function, state, and control variable */
          memcpy(ra + 4, ra, 3 * sizeof(*ra));
          L->top = ra + 4 + 3;
          ProtectNT(luaD_call(L, ra + 4, GETARG_C(i)));  /* do the call */
          updatestack(ci);  /* stack may have changed */
          i = *(pc++);  /* go to next instruction */
          lua_assert(GET_OPCODE(i) == OP_TFORLOOP && ra == RA(i));
          goto l_tforloop;
        }
        VM_CASE(TFORLOOP) {
          l_tforloop:
          if (!ttisnil(s2v(ra + 4))) {  /* continue loop? */
            setobjs2s(L, ra + 2, ra + 4);  /* save control variable */
            pc -= GETARG_Bx(i);  /* jump back */
          }
          vmbreak;
        }
        VM_CASE(SETLIST) {
          int n = GETARG_B(i);
          unsigned int last = GETARG_C(i);
          Table *h = hvalue(s2v(ra));
          if (n == 0)
            n = cast_int(L->top - ra) - 1;  /* get up to the top */
          else
            L->top = ci->top;  /* correct top in case of emergency GC */
          last += n;
          if (TESTARG_k(i)) {
            last += GETARG_Ax(*pc) * (MAXARG_C + 1);
            pc++;
          }
          if (last > luaH_realasize(h))  /* needs more space? */
            luaH_resizearray(L, h, last);  /* preallocate it at once */
          for (; n > 0; n--) {
            TValue *val = s2v(ra + n);
            setobj2t(L, &h->array[last - 1], val);
            last--;
            luaC_barrierback(L, obj2gco(h), val);
          }
          vmbreak;
        }
        VM_CASE(CLOSURE) {
          Proto *p = cl->p->p[GETARG_Bx(i)];
          halfProtect(pushclosure(L, p, cl->upvals, base, ra));
          checkGC(L, ra + 1);
          vmbreak;
        }
        VM_CASE(VARARG) {
          int n = GETARG_C(i) - 1;  /* required results */
          Protect(luaT_getvarargs(L, ci, ra, n));
          vmbreak;
        }
        VM_CASE(VARARGPREP) {
          ProtectNT(luaT_adjustvarargs(L, GETARG_A(i), ci, cl->p));
          if (l_unlikely(trap)) {  /* previous "Protect" updated trap */
            luaD_hookcall(L, ci);
            L->oldpc = 1;  /* next opcode will be seen as a "new" line */
          }
          updatebase(ci);  /* function has new base after adjustment */
          vmbreak;
        }
        VM_CASE(EXTRAARG) {
          lua_assert(0);
          vmbreak;
        }
      }
    }

    #ifdef JIT
    }
    #endif
}

// AOT Wrapper
#ifndef AOT_IS_MODULE
void luaV_execute (lua_State *L, CallInfo *ci) {
    do {
        LClosure *cl = clLvalue(s2v(ci->func));
        if (cl->p->aot_implementation) {
            ci = cl->p->aot_implementation(L, ci);
        } else {
            ci = luaV_execute_(L, ci);
        }
    } while (ci);
}
#endif
