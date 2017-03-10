a.out: semtest.o y.tab.o lex.yy.o errormsg.o util.o table.o symbol.o absyn.o parse.o prabsyn.o semant.o types.o env.o
	cc -g semtest.o y.tab.o lex.yy.o errormsg.o util.o table.o symbol.o absyn.o parse.o prabsyn.o semant.o types.o env.o

semtest.o: semtest.c errormsg.h util.h absyn.h parse.h prabsyn.h
	cc -g -c semtest.c

semant.o: semant.c semant.h
	cc -g -c semant.c

env.o: env.c util.h env.h
	cc -g -c env.c

types.o: types.c util.h symbol.h types.h
	cc -g -c types.c

y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.y
	yacc -dv tiger.y

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

prabsyn.o: prabsyn.c util.h symbol.h absyn.h prabsyn.h
	cc -g -c prabsyn.c

parse.o: parse.c util.h symbol.h absyn.h errormsg.h parse.h
	cc -g -c parse.c

absyn.o: absyn.c util.h symbol.h absyn.h
	cc -g -c absyn.c

symbol.o: symbol.c util.h symbol.h
	cc -g -c symbol.c

table.o: table.c util.h table.h
	cc -g -c table.c

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h absyn.h
	cc -g -c lex.yy.c

lex.yy.c: tiger.lex
	lex tiger.lex

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f a.out *.o y.tab.c y.tab.h lex.yy.c
