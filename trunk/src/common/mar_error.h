/**
 * @file mar_error.h
 * 
 * Contains code which is used for error codes of the MAR library.
 *
 * @author Greg Eddington
 */

#ifndef MAR_ERROR_H
#define MAR_ERROR_H

/** An error code for the MAR library @return */
typedef unsigned char mar_error_code;

/** \defgroup mar_error_codes MAR Error Codes
 *  @{
 */
/** no error has occurred */
#define MAR_ERROR_NONE                                  0
/** mmap not supported by the device */
#define MAR_ERROR_MMAP_NOT_SUPPORTED                    1
/** error occurred when requesting device buffers */
#define MAR_ERROR_BUFFER_REQUEST                        2
/** device has insufficient memory */
#define MAR_ERROR_INSUFFICIENT_DEVICE_MEMORY            3
/** error querying buffer */
#define MAR_ERROR_QUERY_BUF                             4
/** error during mmap */
#define MAR_ERROR_MMAP                                  5
/** error during munmap */
#define MAR_ERROR_MUNMAP                                6
/** error during malloc */
#define MAR_ERROR_MALLOC                                7
/** device was not found */
#define MAR_ERROR_DEVICE_NOT_FOUND                      8
/** device was not a character device */
#define MAR_ERROR_NOT_CHARACTER_DEVICE                  9
/** unable to open device */
#define MAR_ERROR_DEVICE_OPEN                           10
/** device is not a V4L2 device */
#define MAR_ERROR_NOT_V4L2_DEVICE                       11
/** device is not a capture device */
#define MAR_ERROR_NOT_VIDEO_CAPTURE_DEVICE              12
/** device is not a streaming device */
#define MAR_ERROR_NOT_STREAMING_DEVICE                  13
/** pixel format is not supported */
#define MAR_ERROR_PIXEL_FORMAT_NOT_SUPPORTED            14
/** unable to close device */
#define MAR_ERROR_DEVICE_CLOSE                          15
/** error queueing buffer */
#define MAR_ERROR_NO_BUFFER_QUEUED                      16
/** unable to turn stream on */
#define MAR_ERROR_NO_STREAM_ON                          17
/** interrupt occurred */
#define MAR_ERROR_INTERRUPTED                           18
/** error during select call */
#define MAR_ERROR_DEVICE_SELECT                         19
/** timed out waiting for camera frame */
#define MAR_ERROR_CAMERA_TIMEOUT                        20
/** try again */
#define MAR_ERROR_AGAIN                                 21
/** error dequeueing buffer */
#define MAR_ERROR_NO_BUFFER_DEQUEUED                    22
/** error turning stream off */
#define MAR_ERROR_STREAM_NOT_OFF                        23
/** camera type is not supported */
#define MAR_ERROR_CAM_TYPE_NOT_SUPPORTED                24
/** no MAR camera resources available */
#define MAR_ERROR_NO_CAMERAS_AVAILABLE                  25
/** no MSER filter created */
#define MAR_ERROR_MSER_FILTER_NOT_CREATED               26
/** no SIFT filter created */
#define MAR_ERROR_SIFT_FILTER_NOT_CREATED               27
/** augmentation already initialized */
#define MAR_ERROR_AUGMENTATION_ALREADY_INITIALIZED      28
/** error reading augmentation config file */
#define MAR_ERROR_READING_CONFIG                        29
/** augmentation not initialized */
#define MAR_ERROR_AUGMENTATION_NOT_INITIALIZED          30
/** too few matching keypoints to create transformation */
#define MAR_ERROR_TOO_FEW_MATCHING_KEYPOINTS            31
/** too few keypoints in region to create new augmentation */
#define MAR_ERROR_TOO_FEW_KEYPOINTS                     32
/** no augmentation resources currently available */
#define MAR_ERROR_NO_AUGMENTATION_RESOURCES_AVAILABLE   33
/** augmentation ID does not exist */
#define MAR_ERROR_AUGMENTATION_ID_DOES_NOT_EXIST        34
/** The number of error codes */
#define MAR_NUMBER_OF_ERRORS                            35
/** @} */

/**
 * Returns a string describing an error code.
 * 
 * @param error The error code.
 * 
 * @return A string describing the error message.
 */
char *mar_error_get_string(mar_error_code error);

/**
 * Prints a string describing an error to standard error.
 * 
 * @param error The error code.
 */
void mar_print_error(mar_error_code error);

#endif
