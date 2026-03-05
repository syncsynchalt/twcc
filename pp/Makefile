SRC=$(wildcard *.c)
OBJ=$(filter-out y.tab.o, $(patsubst %.c,%.o,$(SRC))) y.tab.o

CFLAGS=-Werror -std=gnu89 -g -I../lib
LDFLAGS=-L../lib -lcommon -lm

all: twpp

test: all
	$(MAKE) -C tests test

../lib/libcommon.a:
	$(MAKE) -C ../lib libcommon.a

twpp: $(OBJ) ../lib/libcommon.a
	cc -o $@ $^ $(LDFLAGS)

y.tab.c: pp_grammar.y
	yacc $^
	yacc -d $^

clean:
	rm -f *.o twpp y.tab.h y.tab.c a.out
	$(MAKE) -C tests $@
