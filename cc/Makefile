SRC=$(wildcard *.c)
OBJ=$(filter-out y.tab.o, $(patsubst %.c,%.o,$(SRC))) y.tab.o

CFLAGS=-Werror -std=gnu89 -g -I../lib
LDFLAGS=-L../lib -lcommon -lm

all: twcc

test: all
	$(MAKE) -C tests test

../lib/libcommon.a:
	$(MAKE) -C ../lib libcommon.a

twcc: $(OBJ) ../lib/libcommon.a
	cc -o $@ $^ $(LDFLAGS)

y.tab.c: cc_grammar.y
	yacc $^
	yacc -d $^

clean:
	rm -f *.o twcc y.tab.h y.tab.c a.out
	$(MAKE) -C tests $@
