/* function prototype from printtree.c */
#ifndef PRINTTREE_H
#define PRINTTREE_H

void printStmList (FILE *out, T_stmList stmList) ;

void pr_tree_exp(FILE *out, T_exp exp, int d);
void pr_stm(FILE *out, T_stm stm, int d);

#endif

