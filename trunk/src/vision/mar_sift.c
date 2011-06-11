/**
 * @file mar_sift.c
 * 
 * Contains code which is used for the scale invariant feature transform algorithm.
 *
 * @author Greg Eddington
 */

#include "../common/mar_common.h"
#include "mar_sift.h"

#include <string.h>
#include <sift.h>

/** The SIFT filter @return */
MAR_PRIVATE VlSiftFilt *sift_filter = NULL;
/** The image buffer to store a grayscale of the camera image for SIFT filtering @return */
MAR_PRIVATE float *sift_image_buffer;
/** The image buffer width @return */
MAR_PRIVATE int sift_image_width = 0;
/** The image buffer height @return */
MAR_PRIVATE int sift_image_height = 0;
/** A buffer for holding SIFT keypoints found from filtering @return */
MAR_PRIVATE mar_sift_keypoint *sift_keypoints = NULL;
/** The size of the SIFT keypoint buffer @return */
MAR_PRIVATE int sift_keypoints_size = MAR_SIFT_DEFAULT_NUMBER_OF_KEYPOINTS;

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
MAR_PUBLIC
mar_error_code mar_sift_new(int width, int height, int number_of_octaves, int number_of_levels, int first_octave)
{
  // Create the image buffer
  sift_image_width = width;
  sift_image_height = height;
  sift_image_buffer = malloc(width * height * sizeof(float));
  if (sift_image_buffer == NULL)
  {
    return MAR_ERROR_MALLOC;
  }

  // Create SIFT filter
  sift_filter = vl_sift_new(width, height, number_of_octaves, number_of_levels, first_octave);

  // Create the SIFT keypoint buffer
  sift_keypoints = malloc(sizeof(mar_sift_keypoint) * MAR_SIFT_DEFAULT_NUMBER_OF_KEYPOINTS);
  if (sift_keypoints == NULL)
  {
    return MAR_ERROR_MALLOC;
  }

  return MAR_ERROR_NONE;
}

/**
 * Frees the SIFT filter created by mar_sift_new.
 */
MAR_PUBLIC
void mar_sift_free()
{
  if (sift_image_buffer != NULL)
  {
    free(sift_image_buffer);
    sift_image_buffer = NULL;
  }

  if (sift_filter != NULL)
  {
    vl_sift_delete(sift_filter);
    sift_filter = NULL;
  }

  if (sift_keypoints != NULL)
  {
    free(sift_keypoints); 
    sift_keypoints = NULL;
  }
}

/**
 * Sets the peak threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold The peak threshold. This is the minimum amount of contrast to accept a keypoint
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_set_peak_threshold(float threshold)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  vl_sift_set_peak_thresh(sift_filter, threshold);

  return MAR_ERROR_NONE;
}

/**
 * Sets the edge threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold The edge threshold. This is the edge rejection threshold.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_set_edge_threshold(float threshold)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  vl_sift_set_edge_thresh(sift_filter, threshold);

  return MAR_ERROR_NONE;
}

/**
 * Gets the peak threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold Will be set to the peak threshold. This is the minimum amount of contrast to accept a keypoint
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_get_peak_threshold(float *threshold)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  *threshold = vl_sift_get_peak_thresh(sift_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the edge threshold value for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param threshold Will be set to the edge threshold. This is the edge rejection threshold.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_get_edge_threshold(float *threshold)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  *threshold = vl_sift_get_edge_thresh(sift_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the first octave used by the SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param first_octave Will be set to the first octave used by the SIFT filter.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_get_first_octave(int *first_octave)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  *first_octave = vl_sift_get_octave_first(sift_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the number of octaves for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param number_of_octaves Will be set to the number of octaves.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_get_number_of_octaves(int *number_of_octaves)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  *number_of_octaves = vl_sift_get_noctaves(sift_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the number of levels per octave for SIFT filter.  May only be called if a SIFT filter has been created.
 *
 * @param number_of_levels Will be set to the number of levels per octave of the SIFT filter.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_get_number_of_levels(int *number_of_levels)
{
  if (sift_filter == NULL)
  {
    return MAR_ERROR_SIFT_FILTER_NOT_CREATED;
  }

  *number_of_levels = vl_sift_get_nlevels(sift_filter);

  return MAR_ERROR_NONE;
}

/**
 * Calculates and returns the SIFT keypoints for a camera frame.
 *
 * @param keypoints A pointer to a pointer which will be modified to point to the array of keypoints.
 * @param num_keypoints The number of SIFT keypoints, and the size of the regions array
 * @param frame_buffer The camera's frame buffer
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_sift_get_keypoints(mar_sift_keypoint **keypoints, int *num_keypoints, unsigned char *frame_buffer)
{
  int i, j, sift_status, norientations, num_points;
  VlSiftKeypoint const *points;
  double orientations[4];
  vl_sift_pix descriptors[MAR_SIFT_NBP * MAR_SIFT_NBP * MAR_SIFT_NBO];

  // Build grayscale image
  for (i = 0; i < sift_image_width * sift_image_height; i++)
  {
    sift_image_buffer[i] = (frame_buffer[i*3 + 0] * 0.3 + frame_buffer[i*3 + 1] * 0.59 + frame_buffer[i*3 + 2] * 0.11) / 255.0;
  }

  // Filter the image
  *num_keypoints = 0;
  sift_status = vl_sift_process_first_octave(sift_filter, sift_image_buffer);	
  while (sift_status != VL_ERR_EOF)
  {
    vl_sift_detect(sift_filter);

    // Get the SIFT keypoints
    points = vl_sift_get_keypoints(sift_filter);
    num_points = vl_sift_get_nkeypoints(sift_filter);

    // Check if the buffer is too small
    if (sift_keypoints_size < *num_keypoints + num_points * 4)
    {
      sift_keypoints_size = *num_keypoints + num_points * 4;
      sift_keypoints = realloc(sift_keypoints, sizeof(mar_sift_keypoint) * (*num_keypoints + num_points * 4));
      if (sift_keypoints == NULL)
      {
        return MAR_ERROR_MALLOC;
      }
    }

    // Iterate through the keypoints
    for (i = 0; i < num_points; i++)
    {
        // Iterate through the orientations
        norientations = vl_sift_calc_keypoint_orientations(sift_filter, orientations, &(points[i]));          
        for (j = 0; j < norientations; j++)
        {
            vl_sift_calc_keypoint_descriptor(sift_filter, descriptors, &(points[i]), orientations[j]);
            
            // Add the sift keypoint
            assert(sizeof(vl_sift_pix) == sizeof(float));
            sift_keypoints[*num_keypoints].x = points[i].x;
            sift_keypoints[*num_keypoints].y = points[i].y;
            sift_keypoints[*num_keypoints].radius = points[i].s;
            sift_keypoints[*num_keypoints].angle = points[i].sigma;
            memcpy(sift_keypoints[*num_keypoints].descriptor, descriptors, MAR_SIFT_NBO * MAR_SIFT_NBP * MAR_SIFT_NBP * sizeof(vl_sift_pix));
            (*num_keypoints)++;
        }
    }

    sift_status = vl_sift_process_next_octave(sift_filter);	
  }

  *keypoints = sift_keypoints;

  return MAR_ERROR_NONE;
}
