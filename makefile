CC=g++
CXXFLAGS=-pipe -Wpedantic -Wall -Wextra -Wunreachable-code -Wformat-security -Warray-bounds -fomit-frame-pointer
CXXFLAGS64=-m64 -march=nocona -Og #supports 64 bit x86 processors
CXXFLAGS32=-m32 -march=i386 -Og #supports legacy 32 bit x86 processors
DEBUG=-ggdb #-fsanitize=address,undefined

SRCDIR=./source
VERSION=3.04
SRC=$(SRCDIR)/$(VERSION)/core.h $(SRCDIR)/$(VERSION)/Core.cpp $(SRCDIR)/$(VERSION)/debug.h $(SRCDIR)/$(VERSION)/main.cpp $(SRCDIR)/$(VERSION)/shared.h $(SRCDIR)/$(VERSION)/sign.cpp $(SRCDIR)/$(VERSION)/sign.h

all: SAD

SAD: $(SRC)
	$(CC) -o ./Linux/$@ $^ $(CXXFLAGS64) $(CXXFLAGS) $(DEBUG)
	objcopy --only-keep-debug ./Linux/$@ ./Linux/$@.debug
	objcopy --strip-all ./Linux/$@
	objcopy --add-gnu-debuglink=./Linux/$@.debug ./Linux/$@

clean:
	rm -v ./Linux/SAD ./Linux/SAD.debug
