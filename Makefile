ALL=xe
ZSHCOMP=_xe

CFLAGS=-g -O2 -Wall -Wno-switch -Wextra -Wwrite-strings

DESTDIR?=
PREFIX?=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man
ZSHCOMPDIR=$(PREFIX)/share/zsh/site-functions

all: $(ALL)

clean: FRC
	rm -f $(ALL)

check: FRC all
	prove -v

install: FRC all
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1 $(DESTDIR)$(ZSHCOMPDIR)
	install -m0755 $(ALL) $(DESTDIR)$(BINDIR)
	install -m0644 $(ALL:=.1) $(DESTDIR)$(MANDIR)/man1
	install -m0644 $(ZSHCOMP) $(DESTDIR)$(ZSHCOMPDIR)

README: xe.1
	mandoc -Tutf8 $< | col -bx >$@

FRC:
