CFLAGS?=-Wall -Wextra -O2
LIB = libgpios.so
SONUM_MAJOR=0
SONUM_MINOR=1
SONUM_REV=1
LIBDIR?=lib
INCDIR?=include
BINDIR?=bin
VERSIONED_LIB = $(LIB).$(SONUM_MAJOR).$(SONUM_MINOR).$(SONUM_REV)
NAMED_LIB = $(LIB).$(SONUM_MAJOR)
SRC = src/libgpios.c
OBJ = $(SRC:.c=.o)
PREFIX?=/usr/local

.PHONY: all clean examples

all: $(LIB) examples

$(VERSIONED_LIB): $(OBJ)
	 $(CC) -shared $(LDFLAGS) -Wl,-soname,$(NAMED_LIB) -o $@ $^

$(NAMED_LIB): $(VERSIONED_LIB)
	ln -sf  $< $@

$(LIB): $(NAMED_LIB)
	ln -sf  $< $@

include/libgpios.h: include/libgpios.h.in
	sed $< -e "s/GPIOS_VERSION_MAJOR @@/GPIOS_VERSION_MAJOR $(SONUM_MAJOR)/" -e "s/GPIOS_VERSION_MINOR @@/GPIOS_VERSION_MINOR $(SONUM_MINOR)/" -e "s/GPIOS_VERSION_REV @@/GPIOS_VERSION_REV $(SONUM_REV)/" > $@

src/%.o: src/%.c include/libgpios.h
	$(CC) $(CFLAGS) -fPIC -Iinclude -c $< -o $@

examples: examples/gpios-set examples/gpios-get

examples/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS)  -Iinclude $< -o $@ -L. -lgpios $(LDFLAGS)

clean:
	rm -f $(OBJ)  $(LIB) $(NAMED_LIB) $(VERSIONED_LIB) examples/gpios-set examples/gpios-get


install: $(LIB) examples
	install -d $(DESTDIR)$(PREFIX)/$(LIBDIR)
	install -m 0755 $(VERSIONED_LIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)/
	ln -sfr $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(VERSIONED_LIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(NAMED_LIB)
	ln -sfr $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(NAMED_LIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIB)
	install -d $(DESTDIR)$(PREFIX)/$(INCDIR)
	install -m 0644 include/libgpios.h $(DESTDIR)$(PREFIX)/$(INCDIR)/
	install -d $(DESTDIR)$(PREFIX)/$(BINDIR)
	install -m 0755 examples/gpios-get examples/gpios-set $(DESTDIR)$(PREFIX)/$(BINDIR)/
