/* function prototype from prabsyn.c */
#ifndef PRABSYN_H
#define PARBSYN_H

#include <stdio.h>
#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */

void pr_exp(FILE *out, A_exp v, int d);

#endif

