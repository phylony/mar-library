/**
 * @file mar_camera.h
 *
 * Contains camera interfacing code for the MAR library.
 *
 * @author Greg Eddington
 */

#ifndef MAR_CAMERA_H
#define MAR_CAMERA_H

#include "../common/mar_error.h"

/** The default camera type */
#define MAR_CAM_DEFAULT_TYPE MAR_CAM_TYPE_V4L2_MMAP
/** The default camera pixel format */
#define MAR_CAM_DEFAULT_FORMAT MAR_CAM_FMT_YUYV
/** The default camera width */
#define MAR_CAM_DEFAULT_WIDTH 320
/** The default camera height */
#define MAR_CAM_DEFAULT_HEIGHT 240
/** The default camera device name */
#define MAR_CAM_DEFAULT_DEV_NAME "/dev/video0"

/** The maximum number of cameras */
#define MAR_CAM_MAX_NUM_CAMERAS 2

/** A constant which can be used to initialize camera IDs to specify no camera */
#define MAR_CAM_NO_CAMERA 255

/** \defgroup camera_types Camera Types
 *  @{
 */
/** V4L2 Memory Mapped Devices **/
#define MAR_CAM_TYPE_V4L2_MMAP 1
/** @} */


/** \defgroup camera_pixel_formats Camera Pixel Formats
 *  @{
 */
/** YUYV/YUY2 FourCC Pixel Format **/
#define MAR_CAM_FMT_YUYV 0x01
/** @} */

/**
 * The camera pixel format @return
 */
typedef unsigned char mar_camera_format;

/**
 * The camera type @return
 */
typedef unsigned char mar_camera_type;

/**
 * A camera identifier @return
 */
typedef unsigned char mar_camera_id;

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
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_new(mar_camera_id *id, mar_camera_type type, char *dev_name, mar_camera_format format, int width, int height);

/**
 * Frees a camera created by the MAR library.
 *
 * @param id The ID of the camera to free
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_free(mar_camera_id id);

/**
 * Starts camera capturing
 *
 * @param id The ID of the camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_start(mar_camera_id id);

/**
 * Updates the camera and captuers a new frame.
 * 
 * @param id The ID of the camera it update
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_update(mar_camera_id id);

/**
 * Stops camera capturing
 *
 * @param id The ID of the camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
mar_error_code mar_camera_stop(mar_camera_id id);

/**
 * Returns the currently set camera pixel format.
 *
 * @param id The ID of the camera to get the pixel format from
 *
 * @return The camera pixel format.
 */
mar_camera_format mar_camera_get_pixel_format(mar_camera_id id);

/**
 * Returns the currently set camera resolution.
 *
 * @param id The ID of the camera to get the resolution from
 * @param width Will be filled with the current resolution width.
 * @param height Will be filled with the current resolution height.
 */
void mar_camera_get_resolution(mar_camera_id id, int *width, int *height);

/**
 * Returns the camera's frame buffer in an RGB24 format.  
 * The frame buffer is 3 * width * height in size.
 *
 * @param id The ID of the camera to get the frame buffer of.
 * 
 * @return The camera frame buffer.
 */
unsigned char *mar_camera_get_frame_buffer(mar_camera_id id);

#endif
