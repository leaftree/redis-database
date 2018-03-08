CC = gcc -g3 -Wall

CCCOLOR   = "\033[33m"
LINKCOLOR = "\033[34;1m"
SRCCOLOR  = "\033[31m"
RMCOLOR   = "\033[1;31m"
BINCOLOR  = "\033[37;1m"
MAKECOLOR = "\033[32;1m"
ENDCOLOR  = "\033[0m"

QUIET_CC  = @printf '%b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_RM  = @printf '%b %b\n' $(LINKCOLOR)REMOVE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_EXE = @printf '%b %b\n' $(LINKCOLOR)EXEC$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LNK = @printf '%b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;

QUIET_PROC=$(QUIET_CC)$(CC)
QUIET_EXEC=$(QUIET_EXE)exec
QUIET_REMOVE=$(QUIET_RM)rm -f
QUIET_LINK=$(QUIET_LNK)gcc

ORAINC=$(ORACLE_HOME)/precomp/public
PROC     = proc
PROCINC  = INCLUDE=$(ORAINC)
PROCFLAG = sqlcheck=full def_sqlcode=yes code=ansi_c lines=yes

ALL_LIBS= $(DB_LIBS) $(OS_LIBS)

CFLAGS += -DDBUG_ON

src  = server.c client.c ora.c util.c

lib = hiredis

target  = server client

all:$(target)

server:server.o ora.o util.o
	$(QUIET_PROC) $(CFLAGS) -I./dbug/inc -I/usr/local/include/cjson $(src) -o $@ -l$(lib) $(ALL_LIBS) -I$(ORAINC) -L/usr/lib -lhiredis -L/usr/local/lib -lcjson -lm -L. -lPbDebug

client:client.o ora.o util.o
	$(QUIET_PROC) $(CFLAGS) -I./dbug/inc -I/usr/local/include/cjson $(src) -o $@ -l$(lib) $(ALL_LIBS) -I$(ORAINC) -L/usr/lib -lhiredis -L/usr/local/lib -lcjson -lm -L. -lPbDebug

cli:
	$(QUIET_EXEC) $(PWD)/$@

ser:
	$(QUIET_EXEC) $(PWD)/$@

ora.c:ora.pc
	$(PROC) $(PROCINC) $(PROCFLAG) INCLUDE=./dbug/inc INAME=ora.pc ONAME=ora.c

tags:$(src)
	ctags *.c *.h

.PHONY:clean
clean:
	$(QUIET_REMOVE) -f $(target)
	$(QUIET_REMOVE) -f core.*
