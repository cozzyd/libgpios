CFLAGS =-Wall -Wextra -Iinclude -O2 -fPIC
LFLAGS=-shared
LIB = libgpios.so
SRC = src/libgpios.c
OBJ = $(SRC:.c=.o)
PREFIX?=/usr/local

.PHONY: all clean examples

all: $(LIB) examples

$(LIB): $(OBJ)
	 $(CC) -o $@ $(LFLAGS) $^

src/%.o: src/%.c include/libgpios.h
	$(CC) $(CFLAGS) -c $< -o $@

examples: examples/gpios-set examples/gpios-get

examples/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) $< -o $@ -L. -lgpios

clean:
	rm -f $(OBJ) $(LIB) examples/gpios-set examples/gpios-get


install: $(LIB) examples
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 0755 $(LIB) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 0644 include/libgpios.h $(DESTDIR)$(PREFIX)/include/
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 examples/gpios-get examples/gpios-set $(DESTDIR)$(PREFIX)/bin/
