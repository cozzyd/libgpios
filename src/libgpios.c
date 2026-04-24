/*
 *  Implementation of ligpios 
 *  Cosmin Deaconu <cozzyd@kicp.uchicago.edu>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */




#include "libgpios.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

// If compiled with -pthread, _REENTRANT ought to be defined
#ifdef _REENTRANT
#include <pthread.h>
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
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool cache_built;

// helper macros so we can concat with line
#define CONCAT2(X,Y) X##Y
#define CONCAT(X,Y) CONCAT2(X,Y)

#define cleanup(X) __attribute__((__cleanup__(X)))

# define lock_guard(X)  \
  pthread_mutex_t * CONCAT(__lockguard__,__LINE__) cleanup(__lock_guard_cleanup)= X; \
  pthread_mutex_lock(X);

//helper function for our lock_guard
static void __lock_guard_cleanup(pthread_mutex_t ** m)
{
  pthread_mutex_unlock(*m);
}

#else
static bool cache_built;
#endif


void gpios_drop_cache()
{
	
#ifdef _REENTRANT
  lock_guard(&cache_mutex);
#endif

  cache_built = false;
  nchipinfos = 0;
  nlabels = 0;

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
        if (flags & GPIOS_OPEN_DRAIN) req.config.flags |= GPIO_V2_LINE_FLAG_OPEN_DRAIN;
        if (flags & GPIOS_OPEN_SOURCE) req.config.flags |= GPIO_V2_LINE_FLAG_OPEN_SOURCE;

    } else {
        req.config.flags = GPIO_V2_LINE_FLAG_INPUT;
        if (flags & GPIOS_POLL_RISING) req.config.flags |= GPIO_V2_LINE_FLAG_EDGE_RISING;
        if (flags & GPIOS_POLL_FALLING) req.config.flags |= GPIO_V2_LINE_FLAG_EDGE_FALLING;
        if (flags & GPIOS_POLL_CLOCK_REALTIME) req.config.flags |= GPIO_V2_LINE_FLAG_EVENT_CLOCK_REALTIME;
    }

    if (flags & GPIOS_ACTIVE_LOW) req.config.flags |= GPIO_V2_LINE_FLAG_ACTIVE_LOW;


    if (ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
        int err = -errno;
        close(chip_fd);
        return err;
    }

    // Done with the chip, we have the line's own FD
    close(chip_fd);

    line->fd = req.fd;
    line->flags = flags;
    return 0;
}

int gpios_set_value(const gpios_line_t *line, bool value) {
    if (!line || line->fd < 0 || !(line->flags & GPIOS_OUTPUT)) return -EINVAL;

    struct gpio_v2_line_values vals = {0};
    vals.bits = value ? 1 : 0;
    vals.mask = 1;

    if (ioctl(line->fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &vals) < 0) {
        return -errno;
    }
    return 0;
}

int gpios_get_value(const gpios_line_t *line, bool *value) {
    if (!line || line->fd < 0 || !value) return -EINVAL;

    struct gpio_v2_line_values vals = {0};
    vals.mask = 1;

    if (ioctl(line->fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &vals) < 0) {
        return -errno;
    }

    *value = (vals.bits & (1)) != 0;
    return 0;
}

void gpios_release(gpios_line_t *line) {
    if (line && (line->fd >= 0) ) {
        close(line->fd);
        line->fd = -1;
    }
}


static int drain_events(int fd, struct gpio_v2_line_event * levent)
{

  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  int nread = 0;
  while (true)
  {
    errno = 0;
    int ret = read(fd, levent, sizeof(*levent));
    if (ret > 0) 
    {
      nread++;
      continue;
    }
    if (ret < 0 && errno == EINTR) continue;
    break;
  }

  fcntl(fd, F_SETFL, flags);

  return nread;

}

static void copy_event(const gpios_line_t * line, const struct gpio_v2_line_event * levent, gpios_event_t * event)
{
    event->when_is_realtime = !!(line->flags & GPIOS_POLL_CLOCK_REALTIME);
    event->rising_edge = levent->id == GPIO_V2_LINE_EVENT_RISING_EDGE;
    event->sequence_number = levent->seqno; 
    event->when.tv_sec = levent->timestamp_ns / 1000000000;
    event->when.tv_nsec = levent->timestamp_ns % 1000000000;
}


int gpios_wait_change (const gpios_line_t * line, gpios_event_t * event, float timeout)
{

  //Make sure we have the right flags set
  if ( !line || (line->fd < 0) ||  ! (line->flags & (GPIOS_POLL_RISING | GPIOS_POLL_FALLING)))
  {
    return -EINVAL;
  }

  struct pollfd pfd =  { .fd = line->fd, .events = POLLIN };
  int timeout_ms = timeout < 0 ? -1 :  timeout >= INT_MAX/1000 ? INT_MAX : timeout * 1000;
  if (timeout && !timeout_ms) timeout_ms = 1;
  struct timespec start;
  if (timeout > 0) clock_gettime(CLOCK_MONOTONIC, &start);
  errno = 0;
  while (true)
  {
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret > 0) break;
    if (ret < 0 && errno == EINTR) 
    {
      if (timeout > 0)
      {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = now.tv_sec - start.tv_sec + 1e-9*(now.tv_nsec - start.tv_nsec);
        if (elapsed >= timeout)
          break;
        timeout_ms -= elapsed*1000;
        memcpy(&start,&now, sizeof(now));
      }


      continue;
    }
    if (ret < 0) return -errno;
    return -ETIMEDOUT;
  }

  if (event)
  {
    struct gpio_v2_line_event levent = {0};
    int nrd = 0;
    while (nrd < (int) sizeof(levent))
    {
      int this_nrd = read(line->fd, ((char*)&levent) + nrd, sizeof(levent)-nrd);
      if (this_nrd < 0)
      {
        if (errno != EINTR)
        {
          return -errno;
        }
        else continue;
      }

      nrd+=this_nrd;
    }


    copy_event(line, &levent, event);

  }

  return 0;
}

int gpios_wait_val(const gpios_line_t * line,bool val, gpios_event_t * event, float timeout)
{
  if (!line || (line->fd < 0 ) || (val && !(line->flags & GPIOS_POLL_RISING)) || (!val && !(line->flags & GPIOS_POLL_FALLING)))
    return -EINVAL;

  bool initial_val;
  struct gpio_v2_line_event levent;
  int nread = drain_events(line->fd, &levent);

  if (gpios_get_value(line, &initial_val))
    return -errno;

  //already at wanted value, return immediately
  if (val == initial_val)
  {
    if (event)
    {
      memset(event, 0, sizeof(*event));
    }

    return 0;
  }

   //otherwise the next change MUST be the one we want ?
  return gpios_wait_change(line, event, timeout);
}
