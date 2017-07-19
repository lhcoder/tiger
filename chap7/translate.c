#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "temp.h"
#include "absyn.h"
#include "frame.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"

#define DB 0

/***************** frame ***********************/
struct Tr_level_ {
	Tr_level parent;
	Temp_label name;
	F_frame frame;
	Tr_accessList formals;
};

struct Tr_access_ {
	Tr_level level;
	F_access access;
};

static Tr_level outer = NULL;

static Tr_access Tr_Access(Tr_level level, F_access access);
static Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
static Tr_accessList makeFormalList(Tr_level l);   //trans F_accessList to Tr_accessList


/***************** IR tree ***********************/
typedef struct patchList_ *patchList;
struct patchList_ {Temp_label *head;  patchList tail;};

struct Cx {patchList trues, falses;  T_stm stm;};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx;} u;
};

static patchList PatchList(Temp_label *head, patchList tail);
static patchList doPatch(patchList tList, Temp_label label);
static patchList joinPatch(patchList first, patchList second);

static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

static T_exp trackLink(Tr_level l, Tr_level g);		//track static link from l to g
static T_expList Tr_transExpList(Tr_expList l);




/***************** frame ***********************/
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
	Tr_expList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;
}


Tr_level Tr_outermost(void) {
	if (!outer)
		outer = Tr_newLevel(NULL, Temp_newlabel(), NULL);
	return outer;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
	Tr_level l = checked_malloc(sizeof(*l));
	l->parent = parent;
	l->name = name;
	l->frame = F_newFrame(name, U_BoolList(TRUE, formals));
	l->formals = makeFormalList(l);
	return l;
}

Tr_accessList Tr_formals(Tr_level level) {
	return level->formals;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
	return Tr_Access(level, F_allocLocal(level->frame, escape));
}


/***************** IR tree ***********************/
static F_fragList fragList = NULL;
F_fragList Tr_getResult() {
	return fragList;
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) {
	// todo:
	F_frag frag = F_ProcFrag(unNx(body), level->frame);
	fragList = F_FragList(frag, fragList);
}

Tr_exp Tr_null() {
	return Tr_Ex(T_Const(0));
}

Tr_exp Tr_simpleVar(Tr_access acc, Tr_level lev) {
	assert(acc && lev);
	return Tr_Ex(F_Exp(acc->access, trackLink(lev, acc->level)));
}

Tr_exp Tr_fieldVar(Tr_exp base, int off) {
	assert(base);
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Const(off * F_wordSize))));
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index) {
	assert(base && index);
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Binop(
						T_mul, unEx(index), T_Const(F_wordSize)))));
}

static Temp_temp nil = NULL;
Tr_exp Tr_nilExp() {
	if (!nil) {
		nil = Temp_newtemp();
		T_exp alloc = F_externalCall(String("malloc"), 
									 T_ExpList(T_Const(0*F_wordSize), NULL));
		return Tr_Ex(T_Eseq(T_Move(T_Temp(nil), alloc), T_Temp(nil)));
	} else
		return Tr_Ex(T_Temp(nil));
}

Tr_exp Tr_intExp(int val) {
	return Tr_Ex(T_Const(val));
}

Tr_exp Tr_stringExp(string str) {
	Temp_label lb = Temp_newlabel();
	F_frag frag = F_StringFrag(lb, str);
	fragList = F_FragList(frag, fragList);
	return Tr_Ex(T_Name(lb));
}

Tr_exp Tr_callExp(Temp_label fun, Tr_expList el, Tr_level lev, Tr_level call) {
	assert(fun);
	T_expList args = Tr_transExpList(el);	
	args = T_ExpList(trackLink(call, lev->parent), args);
	/*
#if DB
	printf("-------call----------\n");
	pr_tree_exp(stdout, T_Call(T_Name(fun), args), 3);
	putchar('\n');
#endif
*/
	return Tr_Ex(T_Call(T_Name(fun), args));	
}

Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right) {
	assert(left && right);
	T_binOp op;
	switch (oper) {
		case A_plusOp:		op = T_plus; break;
		case A_minusOp:		op = T_minus; break;
		case A_timesOp:		op = T_mul;	break;
		case A_divideOp:	op = T_div; break;
		default:	assert(0);
	}
	return Tr_Ex(T_Binop(op, unEx(left), unEx(right)));	
}

Tr_exp Tr_relExp(A_oper oper, Tr_exp left, Tr_exp right) {
	assert(left && right);
	T_relOp op;
	switch (oper) {
		case A_eqOp:	op = T_eq;	break;
		case A_neqOp:	op = T_ne;	break;
		case A_ltOp:	op = T_lt;	break;
		case A_leOp:	op = T_le;	break;
		case A_gtOp:	op = T_gt;	break;
		case A_geOp:	op = T_ge;	break;
		default:	assert(0);
	}
	T_stm stm = T_Cjump(op, unEx(left), unEx(right), NULL, NULL);
	patchList trues = PatchList(&stm->u.CJUMP.true, NULL);
	patchList falses = PatchList(&stm->u.CJUMP.false, NULL);
	return Tr_Cx(trues, falses, stm);
}

Tr_exp Tr_eqStrExp(A_oper oper, Tr_exp left, Tr_exp right) {
	T_exp ans = F_externalCall(String("stringEqual"), 
								T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
	if (oper == A_eqOp)
		return Tr_Ex(ans);
	else
		return Tr_Ex(T_Binop(T_minus, T_Const(1), ans));	
	//<>: 1-1 = 0   1-0=1
}

Tr_exp Tr_eqRefExp(A_oper oper, Tr_exp left, Tr_exp right) {
	T_relOp op;
	if (oper == A_eqOp)
		op = T_eq;
	else if (oper == A_neqOp)
		op = T_ne;
	else
		assert(0);
	T_stm stm = T_Cjump(op, unEx(left), unEx(right), NULL, NULL);
	patchList trues = PatchList(&stm->u.CJUMP.true, NULL);
	patchList falses = PatchList(&stm->u.CJUMP.false, NULL);
	return Tr_Cx(trues, falses, stm);
}

Tr_exp Tr_recordExp(int n, Tr_expList l) {
	assert(n && l);
	Temp_temp r = Temp_newtemp();
	T_stm alloc = T_Move(T_Temp(r), 
						 F_externalCall(String("malloc"), 
										T_ExpList(T_Const(n*F_wordSize), NULL)));
	int i = n - 1;
	T_stm seq = T_Move(T_Mem(T_Binop(T_plus, 
									  T_Temp(r), 
									   T_Const(i-- * F_wordSize))),  unEx(l->head));

	for (l = l->tail; l; l = l->tail, i--) {
		seq = T_Seq(T_Move(T_Mem(T_Binop(T_plus, 
											    T_Temp(r), 
										         T_Const(i*F_wordSize))),  unEx(l->head)), seq); 
	}
	return Tr_Ex(T_Eseq(T_Seq(alloc, seq), T_Temp(r)));
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
	//todo ????
	assert(size && init);
	return Tr_Ex(F_externalCall(String("initArray"), 
								 T_ExpList(unEx(size), 
								 T_ExpList(unEx(init), NULL))));
}

//Tr_expList ÄæÐò´¦Àí
Tr_exp Tr_seqExp(Tr_expList l) {
	assert(l);	
	T_exp seq = unEx(l->head);
	Tr_expList ptr;
	for (ptr = l->tail; ptr; ptr = ptr->tail) 
		seq = T_Eseq(unNx(ptr->head), seq);
#if DB
	printf("-----seqExp---------\n");
	printStmList(stdout, T_StmList(T_Exp(seq), NULL));
	putchar('\n');
#endif
	return Tr_Ex(seq);
}

Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right) {
	return Tr_Nx(T_Move(unEx(left), unEx(right)));
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee) {
	assert(test && then);
	struct Cx cond = unCx(test);
	Temp_label t = Temp_newlabel();
	Temp_label f = Temp_newlabel();
	doPatch(cond.trues, t);
	doPatch(cond.falses, f);

	if (!elsee) {	
		//if - then
		if (then->kind == Tr_cx) {
			return Tr_Nx(T_Seq(cond.stm, 
						  T_Seq(T_Label(t),
						   T_Seq(unNx(then),
								 T_Label(f)))));
		} else {
			return Tr_Nx(T_Seq(cond.stm, 
						  T_Seq(T_Label(t),
						   T_Seq(unNx(then),
								 T_Label(f)))));
		}
	} else {
		//if - then - else
		Temp_label join = Temp_newlabel();
		T_stm joinJump = T_Jump(T_Name(join), Temp_LabelList(join, NULL));
		
		if (then->kind == Tr_nx || elsee->kind == Tr_nx) { 
			return Tr_Nx(T_Seq(cond.stm, 
						  T_Seq(T_Label(t),
						   T_Seq(unNx(then),
						    T_Seq(joinJump,
							 T_Seq(T_Label(f),
							  T_Seq(unNx(then),
									T_Label(join))))))));
		} else {
			Temp_temp r = Temp_newtemp();
			//todo:  special treatment for cx
			return Tr_Ex(T_Eseq(cond.stm, 
						  T_Eseq(T_Label(t),
						   T_Eseq(T_Move(T_Temp(r), unEx(then)),
						    T_Eseq(joinJump,
							 T_Eseq(T_Label(f),
							  T_Eseq(T_Move(T_Temp(r), unEx(elsee)),
							   T_Eseq(T_Label(join), 
									  T_Temp(r)))))))));
		}
	}
}

Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Tr_exp done) {
/*
	test: 
		  if not(cond) goto done 
		  body
		  goto test
	done:
*/		
	assert(test && body);
	struct Cx cond = unCx(test);
	Temp_label lbTest = Temp_newlabel();
	Temp_label lbBody = Temp_newlabel();
	Temp_label lbDone = unEx(done)->u.NAME;
	doPatch(cond.trues, lbBody);
	doPatch(cond.falses, lbDone);
	return Tr_Nx(T_Seq(T_Label(lbTest),
				  T_Seq(cond.stm,
				   T_Seq(T_Label(lbBody),
					T_Seq(unNx(body),
					 T_Seq(T_Jump(T_Name(lbTest), Temp_LabelList(lbTest, NULL)),
						   T_Label(lbDone)))))));
}

Tr_exp Tr_forExp(Tr_level lev, Tr_access iac, Tr_exp lo, Tr_exp hi, Tr_exp body, Tr_exp done) {
/*
	let
		var i = lo
		var limit = high
	in
		if (i < limit)
			do body	i++
			while i < limit
	end
*/
/*
   while (i <= lim)
   do (body; i++)
 */
	T_stm istm = unNx(Tr_assignExp(Tr_simpleVar(iac, lev), lo));
	Tr_access limac = Tr_allocLocal(lev, FALSE);
	T_stm limstm = unNx(Tr_assignExp(Tr_simpleVar(limac, lev), hi));

	T_stm ifstm = T_Cjump(T_lt, unEx(Tr_simpleVar(iac, lev)), unEx(Tr_simpleVar(limac, lev)), NULL, NULL);
	patchList trues = PatchList(&ifstm->u.CJUMP.true, NULL);
	patchList falses = PatchList(&ifstm->u.CJUMP.false, NULL);
	struct Cx ifcond = unCx(Tr_Cx(trues, falses, ifstm));

	T_stm dobody = T_Seq(unNx(body), 
						  T_Move(unEx(Tr_simpleVar(iac, lev)), 
								 T_Binop(T_plus, unEx(Tr_simpleVar(iac, lev)), T_Const(1))));

	T_stm whstm = T_Cjump(T_lt, unEx(Tr_simpleVar(iac, lev)), unEx(Tr_simpleVar(limac, lev)), NULL, NULL);
	trues = PatchList(&whstm->u.CJUMP.true, NULL);
	falses = PatchList(&whstm->u.CJUMP.false, NULL);
	struct Cx whcond = unCx(Tr_Cx(trues, falses, whstm));

	Temp_label lbBody = Temp_newlabel();
	Temp_label lbDone = unEx(done)->u.NAME;


	doPatch(ifcond.trues, lbBody);
	doPatch(ifcond.falses, lbDone);
	doPatch(whcond.trues, lbBody);
	doPatch(whcond.falses, lbDone);


	T_stm circle = T_Seq(ifcond.stm,
					T_Seq(T_Label(lbBody),
					 T_Seq(dobody,
					  T_Seq(whcond.stm,
					   T_Label(lbDone)))));

	return Tr_Nx(T_Seq(T_Seq(istm, limstm), circle));
}

Tr_exp Tr_doneExp() {
	return Tr_Ex(T_Name(Temp_newlabel()));
}

Tr_exp Tr_breakExp(Tr_exp done) {
	assert(done);
	Temp_label lbDone = unEx(done)->u.NAME;
	return Tr_Nx(T_Jump(unEx(done), Temp_LabelList(lbDone, NULL)));
}

static Tr_access Tr_Access(Tr_level level, F_access access) {
	Tr_access a = checked_malloc(sizeof(*a));
	a->level = level;
	a->access = access;
	return a;
}

static Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
	Tr_accessList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l; 
}

static patchList PatchList(Temp_label *head, patchList tail) {
	patchList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;
}

static patchList doPatch(patchList tList, Temp_label label) {
	for (; tList; tList = tList->tail)
		*(tList->head) = label;
}

static patchList joinPatch(patchList first, patchList second) {
	if (!first)	
		return second;
	for (; first->tail; first = first->tail);
	first->tail = second;
	return first;
}

static Tr_exp Tr_Ex(T_exp ex) {
	Tr_exp e = checked_malloc(sizeof(*e));
	e->kind = Tr_ex;
	e->u.ex = ex;
	return e;
}

static Tr_exp Tr_Nx(T_stm nx) {
	Tr_exp e = checked_malloc(sizeof(*e));
	e->kind = Tr_nx;
	e->u.nx = nx;
	return e;
}
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
	Tr_exp e = checked_malloc(sizeof(*e));
	e->kind = Tr_cx;
	e->u.cx.trues = trues;
	e->u.cx.falses = falses;
	e->u.cx.stm = stm;
	return e;
}

static T_exp unEx(Tr_exp e) {
	assert(e);
	switch (e->kind) {
		case Tr_ex:
			return e->u.ex;
		case Tr_nx:
			return T_Eseq(e->u.nx, T_Const(0));
		case Tr_cx: {
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel();
			Temp_label f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
					T_Eseq(e->u.cx.stm,
					 T_Eseq(T_Label(f),
					  T_Eseq(T_Move(T_Temp(r), T_Const(0)),
					   T_Eseq(T_Label(t),
						   T_Temp(r))))));
		}
	}
	assert(0);
}

static T_stm unNx(Tr_exp e) {
	assert(e);
	switch (e->kind) { 
		case Tr_ex:
			return T_Exp(e->u.ex);
		case Tr_nx:
			return e->u.nx;
		case Tr_cx: {
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel();
			Temp_label f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Exp(T_Eseq(T_Move(T_Temp(r), T_Const(1)),
						  T_Eseq(e->u.cx.stm,
						   T_Eseq(T_Label(f),
							T_Eseq(T_Move(T_Temp(r), T_Const(0)),
							 T_Eseq(T_Label(t),
							  T_Temp(r)))))));
		}
	}
	assert(0);
}

static struct Cx unCx(Tr_exp e) {
	assert(e);
	switch (e->kind) {
		case Tr_ex: {
			//todo:   special const(1) const(0)
			T_stm s = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
			patchList t = PatchList(&(s->u.CJUMP.true), NULL);
			patchList f = PatchList(&(s->u.CJUMP.false), NULL);
			Tr_exp tmp = Tr_Cx(t, f, s);
			return tmp->u.cx;
		}
		case Tr_cx:
			return e->u.cx;
	}
	assert(0);
}


static Tr_accessList makeFormalList(Tr_level l) {
	Tr_accessList head = NULL, tail = NULL;
	F_accessList ptr = F_formals(l->frame);
	for (; ptr; ptr = ptr->tail) {
		Tr_access ac = Tr_Access(l, ptr->head);
		if (head) {
			tail->tail = Tr_AccessList(ac, NULL);
			tail = tail->tail;
		} else {
			head = Tr_AccessList(ac, NULL);
			tail = head;
		}
	}
	return head;
}	

static T_exp trackLink(Tr_level l, Tr_level g) {
	T_exp e = T_Temp(F_FP());
	if (l == g) {
		return e;
	}
	while (l != g) {
		assert(l);
		F_access ac = F_formals(l->frame)->head;
		e = F_Exp(ac, e);
		l = l->parent;
	}
	return e;
}

static T_expList Tr_transExpList(Tr_expList l) {
	T_expList h = NULL, t = NULL;
	for (; l; l = l->tail) {
		if (h) {
			t->tail = T_ExpList(unEx(l->head), NULL);
			t = t->tail;
		} else {
			h = T_ExpList(unEx(l->head), NULL);
			t = h;
		}
	}
	return h;
}














