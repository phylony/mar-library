/**
 * @file mar_augment.h
 * 
 * Contains code which is used for augmentation.
 *
 * @author Greg Eddington
 */

#ifndef MAR_AUGMENT_H
#define MAR_AUGMENT_H

#include "../common/mar_error.h"
#include "../camera/mar_camera.h"
#include "../vision/mar_mser.h"
#include "../vision/mar_sift.h" 

/** Can be used to assign to variable to denote that no augmentation has been assigned to an ID */
#define MAR_NO_AUGMENTATION 255

/** The maximum number of augmentations */
#define MAR_MAX_NUMBER_OF_AUGMENTATIONS 32

/** The maximum difference between two keypoints to be considered matching */
#define MAR_MAX_KEYPOINT_DIFFERENCE 2

/** The maximum number of matched keypoints to use when creating a transformation matrix */
#define MAR_MAX_NUM_OF_MATCHED_KEYPOINTS 256

/** The minimum number of matched keypoints to use when creating a transformation matrix */
#define MAR_MIN_NUM_OF_MATCHED_KEYPOINTS 5

/** The minimum number of keypoints to create an augmentation */
#define MAR_MINIMUM_AUGMENTATION_KEYPOINTS 10

/** The maximum skew for an augmentation to be preferred */
#define MAR_AUGMENT_MAX_SKEW 1000.0f

/** The maximum difference in X scaling and Y scaling for an augmentation to be preferred */
#define MAR_AUGMENT_MAX_SCALE_RATIO 1000.0f

/** 
 * A threshold which defines how close two keypoints must be relative to all keypoints in a set to be considered a unique match.
 * A keypoint match is only considered unique if (smallest_matching_distance * threshold) < second_smallest_matching_distance
 */
#define MAR_UNIQUE_KEYPOINT_THRESHOLD 3.5

#define MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS 512

/** An augmentation identifier */
typedef unsigned char mar_augmentation_id;

/**
 * Initializes augmentation using a configuration file
 *
 * @param filename The filename of the configuration file, NULL for default settings
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_init(char *filename);

/**
 * Initializes augmentation using default settings
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_init_from_defaults();

/**
 * Starts the augmentation camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_start_capture();

/**
 * Stops the augmentation camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_stop_capture();

/**
 * Updates an augmentation frame
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 *
 * @todo Add unmatched SIFT keypoints after they appear consecutively so many times
 * @todo Try using all matched keypoints when calculating the transformation matrix
 */
mar_error_code mar_augment_update();

/**
 * Creates a new augmentation
 *
 * @param id Will be willed in with the augmentation's ID
 * @param region The MSER to track for augmentation
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_new_augmentation(mar_augmentation_id *id, mar_mser *region);

/**
 * Augments a point using the affine transformation matrix of a MAR augmentation.
 *
 * @param id The ID of the augmentation to use
 * @param x The X coordinate of the point
 * @param y The Y coordinate of the point
 * @param tx Will be set to the transformed X coordinate of the point
 * @param ty Will be set to the transformed Y coordinate of the point
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_transform_point(mar_augmentation_id id, float x, float y, float *tx, float *ty);

/**
 * Deaugments a point using the affine transformation matrix of a MAR augmentation.  Changes the transform from the
 * current frame's transform to the initial frame's transform.
 *
 * @param id The ID of the augmentation to use
 * @param x The X coordinate of the point
 * @param y The Y coordinate of the point
 * @param tx Will be set to the untransformed X coordinate of the point
 * @param ty Will be set to the untransformed Y coordinate of the point
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_untransform_point(mar_augmentation_id id, float x, float y, float *tx, float *ty);

/**
 * Frees resources for an augmentation
 *
 * @param id The augmentation ID
 */
void mar_augment_free_augmentation(mar_augmentation_id id);

/**
 * Returns the maximally stable extremal regions for a camera frame.
 *
 * @param regions A pointer to a pointer which will be modified to point to the array of regions.
 * @param num_regions The number of MSER, and the size of the regions array
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_get_regions(mar_mser **regions, int *num_regions);

/**
 * Returns the SIFT keypoints for a camera frame.
 *
 * @param keypoints A pointer to a pointer which will be modified to point to the array of keypoints.
 * @param num_keypoints The number of SIFT keypoints, and the size of the regions array
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_get_keypoints(mar_sift_keypoint **keypoints, int *num_keypoints);

/**
 * Gets the camera ID used for augmentation.
 * 
 * @return The MAR library camera ID for the camera being used for augmentation
 */
mar_camera_id mar_augment_get_camera();

/**
 * Returns the camera's frame buffer in an RGB24 format.  
 * The frame buffer is 3 * width * height in size.
 * 
 * @return The camera frame buffer.
 */
unsigned char *mar_augment_get_camera_frame_buffer();

/**
 * Frees the augmentation resources
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_free();

/**
 * Starts the augmentation algorithm.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_start_augmentation();

/**
 * Stops the augmentation algorithm.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_stop_augmentation();

/**
 * Returns the last error code for a specific augmentation
 *
 * @param id The augmentation's ID
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augmentation_get_error(mar_augmentation_id id);

/**
 * Loads the transformation matrix for a given augmentation in a 4x4 column major matrix
 *
 * @param id The augmentation's ID
 * @param id Will be filled with the matrices values in a column major ordering.  
 *           Array length must be 16.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_augment_get_transformation(mar_augmentation_id id, float mat[]);

#endif
