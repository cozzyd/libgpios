#include "libgpios.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main(int nargs, char ** args) {
    gpios_line_t l;


    if (nargs < 2)
    {
      fprintf(stderr,"usage: gpios-get (LABEL:chip:offset)");
    }
    if (gpios_get_line_by_label(args[1], &l, 0) < 0) {
        fprintf(stderr, "Error: Could not find or request GPIO line '%s'.\n", args[1]);
        return 1;
    }

    bool val;
    if (gpios_get_value(&l, &val))
    {
      fprintf(stderr,"problem\n");
    }
    else
    {
      printf("%s: %d\n", args[1], val ? 1 : 0);
    }

    gpios_release(&l);

    return 0;
}
