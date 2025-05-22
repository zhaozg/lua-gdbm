# makefile for gdbm library for Lua

# change these to reflect your Lua installation
LUA       ?= luajit
LUA_CFLAGS = $(shell pkg-config --cflags $(LUA))
LUA_LIBS   = $(shell pkg-config --libs $(LUA))

# if your system already has gdbm, this should suffice
GDBMLIB= -lgdbm

# probably no need to change anything below here
CFLAGS= -fPIC -std=gnu99 $(INCS) $(WARN) -O2 $G
WARN= -pedantic -Wall
INCS= $(LUA_CFLAGS)
MAKESO= $(CC) -shared
#MAKESO= env MACOSX_DEPLOYMENT_TARGET=10.3 $(CC) -bundle -undefined dynamic_lookup

MYNAME= gdbm
MYLIB= l$(MYNAME)
T= $(MYNAME).so
OBJS= $(MYLIB).o
TEST= test.lua

all:	test

test:	$T
	$(LUA) $(TEST)

o:	$(MYLIB).o

so:	$T

$T:	$(OBJS)
	$(MAKESO) -o $@ $(OBJS) $(GDBMLIB)

install: $T
	cp $T $(shell pkg-config --variable=INSTALL_CMOD luajit)

clean:
	rm -f $(OBJS) $T core core.* test.gdbm

doc:
	@echo "$(MYNAME) library:"
	@fgrep '/**' $(MYLIB).c | cut -f2 -d/ | tr -d '*' | sort | column

# eof
