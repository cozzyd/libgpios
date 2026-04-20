CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2 -fPIC
LFLAGS=-shared
LD=ld
LIB = libgpios.so
SRC = src/libgpios.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean examples

all: $(LIB) examples

$(LIB): $(OBJ)
	 gcc -o $@ $(LFLAGS) $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

examples: examples/gpios-set examples/gpios-get

examples/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) $< -o $@ -L. -lgpios

clean:
	rm -f $(OBJ) $(LIB) examples/blink
