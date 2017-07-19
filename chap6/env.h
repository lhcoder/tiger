/**********************************************************
 * Author        : lh
 * Email         : lhcoder@163.com
 * Create time   : 2017-03-06 16:10
 * Last modified : 2017-03-19 17:57
 * Filename      : env.h
 * Description   : 
 * *******************************************************/
#ifndef ENV_H
#define ENV_H

#include "types.h"
#include "symbol.h"
#include "translate.h"

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
	enum {E_varEntry, E_funEntry} kind;
	union { struct {Tr_access access; Ty_ty ty;} var;
			struct {
				Tr_level level;
				Temp_label label;
				Ty_tyList formals; 
				Ty_ty result;
			} fun;
	} u;
};

E_enventry E_VarEntry(Tr_access access, Ty_ty ty);
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result);

S_table E_base_tenv(void);	//Ty_ty environment
S_table E_base_venv(void);	//E_enventry environment

#endif
