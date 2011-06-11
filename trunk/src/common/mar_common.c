/**
 * @file mar_common.c
 * 
 * Contains code common across various components of the MAR library.
 *
 * @author Greg Eddington
 */

#include "mar_common.h"

#include <sys/ioctl.h>
#include <errno.h>

/** 
 * Blocks until the ioctl function finishes 
 *
 * @param fd The file descriptor
 * @param request The control request
 * @param arg The request argument
 *
 * @return The returned value of the ioctl call
 */
int mar_block_ioctl(int fd, int request, void *arg)
{
  int r;

  do r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);

  return r;
}
