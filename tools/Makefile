# Tools for ASP under ASPTOOLS

TOOLS=		lpcat
RELOCATE=	relocate.o
SCC=		scc.o

LPLIB=		../../asplib

INC=		$(LPLIB)/liblp
LIB=		$(LPLIB)/liblp/lib
BIN=		../bin

CC=		gcc
CCFLAGS=	-g -I$(INC)

LDFLAGS=	-static -L$(LIB) -llp

all: 		$(TOOLS)

lpcat:		$(RELOCATE) $(SCC) lpcat.o
		$(CC) $(RELOCATE) $(SCC) lpcat.o -o lpcat $(LDFLAGS)

clean:
		rm -f *.o
		rm -f $(TOOLS)

install:	$(TOOLS)
		for t in $(TOOLS);\
		do cp -f ./$$t $(BIN)/$$t;\
		   strip $(BIN)/$$t; done

%.o:            %.c
		$(CC) $(CCFLAGS) -c $*.c
