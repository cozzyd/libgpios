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
	rm -f $(OBJ) $(LIB) examples/blink


install: $(LIB)
