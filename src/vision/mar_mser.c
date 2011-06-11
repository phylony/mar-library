/**
 * @file mar_mser.c
 * 
 * Contains code which is used for the maximally stable extremal
 * region algorithm.
 *
 * @author Greg Eddington
 */

#include "../common/mar_error.h"
#include "../common/mar_common.h"
#include "mar_mser.h"

#include <mser.h>
#include <stdio.h>
#include <math.h>

/** The MSER filter @return */
MAR_PRIVATE VlMserFilt *mser_filter = NULL;
/** The image buffer to store a grayscale of the camera image for MSER filtering @return */
MAR_PRIVATE unsigned char *mser_image_buffer;
/** The image buffer width @return */
MAR_PRIVATE int mser_image_width = 0;
/** The image buffer height @return */
MAR_PRIVATE int mser_image_height = 0;
/** A buffer for holding MSER ellipses found from filtering @return */
MAR_PRIVATE mar_mser *mser_regions = NULL;
/** The size of the MSER region buffer @return */
MAR_PRIVATE int mser_regions_size = MAR_MSER_DEFAULT_NUMBER_OF_REGIONS;

/**
 * Creates a new MSER filter.  Must be called before calling other MAR MSER functions.
 *
 * @param width The width of the camera frame
 * @param height The height of the camera frame
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_new(int width, int height)
{
  int mser_dimensions[2];

  // Create the image buffer
  mser_image_width = width;
  mser_image_height = height;
  mser_image_buffer = malloc(width * height);
  if (mser_image_buffer == NULL)
  {
    return MAR_ERROR_MALLOC;
  }

  // Create the filter
  mser_dimensions[0] = width;
  mser_dimensions[1] = height;
  mser_filter = vl_mser_new(2, mser_dimensions);

  // Create the MSER region buffer
  mser_regions = malloc(sizeof(mar_mser) * MAR_MSER_DEFAULT_NUMBER_OF_REGIONS);
  if (mser_regions == NULL)
  {
    return MAR_ERROR_MALLOC;
  }

  return MAR_ERROR_NONE;
}

/**
 * Frees the MSER filter created by mar_mser_new.
 */
MAR_PUBLIC
void mar_mser_free()
{
  if (mser_image_buffer != NULL)
  {
    free(mser_image_buffer);
    mser_image_buffer = NULL;
  }

  if (mser_filter != NULL)
  {
    vl_mser_delete(mser_filter);
    mser_filter = NULL;
  }

  if (mser_regions != NULL)
  {
    free(mser_regions);
    mser_regions = NULL;
  }
}

/**
 * Sets the delta value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param delta The filter delta
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_set_delta(float delta)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  vl_mser_set_delta(mser_filter, delta);

  return MAR_ERROR_NONE;
}

/**
 * Sets the minimum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param min_area The minimum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_set_min_area(float min_area)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  vl_mser_set_min_area(mser_filter, min_area);

  return MAR_ERROR_NONE;
}

/**
 * Sets the maximum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param max_area The maximum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_set_max_area(float max_area)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  vl_mser_set_max_area(mser_filter, max_area);

  return MAR_ERROR_NONE;
}

/**
 * Sets the max variation value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param max_variation The filter max variation
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_set_max_variation(float max_variation)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  vl_mser_set_max_variation(mser_filter, max_variation);

  return MAR_ERROR_NONE;
}

/**
 * Sets the minimum diversity value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param min_diversity The filter minimum diversity
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_set_min_diversity(float min_diversity)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  vl_mser_set_min_diversity(mser_filter, min_diversity);

  return MAR_ERROR_NONE;
}

/**
 * Gets the delta value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param delta Will be filled with the filter delta
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_get_delta(float *delta)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  *delta = vl_mser_get_delta(mser_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the minimum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param min_area Will be filled with the minimum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_get_min_area(float *min_area)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  *min_area = vl_mser_get_min_area(mser_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the maximum area for MSER filter regions.  May only be called if a MSER filter has been created.
 *
 * @param max_area Will be filled with the maximum region area, normalized to [0-1], where one is the total area of the image.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_get_max_area(float *max_area)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  *max_area = vl_mser_get_max_area(mser_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the max variation value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param max_variation Will be filled with the filter's max variation
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_get_max_variation(float *max_variation)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  *max_variation = vl_mser_get_max_variation(mser_filter);

  return MAR_ERROR_NONE;
}

/**
 * Gets the minimum diversity value for MSER filter.  May only be called if a MSER filter has been created.
 *
 * @param min_diversity The filter minimum diversity
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_mser_get_min_diversity(float *min_diversity)
{
  if (mser_filter == NULL)
  {
    return MAR_ERROR_MSER_FILTER_NOT_CREATED;
  }

  *min_diversity = vl_mser_get_min_diversity(mser_filter);

  return MAR_ERROR_NONE;
}

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
MAR_PUBLIC
mar_error_code mar_mser_get_regions(mar_mser **regions, int *num_regions, unsigned char *frame_buffer)
{
  int i, j;
  float xx, yy, xy;
  vl_uint const *regs;
  float const *ellipsoids;
  VlMserStats const *stats;

  // Build grayscale image
  for (i = 0; i < mser_image_width * mser_image_height; i++)
  {
    mser_image_buffer[i] = frame_buffer[i*3 + 0] * .3 + frame_buffer[i*3 + 1] * .59 + frame_buffer[i*3 + 2] * .11;
  }

  // Process the image
  vl_mser_process(mser_filter, mser_image_buffer);	
  vl_mser_ell_fit(mser_filter);
  *num_regions = vl_mser_get_regions_num(mser_filter);
  regs = vl_mser_get_regions(mser_filter);   
  ellipsoids = vl_mser_get_ell(mser_filter);
  stats = vl_mser_get_stats(mser_filter);

  // Check if the array needs to be expanded
  if (*num_regions > mser_regions_size) 
  {
    mser_regions_size = *num_regions;
    mser_regions = realloc(mser_regions, sizeof(mar_mser) * mser_regions_size);
    if (mser_regions == NULL)
    {
      return MAR_ERROR_MALLOC;
    }
  }
  
  // Create the ellipses
  for (i = 0; i < *num_regions; i++)
  {
    mser_regions[i].ellipse_x = ellipsoids[i*5+MAR_ELLIPSE_MEAN_X];
    mser_regions[i].ellipse_y = ellipsoids[i*5+MAR_ELLIPSE_MEAN_Y];
    xx = ellipsoids[i*5+MAR_ELLIPSE_VARIANCE_X];
    yy = ellipsoids[i*5+MAR_ELLIPSE_VARIANCE_Y];
    xy = ellipsoids[i*5+MAR_ELLIPSE_COVARIANCE];
    mser_regions[i].ellipse_angle = -1 * 0.5 * atan2f(2*xy, xx-yy);
    mser_regions[i].ellipse_a = sqrt(0.5 * (xx + yy + sqrt((xx - yy) * (xx - yy) + 4 * xy * xy)));
    mser_regions[i].ellipse_b = sqrt(0.5 * (xx + yy - sqrt((xx - yy) * (xx - yy) + 4 * xy * xy)));
  }

  // Build inverse grayscale image
  for (j = 0; j < mser_image_width * mser_image_height; j++)
  {
    mser_image_buffer[j] = ~mser_image_buffer[j];
  }

  // Process the image
  vl_mser_process(mser_filter, mser_image_buffer);	
  vl_mser_ell_fit(mser_filter);
  *num_regions += vl_mser_get_regions_num(mser_filter);
  regs = vl_mser_get_regions(mser_filter);   
  ellipsoids = vl_mser_get_ell(mser_filter);
  stats = vl_mser_get_stats(mser_filter);

  // Check if the array needs to be expanded
  if (*num_regions > mser_regions_size) 
  {
    mser_regions_size = *num_regions;
    mser_regions = realloc(mser_regions, sizeof(mar_mser) * mser_regions_size);
    if (mser_regions == NULL)
    {
      return MAR_ERROR_MALLOC;
    }
  }
  
  // Create the ellipses
  for (j = i; j < *num_regions; j++)
  {
    mser_regions[j].ellipse_x = ellipsoids[j*5+MAR_ELLIPSE_MEAN_X];
    mser_regions[j].ellipse_y = ellipsoids[j*5+MAR_ELLIPSE_MEAN_Y];
    xx = ellipsoids[j*5+MAR_ELLIPSE_VARIANCE_X];
    yy = ellipsoids[j*5+MAR_ELLIPSE_VARIANCE_Y];
    xy = ellipsoids[j*5+MAR_ELLIPSE_COVARIANCE];
    mser_regions[j].ellipse_angle = -1 * 0.5 * atan2f(2*xy, xx-yy);
    mser_regions[j].ellipse_a = sqrt(0.5 * (xx + yy + sqrt((xx - yy) * (xx - yy) + 4 * xy * xy)));
    mser_regions[j].ellipse_b = sqrt(0.5 * (xx + yy - sqrt((xx - yy) * (xx - yy) + 4 * xy * xy)));
  }

  *regions = mser_regions;

  return MAR_ERROR_NONE;
}
