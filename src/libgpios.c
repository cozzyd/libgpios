#include "libgpios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

// If compiled with -pthread, _REENTRANT ought to be defined
#ifdef _REENTRANT
#include <pthreads.h>
#endif

#ifndef MAX_CHIPS
#define MAX_CHIPS 16
#endif

#ifndef MAX_LABELS 
#define MAX_LABELS 1024
#endif

#define CHIP_PATH_PREFIX "/dev/gpiochip"


char chip_labels[MAX_CHIPS][64];
static unsigned nchipinfos = 0;
/** 
 *
 * We will chache the gpio_labels, so you only have to look them up once.
 * This is bad if you only need them once but oh well.
 */ 
struct gpio_label
{
  unsigned chip_index;
  unsigned line_index;
  char label[GPIO_MAX_NAME_SIZE];
};



static struct gpio_label gpio_labels[MAX_LABELS];
static unsigned nlabels = 0;

#ifdef _REENTRANT
static pthread_mutex_t cache_mutex = PTHREAD_DEFAULT_INITIALIZER;
static volatile bool cache_built;
#else
static bool cache_built;
#endif


void gpios_drop_cache()
{
	
#ifdef _REENTRANT
  pthread_mutex_lock(&cache_mutex);
#endif

  cache_built = false;
  nchipinfos = 0;
  nlabels = 0;

#ifdef _REENTRANT
  pthread_mutex_unlock(&cache_mutex);
#endif

}

int gpios_build_cache()
{
  if (cache_built)
  {
    return 0;
  }

#ifdef _REENTRANT
  pthread_mutex_lock(&cache_mutex);
  if (cache_built)  //check again in case another thread finished
    return 0;
#endif

    DIR *dir = opendir("/dev");
    if (!dir) return -errno;

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "gpiochip", strlen("gpiochip")) != 0) continue;

        static char chip_path[64];
        snprintf(chip_path, sizeof(chip_path), "/dev/%s", ent->d_name);

        int chip_fd = open(chip_path, O_RDWR);
        if (chip_fd < 0)
        {
          fprintf(stderr,"libgpios warning: Could not open %s\n", chip_path);
           continue;
        }

        strcpy(chip_labels[nchipinfos], chip_path);

        struct gpiochip_info chip_info;
        if (ioctl(chip_fd, GPIO_GET_CHIPINFO_IOCTL, &chip_info) < 0) {
            close(chip_fd);
            continue;
        }

        for (uint32_t i = 0; i < chip_info.lines; i++) {
            struct gpio_v2_line_info line_info = { .offset = i };
            if (ioctl(chip_fd, GPIO_V2_GET_LINEINFO_IOCTL, &line_info) < 0) continue;

            if (*line_info.name)
            {
              gpio_labels[nlabels].chip_index = nchipinfos;
              gpio_labels[nlabels].line_index = i;
              strcpy(gpio_labels[nlabels].label, line_info.name);

              nlabels++;
          
              if (nlabels >= MAX_LABELS)
              {
                fprintf(stderr,"libgpios warning, read %u GPIO labels. If there are any more, they won't be read. Recompile with a higher MAX_LABELS count if you need to\n", MAX_LABELS);

                break;
              }
            }
        }

        close(chip_fd);
        nchipinfos++;
        if (nchipinfos >= MAX_CHIPS)
        {
          fprintf(stderr,"libgpios warning, read %u gpio chips. If there are any more they won't be read. Recompile with a higher MAX_CHIPS count\n", MAX_CHIPS);

          break;
        }
        if (nlabels >= MAX_LABELS) break;
    }
    closedir(dir);


  cache_built = true;
  #ifdef _REENTRANT
  pthread_mutex_unlock(&cache_mutex);
  #endif

  return 0;
}



static const char * consumer_name = "";

void gpios_set_consumer_name(const char * consumer)
{
  if (consumer && *consumer) consumer_name = consumer;
  else consumer = "";
}



int gpios_get_line_by_label(const char *label, gpios_line_t *line, uint32_t flags) {
    if (!label || !line) return -EINVAL;


    gpios_build_cache(); //noop if already built

    // check the cache for the label
    int found_chip_idx = -1;
    size_t found_offset =  0;

    for (unsigned i = 0; i < nlabels; i++)
    {
      if (!strcmp(label, gpio_labels[i].label))
      {
         found_chip_idx = gpio_labels[i].chip_index;
         found_offset = gpio_labels[i].line_index;
         break;
      }
    }

    if (found_chip_idx == -1)
    {
      // check if it looks like a chip:line label, and open the FD if so
      // we do this check now in case there is a label of this form that we would prefero select that  that
      int maybe_chip = -1;
      int maybe_offset = -1;
      if (2 == sscanf(label,"%d:%d", &maybe_chip, &maybe_offset) && maybe_chip >=0 && maybe_offset >=0)
      {
        char chip_name[64];
        sprintf(chip_name,"/dev/gpiochip%d", maybe_chip);
        return gpios_get_line_by_dev_offset(chip_name, maybe_offset, line, flags);
      }
      else
      {
        return -ENOENT;
      }
    }

    return gpios_get_line_by_dev_offset(chip_labels[found_chip_idx], found_offset, line, flags);
}


int gpios_get_line_by_dev_offset(const char * dev, uint32_t offset, gpios_line_t * line, uint32_t flags) {

    int chip_fd = open(dev, O_RDWR);

    if (chip_fd < 0)
    {
       fprintf(stderr,"libgpios warning: Could not open %s\n", dev);
       return -EIO;
    }



    struct gpio_v2_line_request req = {0};
    req.num_lines = 1;
    req.offsets[0] = offset;
    snprintf(req.consumer, sizeof(req.consumer), "libgpios::%s",consumer_name);

    if (flags & GPIOS_OUTPUT) {
        req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    } else {
        req.config.flags = GPIO_V2_LINE_FLAG_INPUT;
    }

    if (flags & GPIOS_ACTIVE_LOW) {
        req.config.flags = GPIO_V2_LINE_FLAG_ACTIVE_LOW;
    } else {
        req.config.flags = GPIO_V2_LINE_FLAG_ACTIVE_LOW;
    }




    if (ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
        int err = -errno;
        close(chip_fd);
        return err;
    }

    // Done with the chip, we have the line's own FD
    close(chip_fd);

    line->fd = req.fd;
    line->offset = 0; // The line FD is a single-line request, its offset in the request context is 0
    return 0;
}

int gpios_set_value(gpios_line_t *line, bool value) {
    if (!line || line->fd < 0) return -EINVAL;

    struct gpio_v2_line_values vals = {0};
    vals.bits = value ? (1ULL << line->offset) : 0;
    vals.mask = (1ULL << line->offset);

    if (ioctl(line->fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &vals) < 0) {
        return -errno;
    }
    return 0;
}

int gpios_get_value(gpios_line_t *line, bool *value) {
    if (!line || line->fd < 0 || !value) return -EINVAL;

    struct gpio_v2_line_values vals = {0};
    vals.mask = (1ULL << line->offset);

    if (ioctl(line->fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &vals) < 0) {
        return -errno;
    }

    *value = (vals.bits & (1ULL << line->offset)) != 0;
    return 0;
}

void gpios_release(gpios_line_t *line) {
    if (line && line->fd >= 0) {
        close(line->fd);
        line->fd = -1;
    }
}
