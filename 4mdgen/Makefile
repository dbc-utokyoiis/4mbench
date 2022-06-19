# Makefile - iotest
#

CC      = gcc
CFLAGS  = -Wall -O2 -g
LDFLAGS = -Wl,--no-as-needed -lm

# CFLAGS += -g

CP = cp -p
RM = rm -f
MKDIR = mkdir -p
CHMOD = chmod

PREFIX = /usr/local

BINS = 4mdgen
HDRS = 4mdgen.h

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

4mdgen: 4mdgen.o
4mdgen.o: 4mdgen.c 4mdgen.h

# end of Makefile
