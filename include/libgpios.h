#ifndef LIBGPIOS_H
#define LIBGPIOS_H

/**
 *  ligpios is a mild convenience wrapper over the linux gpio-v2 chardev interface. Support for legacy interfaces could be added if needed.
 *
 *  The s stands for simple, superfluous, or stupid, depending on your perspective.
 *
 *  This software was originally intended for the RNO-G experiment.
 *
 *  Cosmin Deaconu <cozzyd@kicp.uchicago.edu>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define GPIOS_VERSION_MAJOR 0
#define GPIOS_VERSION_MINOR 1
#define GPIOS_VERSION_REV   0

/**
 * @brief Handle for a GPIO line.
 * Stores the file descriptor of the requested line and flags used to create it.
 * Note that you should not modify this unless you REALLY know what you're doing!
 */
typedef struct {
    int fd;
    uint32_t flags;
    uint8_t reserved[8];
} gpios_line_t;


/**
 * @brief GPIO polling event result 
 */
typedef struct
{
  struct timespec when;
  uint32_t sequence_number;
  uint32_t rising_edge : 1;
  uint32_t when_is_realtime: 1;
} gpios_event_t;

/**
 *
 * @brief flags for gpios
 */
enum gpios_e_flags
{
  GPIOS_OUTPUT              = 1<<0,  // Set up as output
  GPIOS_ACTIVE_LOW          = 1<<1,  // reverse logic high / low from voltage high/low
  GPIOS_POLL_RISING         = 1<<2,  // allow polling on rising (logic high)
  GPIOS_POLL_FALLING        = 1<<3,  // allow polling on falling (logic low)
  GPIOS_POLL_CLOCK_REALTIME = 1<<4,   // when retrieiving events, use realtime instead of monotonic time
  GPIOS_OPEN_DRAIN          = 1<<5,
  GPIOS_OPEN_SOURCE         = 1<<6
};

/**
 * @brief Find and request a GPIO line by its label
 * Scans all /dev/gpiochip* devices for a line matching the label, caching the
 * result after the first attempt.
 * 
 * @param label The label string to search for (e.g. "STATUS_LED"). You can also use CHIPNUM:OFFSET here and it will work (provided nothing provides that label)
 * @param line Pointer to the handle to be populated.
 * @param flags see gpios_e_flags
 * @return 0 on success, negative error code on failure.
 */
int gpios_get_line_by_label(const char *label, gpios_line_t *line, uint32_t flags);

/**
 * @brief Find and request a GPIO line by its chip and offset
 * 
 * @param dev  name of the char device (e.g. /dev/gpiochip0)
 * @param offset ehe offset within the device
 * @param line Pointer to the handle to be populated.
 * @param flags see gpios_e_flags
 * @return 0 on success, negative error code on failure.
 */

int gpios_get_line_by_dev_offset(const char * dev, uint32_t offset, gpios_line_t * line, uint32_t flags);

/**
 * @brief Set the value of an output GPIO line.
 */
int gpios_set_value(const gpios_line_t *line, bool value);


/* @brief Wait for a gpio to change state, in either direction that was set
 *  Requires the right flag was used.
 *  @param line the gpio line
 *  @param event  Filled on success if not NULL
 *  @param timeout timeout in s before giving up (rounded to nearest ms)
 *  @returns 0 on success , negative error code on failure
 */
int gpios_wait_change(const gpios_line_t * line, gpios_event_t *event, float timeout);

/** @brief Polls the gpio until you get desired value
 * 
 * Line must have the correct GPIOS_POLL available. An event may not be returned even on success if the GPIO is already in the correct state, in which case event will just be zeroed.
 * @param line  the gpio line
 * @param val   the value to match
 * @param event  If a transition happens while poling, this will be filled if not NULL. If we return because we are already in the right state, this is zeroed if not NULL
 * @param timeout_ms timeout in seconds before giving up (rounded to nearest ms)
 * @returns 0 on success , negative error code on failure
 */
int gpios_wait_val(const gpios_line_t * line, bool val,  gpios_event_t *event, float timeout);

/**
 * @brief Get the value of a GPIO line configured as an input.
 */
int gpios_get_value(const gpios_line_t *line, bool *value);

/**
 * @brief Close the file descriptor and release the line.
 */
void gpios_release(gpios_line_t *line);


/**
 * @brief Build label cache
 * @returns 0 on seuccess
 */
int gpios_build_cache();

/**
 * @brief Drop label cache
 */
void gpios_drop_cache();

/**
 * @brief Set gpio consumer name
 * @param Name to use when grabbing lines
 */
void gpios_set_consumer_name(const char * name);


#endif // LIBGPIOS_H
