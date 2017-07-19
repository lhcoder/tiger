#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "parse.h"
#include "prabsyn.h"
#include "semant.h"
#include "escape.h"

int main(int argc, char **argv) {
	if (argc!=2) {
		fprintf(stderr,"usage: a.out filename\n"); 
		exit(1);
	}
	A_exp s = parse(argv[1]);
//	if (s)
//		pr_exp(stdout, s, 4);
	if (!s)
		return 0;

	Esc_findEscape(s);
	SEM_transProg(s);
	return 0;
}
