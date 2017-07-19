/**********************************************************
 * Author        : lh
 * Email         : lhcoder@163.com
 * Create time   : 2017-03-20 08:46
 * Last modified : 2017-03-20 08:46
 * Filename      : escape.c
 * Description   : 
 * *******************************************************/

#include <stdio.h>
#include "symbol.h"
#include "escape.h"

#define DB 0


typedef struct escapeEntry_ *escapeEntry;
struct escapeEntry_ {
	int depth;
	bool *escape;
};

static escapeEntry EscapeEntry(int depth, bool *escape);

static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);

void Esc_findEscape(A_exp exp) {
	assert(exp);
	S_table env = S_empty();
	traverseExp(env, 0, exp);
}

void traverseExp(S_table env, int depth, A_exp e) {
	switch (e->kind) {
		case A_varExp: 
			traverseVar(env, depth, e->u.var);	
			return;
		case A_callExp: {
			A_expList el;
			for (el = e->u.call.args; el; el = el->tail) 
				traverseExp(env, depth, el->head);
			return;
		}
		case A_opExp: {
			traverseExp(env, depth, e->u.op.left);
			traverseExp(env, depth, e->u.op.right);
			return;
		}
		case A_recordExp: {
			A_efieldList ei;
			for (ei = e->u.record.fields; ei; ei = ei->tail)
				traverseExp(env, depth, ei->head->exp);
			return;
		}
		case A_seqExp: {
			A_expList el = e->u.seq;
			for (; el; el = el->tail) 
				traverseExp(env, depth, el->head);
			return;
		}
		case A_assignExp: {
			traverseVar(env, depth, e->u.assign.var);	
			traverseExp(env, depth, e->u.assign.exp);	
			return;
		}
		case A_ifExp: {
			traverseExp(env, depth, e->u.iff.test);	
			traverseExp(env, depth, e->u.iff.then);	
			if (e->u.iff.elsee) 
				traverseExp(env, depth, e->u.iff.elsee);	
			return;
		}
		case A_whileExp: {
			traverseExp(env, depth, e->u.whilee.test);	
			traverseExp(env, depth, e->u.whilee.body);	
			return;
		}
		case A_forExp: {
			//A_ifExp has a escape variable
			e->u.forr.escape = FALSE;
			S_enter(env, e->u.forr.var, EscapeEntry(depth, &(e->u.forr.escape)));
			traverseExp(env, depth, e->u.forr.lo);
			traverseExp(env, depth, e->u.forr.hi);
			traverseExp(env, depth, e->u.forr.body);
			return;
		}
		case A_letExp: {
			A_decList lt;
			for (lt = e->u.let.decs; lt; lt = lt->tail) 
				traverseDec(env, depth, lt->head);
			traverseExp(env, depth, e->u.let.body);
			return;
		}
		case A_arrayExp: {
			traverseExp(env, depth, e->u.array.size);	
			traverseExp(env, depth, e->u.array.init);	
			return;
		}
		default:
			return;
	}
	assert(0);
}

void traverseDec(S_table env, int depth, A_dec d) {
	switch (d->kind) {
		case A_varDec: {
			traverseExp(env, depth, d->u.var.init);
			d->u.var.escape = FALSE;
			S_enter(env, d->u.var.var, EscapeEntry(depth, &(d->u.var.escape)));
			return;
		}

		case A_typeDec: 
			return;
		case A_functionDec: {
			A_fundecList fl;
			for (fl = d->u.function; fl; fl = fl->tail) {
				A_fundec fun = fl->head;
				A_fieldList el;

				S_beginScope(env);
				for (el = fun->params; el; el = el->tail) {
					el->head->escape = FALSE;
					S_enter(env, el->head->name, EscapeEntry(depth+1, &(el->head->escape)));
				}
				traverseExp(env, depth+1, fun->body);
				S_endScope(env);
			}
			return;
		}
	}
	assert(0);
}

void traverseVar(S_table env, int depth, A_var v) {
	switch (v->kind) {
		case A_simpleVar: {
			escapeEntry e = S_look(env, v->u.simple);
			if (!e)
				return;
#if DB
			if (e->depth < depth && *(e->escape) == FALSE)
				printf("%s\n", S_name(v->u.simple));
#endif
			if (e->depth < depth)
				*(e->escape) = TRUE;
			return;
		}
		case A_fieldVar: 
			traverseVar(env, depth, v->u.field.var);
			return;
		case A_subscriptVar: 
			traverseVar(env, depth, v->u.subscript.var);
			traverseExp(env, depth, v->u.subscript.exp);
			return;
	}
	assert(0);
}


escapeEntry EscapeEntry(int depth, bool *escape) {
	escapeEntry e = checked_malloc(sizeof(*e));
	e->depth = depth;
	e->escape = escape;
	return e;
}

