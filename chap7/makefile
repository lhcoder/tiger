objects = util.o errormsg.o lex.yy.o y.tab.o absyn.o parse.o table.o \
		  symbol.o escape.o types.o env.o temp.o i386frame.o tree.o \
		  translate.o semant.o prabsyn.o printtree.o semtest.o 

a.out: $(objects)
	cc -g $(objects)

semtest.o: semtest.c errormsg.h util.h absyn.h parse.h prabsyn.h escape.h
	cc -g -c semtest.c

printtree.o: printtree.c printtree.h util.h symbol.h temp.h tree.h
	cc -g -c printtree.c

prabsyn.o: prabsyn.c util.h symbol.h absyn.h prabsyn.h
	cc -g -c prabsyn.c

semant.o: semant.c semant.h
	cc -g -c semant.c

env.o: env.c util.h env.h
	cc -g -c env.c

translate.o: translate.c translate.h util.h temp.h
	cc -g -c translate.c

tree.o: tree.c tree.h util.h symbol.h temp.h
	cc -g -c tree.c

i386frame.o: i386frame.c frame.h util.h temp.h
	cc -g -c i386frame.c

temp.o: temp.c temp.h util.h table.h symbol.h
	cc -g -c temp.c

types.o: types.c util.h symbol.h types.h
	cc -g -c types.c

escape.o: escape.c escape.h symbol.h
	cc -g -c escape.c

symbol.o: symbol.c util.h symbol.h
	cc -g -c symbol.c

table.o: table.c util.h table.h
	cc -g -c table.c

parse.o: parse.c util.h symbol.h absyn.h errormsg.h parse.h
	cc -g -c parse.c

absyn.o: absyn.c util.h symbol.h absyn.h
	cc -g -c absyn.c

y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.y
	yacc -dv tiger.y

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h absyn.h
	cc -g -c lex.yy.c

lex.yy.c: tiger.lex
	lex tiger.lex

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f a.out *.o y.tab.c y.tab.h lex.yy.c
