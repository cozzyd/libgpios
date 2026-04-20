#ifndef LIBGPIOS_H
#define LIBGPIOS_H

/*  
 *  ligpios is a conv
 *
 *
 */

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Handle for a GPIO line.
 * Stores the file descriptor of the requested line and its offset.
 */
typedef struct {
    int fd;
    uint32_t offset;
} gpios_line_t;


enum
{
  GPIOS_OUTPUT = 1,
  GPIOS_ACTIVE_LOW = 2
} gpios_e_flags;

/**
 * @brief Find and request a GPIO line by its label
 * Scans all /dev/gpiochip* devices for a line matching the label, caching the
 * result after the first attempt. 
 * 
 * @param label The label string to search for (e.g. "STATUS_LED").
 * @param line Pointer to the handle to be populated.
 * @param as_output If true, requests the line as an output; otherwise as an input.
 * @return 0 on success, negative error code on failure.
 */
int gpios_get_line_by_label(const char *label, gpios_line_t *line, uint32_t flags);

int gpios_get_line_by_dev_offset(const char * dev, uint32_t offset, gpios_line_t * line, uint32_t flags);

/**
 * @brief Set the value of an output GPIO line.
 */
int gpios_set_value(gpios_line_t *line, bool value);

int gpios_poll(gpios_line_t * line);
/**
 * @brief Get the value of a GPIO line.
 */
int gpios_get_value(gpios_line_t *line, bool *value);

/**
 * @brief Close the file descriptor and release the line.
 */
void gpios_release(gpios_line_t *line);

#endif // LIBGPIOS_H
