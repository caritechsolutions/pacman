#ifndef UTF8_VALIDATE_H
#define UTF8_VALIDATE_H

#include <stdbool.h>
#include <stddef.h>
#include <gxtype.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief checking if a null-terminated string is properly UTF-8 encoded,
 *
 * @param uint8_t *s: a null-terminated string
 *
 * @return  1: string is UTF-8 encoded
 * @return  0: string is not UTF-8 encoded
 */
extern int utf8_validate_str(uint8_t *s);

#ifdef __cplusplus
}
#endif

#endif
