CC=gcc
AR=ar
CFLAGS=-I/opt/vc/include -I.
LDFLAGS=-L/opt/vc/lib -L. -lEGL -lGLESv2 -ltftgl -lbcm2835 -lm -lnanovg -lwiringPi -lpigpio -lrt -O2

.PHONY: default all clean

default: main
all: default
	
#init: init.o
#	$(CC) -o init init.o $(LDFLAGS)

test: test.o
	$(CC) -o test test.o $(LDFLAGS)

main: main.o
	$(CC) -o main main.o $(LDFLAGS)

	
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
clean:
	-rm -f init init.o
