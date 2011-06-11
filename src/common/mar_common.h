/**
 * @file mar_common.h
 * 
 * Contains code common across various components of the MAR library.
 *
 * @author Greg Eddington
 */

#ifndef MAR_COMMON_H
#define MAR_COMMON_H

#include <string.h>

/** Sets a variable or all fields of a structure to zero */
#define MAR_CLEAR(x) memset (&(x), 0, sizeof (x))

/** \defgroup function_visibility Function Visibility
 *  @{
 */
/** A function private to a file */
#define MAR_PRIVATE static

/** A public function */
#define MAR_PUBLIC  
/** @} */

/** 
 * Blocks until the ioctl function finishes 
 *
 * @param fd The file descriptor
 * @param request The control request
 * @param arg The request argument
 *
 * @return The returned value of the ioctl call
 */
int mar_block_ioctl(int fd, int request, void *arg);

#endif
