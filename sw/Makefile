CFLAGS = -L /usr/local/lib64/digilent/adept/ -ldepp -I /usr/local/include/digilent/adept/
CXXFLAGS = -L /usr/local/lib64/digilent/adept/ -ldepp -I /usr/local/include/digilent/adept/

all: flash flash-det iobert xor-me

flash:

flash-det:

iobert:

entropy: entropy.c
	gcc -o entropy entropy.c -O3

xor-me: xor-me.c
	gcc -o xor-me xor-me.c -O3
