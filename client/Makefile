OS = LINUX
#OS = MACOSX
#OS = WINDOWS

PROG = gc_control_client

# To set up Ubuntu Linux to cross compile for Windows:
#
#    apt-get install mingw32 mingw32-binutils mingw32-runtime
#
# Just edit the variable above for WINDOWS, then use "make" to build rawhid.exe

ifeq ($(OS), LINUX)
	TARGET = $(PROG)
	CC = gcc
	STRIP = strip
ifdef DEBUG
		CFLAGS = -Wall -g -DOS_$(OS)
		LDFLAGS = -g
else
		CFLAGS = -Wall -O2 -DOS_$(OS)
		LDFLAGS =
endif
	LIBS = -lusb
else ifeq ($(OS), MACOSX)
	TARGET = $(PROG).dmg
	SDK = /Developer/SDKs/MacOSX10.5.sdk
	ARCH = -mmacosx-version-min=10.5 -arch ppc -arch i386
	CC = gcc
	STRIP = strip
	CFLAGS = -Wall -O2 -DOS_$(OS) -isysroot $(SDK) $(ARCH)
	LIBS = $(ARCH) -Wl,-syslibroot,$(SDK) -framework IOKit -framework CoreFoundation
else ifeq ($(OS), WINDOWS)
	TARGET = $(PROG).exe
	CC = i586-mingw32msvc-gcc
	STRIP = i586-mingw32msvc-strip
	CFLAGS = -Wall -O2 -DOS_$(OS)
	LIBS = -lhid -lsetupapi
endif

OBJS = $(PROG).o hid.o


all: $(TARGET)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)
ifndef DEBUG
	$(STRIP) $(PROG)
endif

$(PROG).exe: $(PROG)
	cp $(PROG) $(PROG).exe

$(PROG).dmg: $(PROG)
	mkdir tmp
	cp $(PROG) tmp
	hdiutil create -ov -volname "GameCube Control Client" -srcfolder tmp $(PROG).dmg

hid.o: hid_$(OS).c hid.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(PROG) $(PROG).exe $(PROG).dmg
	rm -rf tmp

