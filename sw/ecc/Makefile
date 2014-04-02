CC=gcc
CFLAGS=-O2

all: bchtest bch-me

bchtest: bch.o bchtest.o

bch-me: bch-me.o bch.o

bch-me.o: bch-me.c bch.h

bchtest.o: bchtest.c bch.h

bch.o: bch.c bch.h fls.h

