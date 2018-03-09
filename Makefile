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

INC = -I./dbug/inc -I/usr/local/include/cjson

ALL_LIBS= $(DB_LIBS) $(OS_LIBS)

CFLAGS += -DDBUG_ON

src  = server.c ora.c util.c redisop.c
serverObj = server.o ora.o util.o redisop.o
clientObj = client.o ora.o util.o redisop.o

lib = hiredis

target  = server client

all:$(target)

server:$(serverObj)
	$(QUIET_PROC) $(CFLAGS) $(serverObj) $(INC) -o $@ -l$(lib) $(ALL_LIBS) -L/usr/lib -lhiredis -L/usr/local/lib -lcjson -lm -L. -ldbug

client:$(clientObj)
	$(QUIET_PROC) $(CFLAGS) $(clientObj) $(INC) -o $@ -l$(lib) $(ALL_LIBS) -I$(ORAINC) -L/usr/lib -lhiredis -L/usr/local/lib -lcjson -lm -L. -ldbug

cli:
	$(QUIET_EXEC) $(PWD)/$@

ser:
	$(QUIET_EXEC) $(PWD)/$@

ora.c:ora.pc
	$(PROC) $(PROCINC) $(PROCFLAG) INCLUDE=/usr/local/include/cjson INAME=ora.pc ONAME=ora.c

.c.o:
	$(QUIET_PROC) $(CFLAGS) $(INC) -I$(ORAINC) -c $*.c

tags:$(src)
	ctags *.c *.h

.PHONY:clean
clean:
	$(QUIET_REMOVE) -f $(target)
	$(QUIET_REMOVE) -f core.*
	$(QUIET_REMOVE) -f *.o
