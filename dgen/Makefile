# Makefile - iotest
#

CC      = gcc
CFLAGS  = -Wall -O2
LDFLAGS =

# CFLAGS += -g

CP = cp -p
RM = rm -f
MKDIR = mkdir -p
CHMOD = chmod

PREFIX = /usr/local

BINS = dgen
HDRS = dgen.h

#
# Do not change below.
#

export PREFIX

#
# top target
# 

all: build

build: build-top

install: install-top

clean: clean-top

#
# this directory
#

build-top: $(BINS)

install-top: $(BINS)
	@if [ ! -d $(PREFIX)/bin ]; then \
		$(MKDIR) $(PREFIX)/bin;  \
	fi
	$(CP) $(BINS) $(PREFIX)/bin
	@if [ ! -d $(PREFIX)/include ]; then \
		$(MKDIR) $(PREFIX)/include;  \
	fi
	$(CP) $(HDRS) $(PREFIX)/include

clean-top:
	-$(RM) *.o $(BINS)

#
# commands
#

dgen: dgen.o
dgen.o: dgen.c dgen.h

# end of Makefile
