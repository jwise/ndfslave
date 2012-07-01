CFLAGS = -L /usr/local/lib64/digilent/adept/ -ldepp -I /usr/local/include/digilent/adept/
CXXFLAGS = -L /usr/local/lib64/digilent/adept/ -ldepp -I /usr/local/include/digilent/adept/

all: flash iobert xor-me

flash:

iobert:

entropy: entropy.c
	gcc -o entropy entropy.c -O3

xor-me: xor-me.c
	gcc -o xor-me xor-me.c -O3
