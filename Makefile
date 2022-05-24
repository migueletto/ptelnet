CC        = m68k-palmos-gcc
AS        = m68k-palmos-as
MULTIGEN  = m68k-palmos-multigen

PILRC     = pilrc
BUILDPRC  = build-prc

CFLAGS    = -O2 -Wall -Wno-switch -palmos5
CSFLAGS   = -O2 -S -Wall -Wno-switch
LDFLAGS   = -L.

PROG      = ptelnet

OBJS      = db.o error.o log.o memo.o misc.o network.o ptelnet.o screen.o serial.o telnet.o tn3270.o vt100.o

all: $(PROG).prc

$(PROG).prc: $(PROG) $(PROG).def res
	$(BUILDPRC) $(PROG).def $(PROG) *.bin

$(PROG)-sections.s $(PROG)-sections.ld: $(PROG).def
	$(MULTIGEN) $(PROG).def

$(PROG)-sections.o: $(PROG)-sections.s
	$(AS) -mno-68881 -m68000 -o $(PROG)-sections.o $(PROG)-sections.s

res: $(PROG).rcp
	$(PILRC) $(PROG).rcp .
	touch res

$(PROG): $(OBJS) $(PROG)-sections.o $(PROG)-sections.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(PROG)-sections.o $(PROG)-sections.ld -lPalmOSGlue libnfm.a libnfm2.a -o $(PROG)

.S.o:
	$(CC) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

clean:
	rm -f *.[osS] $(PROG) *.bin *.grc res $(PROG)-sections.ld $(PROG).prc
