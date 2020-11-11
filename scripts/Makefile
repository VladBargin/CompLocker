CC=gcc
AR=ar
CFLAGS=-I/opt/vc/include -I.
LDFLAGS=-L/opt/vc/lib -L. -lEGL -lGLESv2 -ltftgl -lbcm2835 -lm -lnanovg -lwiringPi

.PHONY: default all clean

default: init
all: default
	
init: init.o
	$(CC) -o init init.o $(LDFLAGS)
	
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
clean:
	-rm -f init init.o
