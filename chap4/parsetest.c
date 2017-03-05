#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "parse.h"
#include "prabsyn.h"

int main(int argc, char **argv) {
 if (argc!=2) {fprintf(stderr,"usage: a.out filename\n"); exit(1);}
 A_exp s = parse(argv[1]);
 if (s)
	 pr_exp(stdout, s, 4);
 return 0;
}
