CC=g++
CXXFLAGS=-pipe -Wpedantic -Wall -Wextra
CXXFLAGS64=-m64 -march=nocona -Og #supports 64 bit x86 processors
CXXFLAGS32=-m32 -march=i386 -Og #supports legacy 32 bit x86 processors
DEBUG=-ggdb #-fsanitize=address,undefined -Wformat-security -Warray-bounds

SRCDIR=./source
VERSION=3.08
SRC=$(SRCDIR)/$(VERSION)/*.h $(SRCDIR)/$(VERSION)/*.cpp

all: SADX

SADX: $(SRC)
	$(CC) -o ./Linux/$@ $^ $(CXXFLAGS64) $(CXXFLAGS) $(DEBUG)
	objcopy --only-keep-debug ./Linux/$@ ./Linux/$@.debug
	objcopy --strip-all ./Linux/$@
	objcopy --add-gnu-debuglink=./Linux/$@.debug ./Linux/$@

clean:
	rm -v ./Linux/SADX ./Linux/SADX.debug
