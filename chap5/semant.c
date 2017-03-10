/**********************************************************
 * Author        : lh
 * Email         : lhcoder@163.com
 * Create time   : 2017-03-06 15:53
 * Last modified : 2017-03-10 15:28
 * Filename      : semant.c
 * Description   : 
 * *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "types.h"
#include "env.h"
#include "semant.h"

typedef void *Ty_exp;

struct expty {
	Ty_exp exp;
	Ty_ty ty;
};

struct expty expTy(Ty_exp *exp, Ty_ty ty) {
	struct expty e;
	e.exp = exp;
	e.ty = ty;
	return e;
}

static struct expty transVar(S_table venv, S_table tenv, A_var v);
static struct expty transExp(S_table venv, S_table tenv, A_exp a);
static void transDec(S_table venv, S_table tenv, A_dec d);
static Ty_ty transTy(S_table tenv, A_ty a);


static Ty_fieldList transFieldList(S_table tenv, A_fieldList a); 
static Ty_tyList transTyList(S_table tenv, A_fieldList a);

static Ty_ty actual_ty(Ty_ty t); 
static bool cmp_ty(Ty_ty a, Ty_ty b); 


struct set_ {
	S_symbol s[1000];
	int pos;
};
typedef struct set_ *set;

static set set_init();
static void set_reset(set s);
static bool set_push(set s, S_symbol x);

static set s;

//nested layers of loop
static int loop;


void SEM_transProg(A_exp exp) {
	S_table tenv = E_base_tenv();
	S_table venv = E_base_venv();
	loop = 0;
	s = set_init();
	transExp(venv, tenv, exp);
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
	switch (v->kind) {
		case A_simpleVar: {
			E_enventry e = S_look(venv, v->u.simple);
			if (e && e->kind == E_varEntry) 
				return expTy(NULL, actual_ty(e->u.var.ty));
			else {
				EM_error(v->pos, "undefined variable '%s'", S_name(v->u.simple));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar: {
			struct expty exp = transVar(venv, tenv, v->u.field.var); 
			Ty_ty t = exp.ty;
			if (t->kind != Ty_record) {
				EM_error(v->pos, "variable not record");
				return expTy(NULL, Ty_Int());
			} else {
				Ty_fieldList fl = t->u.record;
				for (; fl; fl = fl->tail)
					if (fl->head->name == v->u.field.sym) 
						return expTy(NULL, actual_ty(fl->head->ty));
				EM_error(v->pos, "'%s' was not a member", S_name(v->u.field.sym));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_subscriptVar: {
			struct expty var = transVar(venv, tenv, v->u.subscript.var); 
			struct expty exp = transExp(venv, tenv, v->u.subscript.exp); 
			if (var.ty->kind != Ty_array) {
				EM_error(v->pos, "variable not array");
				return expTy(NULL, Ty_Int());
			} 
			if (exp.ty->kind != Ty_int) 
				EM_error(v->pos, "Subscript was not an integer");
			return expTy(NULL, actual_ty(var.ty->u.array));
		}
	}
	assert(0);
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
	switch (a->kind) {
		case A_varExp: 
			return transVar(venv, tenv, a->u.var);	
		case A_nilExp: 
			return expTy(NULL, Ty_Nil());	
		case A_intExp: 
			return expTy(NULL, Ty_Int());	
		case A_stringExp: 
			return expTy(NULL, Ty_String());	
		case A_callExp: {
			E_enventry fun = S_look(venv, a->u.call.func);
			if (!fun) {
				EM_error(a->pos, "undeclared function '%s'", S_name(a->u.call.func));
				return expTy(NULL, Ty_Int());
			} else if (fun->kind == E_varEntry) {
				EM_error(a->pos, "'%s' was a variable", S_name(a->u.call.func));
				return expTy(NULL, fun->u.var.ty);
			}

			Ty_tyList tl = fun->u.fun.formals;
			A_expList el = a->u.call.args;
			for (; tl && el; tl = tl->tail, el = el->tail) {
				struct expty exp = transExp(venv, tenv, el->head);
				if (!cmp_ty(tl->head, exp.ty))
					EM_error(a->pos, "argument type mismatch");
			}
			if (tl) 
				EM_error(a->pos, "too few arguments");
			else if (el)
				EM_error(a->pos, "too many arguments");
			return expTy(NULL, fun->u.fun.result);
		}
		case A_opExp: {
			A_oper oper = a->u.op.oper;
			struct expty left = transExp(venv, tenv, a->u.op.left);
			struct expty right = transExp(venv, tenv, a->u.op.right);
			switch (oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp: {
					if (left.ty->kind != Ty_int) 
						EM_error(a->u.op.left->pos, "integer required");
					if (right.ty->kind != Ty_int) 
						EM_error(a->u.op.right->pos, "integer required");
					return expTy(NULL, Ty_Int());
				} 
				case A_eqOp:
				case A_neqOp: {
					if (left.ty->kind == Ty_void) 
						EM_error(a->u.op.left->pos, "expression had no value");	
					else if (right.ty->kind == Ty_void) 
						EM_error(a->u.op.right->pos, "expression had no value");	
					else if (!cmp_ty(left.ty, right.ty))
						EM_error(a->u.op.right->pos, "comparison type mismatch");
					return expTy(NULL, Ty_Int());
				}
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					if (left.ty->kind != Ty_int && left.ty->kind != Ty_string) 
						EM_error(a->u.op.left->pos, "string or integer required");
					else if (right.ty->kind != left.ty->kind) 
						EM_error(a->u.op.right->pos, "comparison type mismatch");
					return expTy(NULL, Ty_Int());
				}
			} //end of switch (oper)
		} // end of case A_opExp
		case A_recordExp: {
			Ty_ty t = actual_ty(S_look(tenv, a->u.record.typ));
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.record.typ));
				return expTy(NULL, Ty_Int());
			} else if (t->kind != Ty_record) {
				EM_error(a->pos, "'%s' was not a record type", S_name(a->u.record.typ));
				return expTy(NULL, t);
			}

			Ty_fieldList ti = t->u.record;
			A_efieldList ei = a->u.record.fields;
			for (; ti && ei; ti = ti->tail, ei = ei->tail) {
				if (ti->head->name != ei->head->name) {
					EM_error(a->pos, "need member '%s' but '%s'", S_name(ti->head->name), S_name(ei->head->name));
					continue;
				}
				struct expty exp = transExp(venv, tenv, ei->head->exp);
				if (!cmp_ty(ti->head->ty, exp.ty))
					EM_error(a->pos, "member '%s' type mismatch", S_name(ti->head->name));
			}

			if (ti) 
				EM_error(a->pos, "too few initializers for '%s'", S_name(a->u.record.typ));
			else if (ei)
				EM_error(a->pos, "too many initializers for '%s'", S_name(a->u.record.typ));

			return expTy(NULL, t);
		}
		case A_seqExp: {
			if (a->u.seq == NULL)
				return expTy(NULL, Ty_Void());
			A_expList el = a->u.seq;
			for (; el->tail; el = el->tail)
				transExp(venv, tenv, el->head);
			return transExp(venv, tenv, el->head);
		}
		case A_assignExp: {
			struct expty var = transVar(venv, tenv, a->u.assign.var);	
			struct expty exp = transExp(venv, tenv, a->u.assign.exp);	
			if (!cmp_ty(var.ty, exp.ty))
				EM_error(a->pos, "assignment type mismatch");
			return expTy(NULL, Ty_Void());
		}
		case A_ifExp: {
			struct expty t = transExp(venv, tenv, a->u.iff.test);	
			struct expty h = transExp(venv, tenv, a->u.iff.then);	
			if (t.ty->kind != Ty_int)
				EM_error(a->pos, "if-exp was not an integer");
			if (a->u.iff.elsee) {
				struct expty e = transExp(venv, tenv, a->u.iff.elsee);
				if (!cmp_ty(h.ty, e.ty) && !(h.ty->kind == Ty_nil && e.ty->kind == Ty_nil))
					EM_error(a->pos, "then-else had different types");
				return expTy(NULL, h.ty);
			} else {
				if (h.ty->kind != Ty_void)
					EM_error(a->pos, "if-then shouldn't return a value");
				return expTy(NULL, Ty_Void());
			}
		}
		case A_whileExp: {
			struct expty t = transExp(venv, tenv, a->u.whilee.test);	
			loop++;
			struct expty b = transExp(venv, tenv, a->u.whilee.body);	
			loop--;
			if (t.ty->kind != Ty_int)
				EM_error(a->pos, "while-exp was not an integer");
			if (b.ty->kind != Ty_void)
				EM_error(a->pos, "do-exp shouldn't return a value");
			return expTy(NULL, Ty_Void());
		}
		case A_forExp: {
			struct expty l = transExp(venv, tenv, a->u.forr.lo);
			struct expty h = transExp(venv, tenv, a->u.forr.hi);
			S_beginScope(venv);
			S_beginScope(tenv);
			loop++;
			S_enter(venv, a->u.forr.var, E_VarEntry(Ty_Int()));
			struct expty b = transExp(venv, tenv, a->u.forr.body);
			S_endScope(venv);
			S_endScope(tenv);
			loop--;
			if (l.ty->kind != Ty_int)
				EM_error(a->pos, "low bound was not an integer");
			if (h.ty->kind != Ty_int)
				EM_error(a->pos, "high bound was not an integer");
			if (b.ty->kind != Ty_void)
				EM_error(a->pos, "body exp shouldn't return a value");
			return expTy(NULL, Ty_Void());
		}
		case A_breakExp: {
			if (!loop)
				EM_error(a->pos, "break statement not within loop");
			return expTy(NULL, Ty_Void());
		}
		case A_letExp: {
			struct expty exp;
			A_decList lt;
			S_beginScope(venv);
			S_beginScope(tenv);
			for (lt = a->u.let.decs; lt; lt = lt->tail)
				transDec(venv, tenv, lt->head);
			exp = transExp(venv, tenv, a->u.let.body);
			S_endScope(venv);
			S_endScope(tenv);
			return exp;
		}
		case A_arrayExp: {
			Ty_ty t = actual_ty(S_look(tenv, a->u.array.typ));
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.array.typ));
				return expTy(NULL, Ty_Int());
			} else if (t->kind != Ty_array) {
				EM_error(a->pos, "'%s' was not a array type", S_name(a->u.array.typ));
				return expTy(NULL, Ty_Int());
			}
			struct expty z = transExp(venv, tenv, a->u.array.size);	
			struct expty i = transExp(venv, tenv, a->u.array.init);	
			if (z.ty->kind != Ty_int)
				EM_error(a->pos, "array size was not an integer value");
			if (!cmp_ty(i.ty, t->u.array))
				EM_error(a->pos, "array init type mismatch");
			return expTy(NULL, t);
		}
	}
	assert(0);
}

void transDec(S_table venv, S_table tenv, A_dec d) {
	switch (d->kind) {
		case A_varDec: {
			struct expty e = transExp(venv, tenv, d->u.var.init);
			if (d->u.var.typ) {
				Ty_ty t = S_look(tenv, d->u.var.typ);
				if (!t)
					EM_error(d->pos, "undefined type '%s'", S_name(d->u.var.typ));
				else {
					if (!cmp_ty(t, e.ty)) 
						EM_error(d->pos, "var init type mismatch");
					S_enter(venv, d->u.var.var, E_VarEntry(t));
					break;
				}
			}
			if (e.ty == Ty_Void())
				EM_error(d->pos, "initialize with no value");
			else if (e.ty == Ty_Nil())
				EM_error(d->pos, "'%s' is not a record", S_name(d->u.var.var));
			S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
			break;
		}

		case A_typeDec: {
			//only one dec in declist
			//S_enter(tenv, d->u.type->head->name, transTy(tenv, d->u.type->head->ty));
			A_nametyList l;
			set_reset(s);
			for (l = d->u.type; l; l = l->tail) {
				if (!set_push(s, l->head->name)) {
					EM_error(d->pos, "redefinition of '%s'", S_name(l->head->name));
					continue;
				}
				Ty_ty t = Ty_Name(l->head->name, NULL);
				S_enter(tenv, l->head->name, t);
			}

			set_reset(s);
			for (l = d->u.type; l; l = l->tail) {
				if (!set_push(s, l->head->name)) 
					continue;
				Ty_ty t = S_look(tenv, l->head->name);
				t->u.name.ty = transTy(tenv, l->head->ty);
			}

			//check recursive definition
			for (l = d->u.type; l; l = l->tail) {
				Ty_ty t = S_look(tenv, l->head->name);
				set_reset(s);
				t = t->u.name.ty;
				while (t && t->kind == Ty_name) {
					if (!set_push(s, t->u.name.sym)) {
						EM_error(d->pos, "illegal recursive definition '%s'", S_name(t->u.name.sym));
						t->u.name.ty = Ty_Int();
						break;
					}
					t = t->u.name.ty;	
					t = t->u.name.ty;	
				}
			}
			break;
		}
		case A_functionDec: {
			A_fundecList fl;
			set fs = set_init();
			set_reset(fs);
			for (fl = d->u.function; fl; fl = fl->tail) {
				A_fundec fun = fl->head;
				if (!set_push(fs, fun->name)) {
					EM_error(fun->pos, "redefinition of function '%s'", S_name(fun->name));
					continue;
				}
				Ty_ty re = NULL;
				if (fun->result) {
					re = S_look(tenv, fun->result);
					if (!re) {
						EM_error(d->pos, "function result: undefined type '%s'", S_name(fun->result));
						re = Ty_Int();
					}
				} else
					re = Ty_Void();
				set_reset(s);
				Ty_tyList pa = transTyList(tenv, fun->params);
				S_enter(venv, fun->name, E_FunEntry(pa, re));
			}

			set_reset(fs);
			for (fl = d->u.function; fl; fl = fl->tail) {
				A_fundec fun = fl->head;
				if (!set_push(fs, fun->name))
					continue;
				S_beginScope(venv);
				A_fieldList el;
				set_reset(s);
				for (el = fun->params; el; el = el->tail) {
					if (!set_push(s, el->head->name))
						continue;
					Ty_ty t = S_look(tenv, el->head->typ);
					if (!t)
						t = Ty_Int();
					S_enter(venv, el->head->name, E_VarEntry(t));
				}
				E_enventry ent = S_look(venv, fun->name);
				struct expty exp = transExp(venv, tenv, fun->body);

				if (ent->u.fun.result->kind == Ty_void && exp.ty->kind != Ty_void)
					EM_error(fun->pos, "procedure '%s' should't return a value", S_name(fun->name));
				else if (!cmp_ty(ent->u.fun.result, exp.ty))
					EM_error(fun->pos, "body result type mismatch");
				S_endScope(venv);
			}
			break;
		}
	}
}

Ty_ty transTy(S_table tenv, A_ty a) {
	switch (a->kind) {
		case A_nameTy: {
			Ty_ty t = S_look(tenv, a->u.name);
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.name));
				return Ty_Int();
			} else
				return Ty_Name(a->u.name, t);
		}
		case A_recordTy:
			set_reset(s);
			return Ty_Record(transFieldList(tenv, a->u.record));
		case A_arrayTy: {
			Ty_ty t = S_look(tenv, a->u.array);
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.array));
				return Ty_Array(Ty_Int());
			} else
				return Ty_Array(S_look(tenv, a->u.array));
		}
	}
	assert(0);
}



Ty_fieldList transFieldList(S_table tenv, A_fieldList a) {
	if (!a)
		return NULL;
	if (!set_push(s, a->head->name)) {
		EM_error(a->head->pos, "redeclaration of '%s'", S_name(a->head->name));
		return transFieldList(tenv, a->tail);
	}
	Ty_ty t = S_look(tenv, a->head->typ);
	if (!t) {
		EM_error(a->head->pos, "undefined type '%s'", S_name(a->head->typ));
		t = Ty_Int();
	}
	return Ty_FieldList(Ty_Field(a->head->name, t), transFieldList(tenv, a->tail));
}

Ty_tyList transTyList(S_table tenv, A_fieldList a) {
	if (!a)
		return NULL;
	if (!set_push(s, a->head->name)) {
		EM_error(a->head->pos, "redeclaration of '%s'", S_name(a->head->name));
		return transTyList(tenv, a->tail);
	}
	Ty_ty t = S_look(tenv, a->head->typ);
	if (!t) {
		EM_error(a->head->pos, "undefined type '%s'", S_name(a->head->typ));
		t = Ty_Int();
	}
	return Ty_TyList(t, transTyList(tenv, a->tail));
}

//same type use same Ty_ty
bool cmp_ty(Ty_ty a, Ty_ty b) {
	assert(a&&b);
	a = actual_ty(a);
	b = actual_ty(b);
	if (a == b)
		return TRUE;
	else {
		if (a->kind == Ty_record && b->kind == Ty_nil || a->kind == Ty_nil && b->kind == Ty_record)
			return TRUE;
		else 
			return FALSE;
	}
}

//skip Ty_name
Ty_ty actual_ty(Ty_ty t) {
	if (!t)
		return NULL;

	while (t && t->kind == Ty_name)
		t = t->u.name.ty;
	return t;
}


set set_init() {
	set t = checked_malloc(sizeof(*t));
	t->pos = 0;
	return t;
}

void set_reset(set s) {
	s->pos = 0;
}

bool set_push(set s, S_symbol x) {
	int i;
	for (i = 0; i < s->pos; i++)
		if (s->s[i] == x)
			return FALSE;
	s->s[s->pos++] = x;
	return TRUE;
}



