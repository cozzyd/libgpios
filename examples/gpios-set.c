#include "libgpios.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void usage() 
{
  fprintf(stderr,"Usage: gpios-set (LABEL|chip:offset) VAL=0 [-w WAIT=0]");
}


int main(int nargs, char ** args) {
    gpios_line_t l;

    char * label = NULL;
    int val = 0;
    int wait = 0;
    bool val_set = false;

    for (int iarg = 1; iarg < nargs; iarg++)
    {
      if (!strcmp(args[iarg],"-w") && iarg < nargs-1)
      {
        wait = atoi(args[++iarg]);
      }
      else if (!label) label = args[iarg];
      else if (!val_set)
      {
        val = atoi(args[iarg]);
        val_set = true;
      }
      else
      {
        usage();
      }

    }
    if (!label)
    {
      usage();
    }

    if (gpios_get_line_by_label(label, &l, GPIOS_OUTPUT) < 0) {
        fprintf(stderr, "Error: Could not find or request GPIO line '%s'.\n", label);
        return 1;
    }

    gpios_set_value(&l, val);

    if (wait)
    {
      sleep(wait);
    }

    gpios_release(&l);

    return 0;
}
