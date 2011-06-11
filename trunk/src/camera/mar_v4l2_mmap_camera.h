/**
 * @file mar_v4l2_mmap_camera.h
 *
 * Contains camera interfacing code for the MAR library for cameras which are
 * V4L2 compliant and are capable of memory mapping.
 *
 * @author Greg Eddington
 */

#ifndef MAR_V4L2_MMAP_CAMERA_H
#define MAR_V4L2_MMAP_CAMERA_H

#include "../common/mar_error.h"
#include "mar_camera.h"
#include <unistd.h>
#include <stdint.h>

/**
 * The maximum number of camera mmap buffers
 */
#define MAR_V4L2_MMAP_CAMERA_MAX_MMAP_BUFFER_NUMBER 4

/**
 * A MAR camera instance for memory mapped V4L2 devices
 */
typedef struct
{
  /** The device file descriptor @return Do not access directly when using the library **/
  int dev_fd;               
  /** The camera pixel format @return Do not access directly when using the library **/
  mar_camera_format format; 
  /** The camera width format @return Do not access directly when using the library **/
  int width;               
  /** The camera height format @return Do not access directly when using the library **/ 
  int height;          
  /** The camera mmap buffers @return Do not access directly when using the library **/ 
  uint8_t *mmap_buffers[MAR_V4L2_MMAP_CAMERA_MAX_MMAP_BUFFER_NUMBER];          
  /** The number of camera mmap buffers @return Do not access directly when using the library **/ 
  int num_mmap_buffers;    
  /** The camera mmap buffer length @return Do not access directly when using the library **/ 
  size_t mmap_buffer_length;
  /** The camera frame buffer @return Do not access directly when using the library **/ 
  uint8_t *frame_buffer;       
  /** The camera mmap buffer length @return Do not access directly when using the library **/ 
  size_t frame_buffer_length;
} 
mar_v4l2_mmap_camera;

/**
 * Initializes the camera portion of MAR.
 *
 * @param cam A pointer to a pointer which will be modified to point at a new camera
 * @param dev_name The camera device
 * @param format The camera pixel format
 * @param width The camera resolution width
 * @param height The camera resolution height
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_v4l2_mmap_camera_new(mar_v4l2_mmap_camera **cam, char *dev_name, mar_camera_format format, int width, int height);

/**
 * Frees a camera created by the MAR library.
 *
 * @param camera The camera to free
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_v4l2_mmap_camera_free(mar_v4l2_mmap_camera *camera);

/**
 * Starts camera capturing
 *
 * @param camera The camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_v4l2_mmap_camera_start(mar_v4l2_mmap_camera *camera);

/**
 * Updates the camera and captuers a new frame.
 * 
 * @param camera The camera to update
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_v4l2_mmap_camera_update(mar_v4l2_mmap_camera *camera);

/**
 * Stops camera capturing
 *
 * @param camera The camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_v4l2_mmap_camera_stop(mar_v4l2_mmap_camera *camera);

/**
 * Returns the currently set camera pixel format.
 *
 * @param camera The camera to get the pixel format from
 *
 * @return The camera pixel format.
 */
mar_camera_format mar_v4l2_mmap_camera_get_pixel_format(mar_v4l2_mmap_camera *camera);

/**
 * Returns the currently set camera resolution.
 *
 * @param camera The camera to get the resolution from
 * @param width Will be filled with the current resolution width.
 * @param height Will be filled with the current resolution height.
 */
void mar_v4l2_mmap_camera_get_resolution(mar_v4l2_mmap_camera *camera, int *width, int *height);

/**
 * Returns the camera's frame buffer in an RGB24 format.  
 * The frame buffer is 3 * width * height in size.
 *
 * @param camera The camera to get the frame buffer of.
 * 
 * @return The camera buffer.
 */
unsigned char *mar_v4l2_mmap_camera_get_frame_buffer(mar_v4l2_mmap_camera *camera);

#endif
