CFLAGS?=-Wall -Wextra -O2
LIB = libgpios.so
VERSIONED_LIB = libgpios.so.1
SRC = src/libgpios.c
OBJ = $(SRC:.c=.o)
PREFIX?=/usr/local

.PHONY: all clean examples

all: $(LIB) examples

$(VERSIONED_LIB): $(OBJ)
	 $(CC) -shared $(LDFLAGS) -Wl,-soname,$@ -o $@ $^

$(LIB): $(VERSIONED_LIB)
	ln -sf  $(VERSIONED_LIB) $(LIB)


src/%.o: src/%.c include/libgpios.h
	$(CC) $(CFLAGS) -fPIC -Iinclude -c $< -o $@

examples: examples/gpios-set examples/gpios-get

examples/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS)  -Iinclude $< -o $@ -L. -lgpios $(LDFLAGS)

clean:
	rm -f $(OBJ) $(LIB) examples/gpios-set examples/gpios-get


install: $(LIB) examples
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 0755 $(LIB) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 0644 include/libgpios.h $(DESTDIR)$(PREFIX)/include/
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 examples/gpios-get examples/gpios-set $(DESTDIR)$(PREFIX)/bin/
