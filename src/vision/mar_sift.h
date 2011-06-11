/**
 * @file mar_sift.h
 * 
 * Contains code which is used for the scale invariant feature transform algorithm.
 *
 * @author Greg Eddington
 */

#ifndef MAR_SIFT_H
#define MAR_SIFT_H

#include "../common/mar_error.h"

/** The default number of SIFT keypoint buffer slots */
#define MAR_SIFT_DEFAULT_NUMBER_OF_KEYPOINTS 1024
/** Used to specify the number of octaves of the SIFT filter being the maximum number of octaves possible */
#define MAR_SIFT_MAX_OCTAVES -1
/** The default number of octaves used by the SIFT filter. */
#define MAR_SIFT_DEFAULT_NUMBER_OF_OCTAVES MAR_SIFT_MAX_OCTAVES
/** The default number of levels of the SIFT filter. */
#define MAR_SIFT_DEFAULT_NUMBER_OF_LEVELS 3
/** The default first octave of the SIFT filter */
#define MAR_SIFT_DEFAULT_FIRST_OCTAVE 0
/** The default value for the SIFT filter's peak threshold */
#define MAR_SIFT_DEFAULT_PEAK_THRESHOLD 0
/** The default value for the SIFT filter's edge threshold */
#define MAR_SIFT_DEFAULT_EDGE_THRESHOLD 100
/** Number of SIFT bins per orientation */
#define MAR_SIFT_NBO 8
/** Number of SIFT bins per position */
#define MAR_SIFT_NBP 4

/**
 * A type to encompass a SIFT keypoint
 */
typedef struct
{
  /** The SIFT keypoint's center's X coordinate @return Read-Only */
  float x;
  /** The SIFT keypoint's center's Y coordinate @return Read-Only */
  float y;
  /** The SIFT keypoint's radius @return Read-Only */
  float radius;
  /** The SIFT keypoint's angle of rotation @return Read-Only */
  float angle;
  /** The SIFT keypoint's descriptor used for matching */
  float descriptor[MAR_SIFT_NBO * MAR_SIFT_NBP * MAR_SIFT_NBP];
}
mar_sift_keypoint;

/**
 * Creates a new SIFT filter.  Must be called before calling other MAR SIFT functions.
 *
 * @param width The width of the camera frame
 * @param height The height of the camera frame
 * @param number_of_octaves The number of octaves used by the SIFT filter.  
 * Increasing the scale by an octave means doubling the size of the smoothing kernel, 
 * whose effect is roughly equivalent to halving the image resolution. 
 * @param number_of_levels The number of levels used by the SIFT filter.
 * Each octave is sampled at this given number of intermediate scales 
 * Increasing this number might in principle return more refined keypoints, but in practice can make their selection unstable due to noise.
 * @param first_octave The SIFT filter will iterate from first_octave to number_of_octaves
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_new(int width, int height, int number_of_octaves, int number_of_levels, int first_octave);

/**
 * Frees the SIFT filter created by mar_sift_new.
 */
void mar_sift_free();

/**
 * Calculates and returns the SIFT keypoints for a camera frame.
 *
 * @param keypoints A pointer to a pointer which will be modified to point to the array of keypoints.
 * @param num_keypoints The number of SIFT keypoints, and the size of the regions array
 * @param frame_buffer The camera's frame buffer
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_get_keypoints(mar_sift_keypoint **keypoints, int *num_keypoints, unsigned char *frame_buffer);

/**
 * Sets the peak threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold The peak threshold. This is the minimum amount of contrast to accept a keypoint
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_set_peak_threshold(float threshold);

/**
 * Sets the edge threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold The edge threshold. This is the edge rejection threshold.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_set_edge_threshold(float threshold);

/**
 * Gets the peak threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold Will be set to the peak threshold. This is the minimum amount of contrast to accept a keypoint
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_get_peak_threshold(float *threshold);

/**
 * Gets the edge threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold Will be set to the edge threshold. This is the edge rejection threshold.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_get_edge_threshold(float *threshold);

/**
 * Gets the first octave used by the SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param first_octave Will be set to the first octave used by the SIFT filter.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_get_first_octave(int *first_octave);

/**
 * Gets the number of octaves for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param number_of_octaves Will be set to the number of octaves.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_get_number_of_octaves(int *number_of_octaves);

/**
 * Gets the number of levels per octave for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param number_of_levels Will be set to the number of levels per octave of the SIFT filter.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
mar_error_code mar_sift_get_number_of_levels(int *number_of_levels);

#endif
