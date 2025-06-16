# Tools for ASP under ASPTOOLS

TOOLS=		lpcat lpshift planar
RELOCATE=	relocate.o
SCC=		scc.o

LPLIB=		../../asplib
SGLIB=		../../sgb

INC=		$(LPLIB)/liblp
LIB=		$(LPLIB)/liblp/lib
BIN=		../bin

CC=		gcc
CCFLAGS=	-g -I$(INC)

LDFLAGS=	-static -L$(LIB) -llp
SGB_LFLAGS=	-static -L$(SGLIB)/lib -lgb

all: 		$(TOOLS)

lpcat:		$(RELOCATE) $(SCC) lpcat.o
		$(CC) $(RELOCATE) $(SCC) lpcat.o -o lpcat $(LDFLAGS)

lpshift:	$(RELOCATE) $(SCC) lpshift.o
		$(CC) $(RELOCATE) $(SCC) lpshift.o -o lpshift $(LDFLAGS)

planar:		planar.o
		$(CC) planar.o -o planar $(SGB_LFLAGS)

clean:
		rm -f *.o
		rm -f $(TOOLS)

install:	$(TOOLS)
		for t in $(TOOLS);\
		do cp -f ./$$t $(BIN)/$$t;\
		   strip $(BIN)/$$t; done

%.o:            %.c
		$(CC) $(CCFLAGS) -c $*.c
