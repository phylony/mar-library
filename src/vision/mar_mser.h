/**
 * @file mar_mser.h
 * 
 * Contains code which is used for the maximally stable extremal
 * region algorithm.
 *
 * @author Greg Eddington
 */

#ifndef MAR_MSER_H
#define MAR_MSER_H

#include "../common/mar_error.h"

/** The default value for the SIFT filter's delta */
#define MAR_MSER_DEFAULT_DELTA 6
/** The default value for the SIFT filter's min area */
#define MAR_MSER_DEFAULT_MIN_AREA 0.005
/** The default value for the SIFT filter's max area */
#define MAR_MSER_DEFAULT_MAX_AREA 0.4
/** The default value for the SIFT filter's min diversity */
#define MAR_MSER_DEFAULT_MIN_DIVERSITY 0.7
/** The default value for the SIFT filter's max variation */
#define MAR_MSER_DEFAULT_MAX_VARIATION 0.2

/** The VLFeat libraries MSER ellipse's mean X index */
#define MAR_ELLIPSE_MEAN_X      0
/** The VLFeat libraries MSER ellipse's mean Y index */
#define MAR_ELLIPSE_MEAN_Y      1
/** The VLFeat libraries MSER ellipse's X variance index */
#define MAR_ELLIPSE_VARIANCE_X  2
/** The VLFeat libraries MSER ellipse's Y variance index */
#define MAR_ELLIPSE_VARIANCE_Y  4
/** The VLFeat libraries MSER ellipse's covariance index */
#define MAR_ELLIPSE_COVARIANCE  3

/** The default number of MSER region buffer slots */
#define MAR_MSER_DEFAULT_NUMBER_OF_REGIONS 256

/**
 * A type to encompass maximally stable extremal region information
 */
typedef struct
{
  /** The MSER ellipse's center's X coordinate @return Read-Only */
  float ellipse_x;
  /** The MSER ellipse's center's Y coordinate @return Read-Only */
  float ellipse_y;
  /** The MSER ellipse's semimajor axis @return Read-Only */
  float ellipse_a;
  /** The MSER ellipse's semiminor axis @return Read-Only */
  float ellipse_b;
  /** The MSER ellipse's angle of rotation @return Read-Only */
  float ellipse_angle;
}
mar_mser;

/**
 * Creates a new MSER filter.  Must be called before calling other MAR MSER functions.
 *
 * @param width The width of the camera frame
 * @param height The height of the camera frame
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_new(int width, int height);

/**
 * Frees the MSER filter created by mar_mser_new.
 */
void mar_mser_free();

/**
 * Sets the delta value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param delta The filter delta
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_set_delta(float delta);

/**
 * Sets the minimum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param min_area The minimum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_set_min_area(float min_area);

/**
 * Sets the maximum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param max_area The maximum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_set_max_area(float max_area);

/**
 * Sets the max variation value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param max_variation The filter max variation
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_set_max_variation(float max_variation);

/**
 * Sets the minimum diversity value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param min_diversity The filter minimum diversity
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_set_min_diversity(float min_diversity);

/**
 * Calculates and returns the maximally stable extremal regions for a
 * camera frame.
 *
 * @param regions A pointer to a pointer which will be modified to point to the array of regions.
 * @param num_regions The number of MSER, and the size of the regions array
 * @param frame_buffer The camera's frame buffer
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_get_regions(mar_mser **regions, int *num_regions, unsigned char *frame_buffer);

/**
 * Gets the delta value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param delta Will be filled with the filter delta
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_get_delta(float *delta);

/**
 * Gets the minimum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param min_area Will be filled with the minimum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_get_min_area(float *min_area);

/**
 * Gets the maximum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param max_area Will be filled with the maximum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_get_max_area(float *max_area);

/**
 * Gets the max variation value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param max_variation Will be filled with the filter's max variation
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_get_max_variation(float *max_variation);

/**
 * Gets the minimum diversity value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param min_diversity The filter minimum diversity
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_mser_get_min_diversity(float *min_diversity);

#endif
