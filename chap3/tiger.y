/**********************************************************
 * Author        : lh
 * Email         : lhcoder@163.com
 * Create time   : 2017-02-27 21:18
 * Last modified : 2017-02-27 21:18
 * Filename      : tiger.y
 * Description   : parse for tiher
 * *******************************************************/

%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#define YYDEBUG 1

int yylex();

void yyerror(char *s)
{
	 EM_error(EM_tokPos, "%s", s);
}
%}


%union {
	int pos;
	int ival;
	string sval;
}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%left SEMICOLON
%right THEN ELSE DOT DO OF
%right ASSIGN 
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%start program

%%

program	: exp
		;



decs	: dec decs   
		|
		;

dec		: tydec		
		| vardec	
		| fundec
		;


tydec	: TYPE ID EQ ty
		;

ty		: ID
		| LBRACE tyfields RBRACE
		| ARRAY OF ID
		;

tyfields: tyfield
		|
		;

tyfield: ID COLON ID 
		| tyfield COMMA ID COLON ID
		;

vardec	: VAR ID ASSIGN exp			
		| VAR ID COLON ID ASSIGN exp
		;


fundec	: FUNCTION ID LPAREN tyfields RPAREN EQ exp
		| FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp
		;



exp : lvalue
	| INT		
	| STRING
	| NIL
	| LPAREN expseq RPAREN
	| LPAREN RPAREN

	| MINUS exp  %prec UMINUS
	| exp PLUS exp
	| exp MINUS exp
	| exp TIMES exp
	| exp DIVIDE exp

	| exp EQ exp
	| exp NEQ exp
	| exp LT exp
	| exp LE exp
	| exp GT exp
	| exp GE exp

	| exp AND exp
	| exp OR exp
	
	| funcall

	| ID LBRACK exp RBRACK OF exp
	| ID LBRACE RBRACE
	| ID LBRACE ass RBRACE
	| lvalue ASSIGN exp

	| IF exp THEN exp
	| IF exp THEN exp ELSE exp
	| WHILE exp DO exp
	| FOR ID ASSIGN exp TO exp DO exp
	| BREAK

	| LET decs IN END
	| LET decs IN expseq END	
	| LPAREN error RPAREN
	| error SEMICOLON exp
	;


funcall	: ID LPAREN RPAREN
		| ID LPAREN para RPAREN
		;

para	: exp
		| exp COMMA para
		;

ass		: ID EQ exp
		| ID EQ exp COMMA ass
		;

expseq	: exp				
		| exp SEMICOLON expseq	
		;

lvalue	: ID	
		| lvalue DOT ID
		| lvalue LBRACK exp RBRACK 
		| ID LBRACK exp RBRACK 
		;












