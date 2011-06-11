/**
 * @file mar_camera.c
 *
 * Contains camera interfacing code for the MAR library.
 *
 * @author Greg Eddington
 */

#include "mar_camera.h"

#include "../common/mar_common.h"
#include "mar_v4l2_mmap_camera.h"

/**
 * Contains information for MAR library camera instances.
 */
typedef struct
{
  /** The type of camera @return Do not access directly when using the library */
  mar_camera_type type;
  /** A pointer to the camera data structure @return Do not access directly when using the library */
  void *camera;
}
mar_camera;

/**
 * @return An array of cameras instantiated by the MAR library.
 */
MAR_PRIVATE
mar_camera mar_cameras[MAR_CAM_MAX_NUM_CAMERAS] = { { 0 } };

/**
 * Initializes a MAR camera.
 *
 * @param id A pointer to an int which will be filled with the ID of the newly created camera
 * @param type The type of camera to create
 * @param dev_name The camera device name
 * @param format The camera pixel format
 * @param width The camera resolution width
 * @param height The camera resolution height
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_camera_new(mar_camera_id *id, mar_camera_type type, char *dev_name, mar_camera_format format, int width, int height)
{
  int i;
  mar_error_code retval;

  // Check for an empty camera
  for (i = 0 ; i < MAR_CAM_MAX_NUM_CAMERAS && mar_cameras[i].camera != 0; i++);
  if (i == MAR_CAM_MAX_NUM_CAMERAS)
  {
    // No camera spots available
    return MAR_ERROR_NO_CAMERAS_AVAILABLE;
  }
  *id = i;
  mar_cameras[i].type = type;

  // Create the camera
  switch(type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      retval = mar_v4l2_mmap_camera_new((mar_v4l2_mmap_camera **)&mar_cameras[i].camera, dev_name, format, width, height);
      if (retval != MAR_ERROR_NONE)
      {
        mar_cameras[i].camera = 0;
      }
      return retval;
    default:
      return MAR_ERROR_CAM_TYPE_NOT_SUPPORTED;
  }
}

/**
 * Frees a camera created by the MAR library.
 *
 * @param id The ID of the camera to free
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_free(mar_camera_id id)
{
  mar_error_code retval;

  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      retval = mar_v4l2_mmap_camera_free((mar_v4l2_mmap_camera *)mar_cameras[id].camera);
      mar_cameras[id].camera = 0;
      return retval;
    default:
      mar_cameras[id].camera = 0;
      return MAR_ERROR_CAM_TYPE_NOT_SUPPORTED;
  }
}

/**
 * Starts camera capturing
 *
 * @param id The ID of the camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_start(mar_camera_id id)
{
  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      return mar_v4l2_mmap_camera_start((mar_v4l2_mmap_camera *)mar_cameras[id].camera);
    default:
      return MAR_ERROR_CAM_TYPE_NOT_SUPPORTED;
  }
}

/**
 * Updates the camera and captuers a new frame.
 * 
 * @param id The ID of the camera it update
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_update(mar_camera_id id)
{
  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      return mar_v4l2_mmap_camera_update((mar_v4l2_mmap_camera *)mar_cameras[id].camera);
    default:
      return MAR_ERROR_CAM_TYPE_NOT_SUPPORTED;
  }
}

/**
 * Stops camera capturing
 *
 * @param id The ID of the camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_stop(mar_camera_id id)
{
  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      return mar_v4l2_mmap_camera_stop((mar_v4l2_mmap_camera *)mar_cameras[id].camera);
    default:
      return MAR_ERROR_CAM_TYPE_NOT_SUPPORTED;
  }
}

/**
 * Returns the currently set camera pixel format.
 *
 * @param id The ID of the camera to get the pixel format from
 *
 * @return The camera pixel format.
 */
mar_camera_format mar_camera_get_pixel_format(mar_camera_id id)
{
  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      return mar_v4l2_mmap_camera_get_pixel_format((mar_v4l2_mmap_camera *)mar_cameras[id].camera);
    default:
      return MAR_ERROR_CAM_TYPE_NOT_SUPPORTED;
  }
}

/**
 * Returns the currently set camera resolution.
 *
 * @param id The ID of the camera to get the resolution from
 * @param width Will be filled with the current resolution width.
 * @param height Will be filled with the current resolution height.
 */
void mar_camera_get_resolution(mar_camera_id id, int *width, int *height)
{
  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      mar_v4l2_mmap_camera_get_resolution((mar_v4l2_mmap_camera *)mar_cameras[id].camera, width, height);
      break;
    default:
      *width = 0;
      *height = 0;
  }
}

/**
 * Returns the camera's frame buffer in an RGB24 format.  
 * The frame buffer is 3 * width * height in size.
 *
 * @param id The ID of the camera to get the frame buffer of.
 * 
 * @return The camera frame buffer.
 */
unsigned char *mar_camera_get_frame_buffer(mar_camera_id id)
{
  switch(mar_cameras[id].type)
  {
    case MAR_CAM_TYPE_V4L2_MMAP:
      return mar_v4l2_mmap_camera_get_frame_buffer((mar_v4l2_mmap_camera *)mar_cameras[id].camera);
    default:
      return NULL;
  }
}
