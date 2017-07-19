#include "util.h"
#include "temp.h"
#include "frame.h"

#define DB 0

struct F_frame_ {
	F_accessList formals, locals;
	int local_count;
	Temp_label label;
};

struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};

const int F_WORD_SIZE = 4;		//i386: 32bit
static const int F_KEEP = 4;	//number of parameters kept in regs;

static F_access InFrame(int offset) {
	F_access ac = checked_malloc(sizeof(*ac));
	ac->kind = inFrame;
	ac->u.offset = offset;
#if DB
	printf("\t\tinFrame: %d\n", offset);
#endif
	return ac;
}

static F_access InReg(Temp_temp reg) {
	F_access ac = checked_malloc(sizeof(*ac));
	ac->kind = inReg;
	ac->u.reg = reg;
#if DB
	printf("\t\tinReg\n");
#endif
	return ac;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
	F_accessList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame fr = checked_malloc(sizeof(*fr));
	fr->label = name;		
	fr->formals = NULL;
	fr->locals = NULL;
	fr->local_count = 0;

	F_accessList head = NULL, tail = NULL;
	int rn = 0, fn = 0;
	U_boolList ptr;
	for (ptr = formals; ptr; ptr = ptr->tail) {
		F_access ac = NULL;
		if (rn < F_KEEP && !(ptr->head)) {
			ac = InReg(Temp_newtemp());	
			rn++;
		} else {
			fn++;
			ac = InFrame((fn+1)*F_WORD_SIZE);	//1 return address
		}

		if (head) {
			tail->tail = F_AccessList(ac, NULL);
			tail = tail->tail;
		} else {
			head = F_AccessList(ac, NULL);
			tail = head;
		}
	}
	fr->formals = head;

	return fr;
}

Temp_label F_name(F_frame f) {
	return f->label;
}

F_accessList F_formals(F_frame f) {
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
	f->local_count++;
	if (escape) 
		return InFrame(-F_WORD_SIZE * (f->local_count));
	else 
		return InReg(Temp_newtemp());
}







