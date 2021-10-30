
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SRC   = zmrx.c zmtx.c zmdm.c crctab.c unixterm.c unixfile.c
RZOBJ = zmrx.o zmdm.o crctab.o unixterm.o unixfile.o
SZOBJ = zmtx.o zmdm.o crctab.o unixterm.o unixfile.o


CFLAGS := -Wall -Werror -std=c99 -O3

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

ALL = zmtx zmrx

all: $(DEPDIR) $(ALL)

zmrx: $(RZOBJ)
	$(CC) $(CFLAGS) $(RZOBJ) -o zmrx 

zmtx: $(SZOBJ)
	$(CC) $(CFLAGS) $(SZOBJ) -o zmtx 

%.o : %.c $(DEPDIR)/%d | $(DEPDIR)
	$(COMPILE.c) $<

$(DEPDIR): 
	@mkdir -p $@

DEPFILES := $(SRC:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))

clean:
	rm -f *.o $(ALL)
