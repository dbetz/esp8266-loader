MKDIR=mkdir
TOUCH=touch
RM=rm -r -f

ifeq ($(CROSS),)
  PREFIX=
else
  ifeq ($(CROSS),win32)
    PREFIX=i586-mingw32msvc-
    OS=msys
  else
    ifeq ($(CROSS),rpi)
      PREFIX=arm-linux-gnueabihf-
      OS=linux
    else
      echo "Unknown cross compilation selected"
    endif
  endif
endif

CC=$(PREFIX)gcc
CPP=$(PREFIX)g++

CFLAGS=-Wall

ifeq ($(OS),Windows_NT)
OS=msys
endif

ifeq ($(OS),linux)
CFLAGS+=-DLINUX
EXT=
OSINT=$(OBJDIR)/sock_posix.o
LIBS=

else ifeq ($(OS),raspberrypi)
OS=linux
CFLAGS+=-DLINUX -DRASPBERRY_PI
EXT=
OSINT=$(OBJDIR)/sock_posix.o
LIBS=

else ifeq ($(OS),msys)
CFLAGS+=-DMINGW
EXT=.exe
OSINT=$(OBJDIR)/sock_posix.o
LIBS=-lws2_32 -liphlpapi -lsetupapi

else ifeq ($(OS),macosx)
CFLAGS+=-DMACOSX
EXT=
OSINT=$(OBJDIR)/sock_posix.o
LIBS=

else ifeq ($(OS),)
$(error OS not set)

else
$(error Unknown OS $(OS))
endif

BUILD=$(realpath ..)/discover-$(OS)-build

HDRDIR=hdr
SRCDIR=src
OBJDIR=$(BUILD)/obj
BINDIR=$(BUILD)/bin

HDRS=\
$(HDRDIR)/sock.h \

OBJS=\
$(OBJDIR)/discover.o \
$(OSINT)

CFLAGS+=-I$(HDRDIR)
CPPFLAGS=$(CFLAGS)

all:	 $(BINDIR)/discover$(EXT)

$(OBJS):	$(OBJDIR)/created $(HDRS) Makefile

$(BINDIR)/discover$(EXT):	$(BINDIR)/created $(OBJS)
	$(CPP) -o $@ $(OBJS) $(LIBS) -lstdc++

$(BUILD)/%.elf:	%.c
	propeller-elf-gcc -Os -mlmm -o $@ $<
    
$(BUILD)/%.binary:	%.spin
	openspin -o $@ $<

$(BUILD)/%-slow.binary:	%.spin
	openspin -DSLOW -o $@ $<

setup:	$(BUILD)/blink-slow.binary
	propeller-load -e $(BUILD)/blink-slow.binary

run:	$(BINDIR)/discover$(EXT) $(BUILD)/blink.binary
	$(BINDIR)/discover$(EXT) $(BUILD)/blink.binary -t

runbig:	$(BINDIR)/discover$(EXT) $(BUILD)/toggle.elf
	$(BINDIR)/discover$(EXT) $(BUILD)/toggle.elf -t

E:	$(BINDIR)/discover$(EXT) $(BUILD)/blink.binary
	$(BINDIR)/discover$(EXT) $(BUILD)/blink.binary -e

Ebig:	$(BINDIR)/discover$(EXT) $(BUILD)/toggle.elf
	$(BINDIR)/discover$(EXT) $(BUILD)/toggle.elf -e

P:	$(BINDIR)/discover$(EXT)
	$(BINDIR)/discover$(EXT) -P
	
P0:	$(BINDIR)/discover$(EXT)
	$(BINDIR)/discover$(EXT) -P0
	
X:	$(BINDIR)/discover$(EXT)
	$(BINDIR)/discover$(EXT) -X
	
X0:	$(BINDIR)/discover$(EXT)
	$(BINDIR)/discover$(EXT) -X0
	
$(OBJDIR)/%.o:	$(SRCDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o:	$(SRCDIR)/%.cpp $(HDRS)
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(BUILD)

%/created:
	@$(MKDIR) -p $(@D)
	@$(TOUCH) $@
