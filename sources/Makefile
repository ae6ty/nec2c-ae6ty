#example: make -k OS=Windows nec2c.exe (maybe)
#defaults
CFLAGS = -Ofast
CC = clang
LDFLAGS = -lm -lpthread
#set variables based on OS...
ifeq ($(OS),Windows)
	CFLAGS = -Ofast -D WINDOWS
	CC = gcc 
	LDFLAGS = -static -lm -pthread
endif
ifeq ($(OS),Linux)
	CFLAGS = -Ofast -p
	CC = gcc
	LDFLAGS = -lm -pthread
endif
ifeq ($(OS),OSX)
	CFLAGS = -Ofast
	CC = clang
	LDFLAGS = -lm -lpthread
endif

objFiles = calculations.o ground.o matrix.o radiation.o fields.o \
input.o misc.o shared.o geometry.o main.o network.o somnec.o backtrace.o \
getTimeOfDay.o lu.o flock.o

hdrFiles = nec2c.h shared.h lu.h flock.h

nec2c.exe: $(objFiles)
	echo $(OS)
	$(CC)  $(CFLAGS) $(objFiles) $(LDFLAGS) -o nec2c
	cp nec2c.exe ../bin

# for linux
nec2c-ubuntu-x86: $(objFiles)
	$(CC)  $(CFLAGS) $(objFiles) $(LDFLAGS) -o nec2c-ubuntu-x86
	cp nec2c-ubuntu-x86 ../bin

nec2c-mac-x86: $(objFiles)
	$(CC)  $(CFLAGS) $(objFiles) -o nec2c-mac-x86
	cp nec2c-mac-x86 ../bin

nec2c-debian-arm: $(objFiles)
	$(CC) -p $(CFLAGS) $(objFiles) $(LDFLAGS) -o nec2c-debian-arm
	cp nec2c-debian-arm ../bin

nec2c-mac-m1: $(objFiles)
	$(CC)  $(CFLAGS) $(objFiles) -o nec2c-mac-m1
	cp nec2c-mac-m1 ../bin

$(objFiles): $(hdrFiles) Makefile 

clean:
	rm -f $(objFiles) nec2c *~

zip:
	cd .. ; ls ; rm -rf bin.zip; zip -q -r bin.zip bin/* -x "*.DS_Store"
