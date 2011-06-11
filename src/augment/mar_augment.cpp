/**
 * @file mar_augment.c
 * 
 * Contains code which is used for augmentation.
 *
 * @author Greg Eddington
 * @todo Change to C by finding a C matrix library
 */

extern "C"
{
  #include "mar_augment.h"
  #include "../common/mar_common.h"
  #include <libconfig.h> 
  #include <float.h>
}

#include <armadillo>

using namespace arma;

/** Whether or not the augmentation module has been initialized @return */
MAR_PRIVATE char mar_augment_initialized = 0;
/** The configuration context used to load and store configuration values @return */
MAR_PRIVATE config_t mar_cfg;
/** The ID of the camera used for augmentation @return */
MAR_PRIVATE mar_camera_id camera_id = MAR_CAM_NO_CAMERA;
/** Whether or not to run the augmentation @return */
MAR_PRIVATE char mar_run_augmentation = 0;

/** Whether or not the MSER have been calculated @return */
MAR_PRIVATE char mar_mser_calculated_this_frame = 0;
/** The MSER @return */
MAR_PRIVATE mar_mser *mar_mser_regions;
/** The number of MSER @return */
MAR_PRIVATE int mar_mser_num_regions;

/** Whether or not the SIFT keypoints have been calculated @return */
MAR_PRIVATE char mar_sift_calculated_this_frame = 0;
/** The SIFT keypoints @return */
MAR_PRIVATE mar_sift_keypoint *mar_sift_keypoints;
/** The number of SIFT keypoints @return */
MAR_PRIVATE int mar_sift_num_keypoints;

/** A MAR library augmentation */
typedef struct
{
  /** The MSER being tracked */
  mar_mser mser;
  /** The affine transformation matrix which transforms points on the initial surface to points on the latest frame's surface */
  fmat::fixed<3, 3> transform;
  /** The affine transformation matrix which transforms points on the latest frame's surface to points on the initial surface */
  fmat::fixed<3, 3> transform_inverse;
  /** A list of SIFT keypoints on the surface in the initial frame */
  mar_sift_keypoint initial_keypoints[MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS];
  /** Number of initial SIFT keypoints */
  int num_initial_keypoints;
  /** The index into the keypoint buffer for new keypoints */
  size_t new_keypoint_cursor;
  /** A list of SIFT keypoints on the surface in the initial frame */
  mar_sift_keypoint potential_keypoints[MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS];
  /** Number of initial SIFT keypoints */
  int num_potential_keypoints;
}
mar_augmentation;

/** The number of augmentations */
MAR_PRIVATE int mar_number_of_augmentations = 0;
/** Whether the augmentation is initialized */
MAR_PRIVATE char mar_augmentation_initialized[MAR_MAX_NUMBER_OF_AUGMENTATIONS] = { 0 };
/** Whether the augmentation exists */
MAR_PRIVATE mar_augmentation mar_augmentations[MAR_MAX_NUMBER_OF_AUGMENTATIONS];
/** Whether the last augmentation was successful or not */
MAR_PRIVATE mar_error_code mar_augmentation_successful[MAR_MAX_NUMBER_OF_AUGMENTATIONS] = { 0 };

/**
 * Initializes augmentation using a configuration file
 *
 * @param filename The filename of the configuration file, NULL for default settings
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 *
 * @todo initialize state variables
 */
MAR_PUBLIC
mar_error_code mar_augment_init(char *filename)
{
  mar_error_code mrv;
  int camera_type = MAR_CAM_DEFAULT_TYPE, 
    camera_format = MAR_CAM_DEFAULT_FORMAT, 
    camera_width = MAR_CAM_DEFAULT_WIDTH, 
    camera_height = MAR_CAM_DEFAULT_HEIGHT, 
    sift_number_of_octaves = MAR_SIFT_DEFAULT_NUMBER_OF_OCTAVES, 
    sift_number_of_levels = MAR_SIFT_DEFAULT_NUMBER_OF_LEVELS, 
    sift_first_octave = MAR_SIFT_DEFAULT_FIRST_OCTAVE;
  const char *camera_dev_name = MAR_CAM_DEFAULT_DEV_NAME;
  double mser_delta = MAR_MSER_DEFAULT_DELTA, 
    mser_min_area = MAR_MSER_DEFAULT_MIN_AREA, 
    mser_max_area = MAR_MSER_DEFAULT_MAX_AREA, 
    mser_min_diversity = MAR_MSER_DEFAULT_MIN_DIVERSITY, 
    mser_max_variation = MAR_MSER_DEFAULT_MAX_VARIATION, 
    sift_peak_threshold = MAR_SIFT_DEFAULT_PEAK_THRESHOLD, 
    sift_edge_threshold = MAR_SIFT_DEFAULT_EDGE_THRESHOLD;

  // Check if augmentation has already been initialized
  if (mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_ALREADY_INITIALIZED;
  }

  config_init(&mar_cfg);

  // Load the config file
  if (filename != NULL)
  {
    if (!config_read_file(&mar_cfg, filename)) 
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&mar_cfg), config_error_line(&mar_cfg), config_error_text(&mar_cfg));
        config_destroy(&mar_cfg);
        return MAR_ERROR_READING_CONFIG;
    }
  }

  // Create camera
  config_lookup_int(&mar_cfg, "camera.camera_type", &camera_type);
  config_lookup_string(&mar_cfg, "camera.dev_name", &camera_dev_name);
  config_lookup_int(&mar_cfg, "camera.camera_format", &camera_format);
  config_lookup_int(&mar_cfg, "camera.camera_width", &camera_width);
  config_lookup_int(&mar_cfg, "camera.camera_height", &camera_height);
  mrv = mar_camera_new(&camera_id, (mar_camera_type)camera_type, (char *)camera_dev_name, (mar_camera_format)camera_format, camera_width, camera_height);
  if (mrv != MAR_ERROR_NONE)
  {
    config_destroy(&mar_cfg);
    return mrv;
  }

  // Create the MSER filter
  mrv = mar_mser_new(camera_width, camera_height);
  if (mrv != MAR_ERROR_NONE)
  {
    mar_camera_free(camera_id);
    config_destroy(&mar_cfg);
    return mrv;
  }

  // Configure the MSER filter
  config_lookup_float(&mar_cfg, "mser.delta", &mser_delta);
  mar_mser_set_delta(mser_delta);

  config_lookup_float(&mar_cfg, "mser.min_area", &mser_min_area);
  mar_mser_set_min_area(mser_min_area);

  config_lookup_float(&mar_cfg, "mser.max_area", &mser_max_area);
  mar_mser_set_max_area(mser_max_area);

  config_lookup_float(&mar_cfg, "mser.min_diversity", &mser_min_diversity);
  mar_mser_set_min_diversity(mser_min_diversity);

  config_lookup_float(&mar_cfg, "mser.max_variation", &mser_max_variation);
  mar_mser_set_max_variation(mser_max_variation);

  // Create the SIFT filter
  config_lookup_int(&mar_cfg, "sift.number_of_octaves", &sift_number_of_octaves);
  config_lookup_int(&mar_cfg, "sift.number_of_levels", &sift_number_of_levels);
  config_lookup_int(&mar_cfg, "sift.first_octave", &sift_first_octave);
  mrv = mar_sift_new(camera_width, camera_height, sift_number_of_octaves, sift_number_of_levels, sift_first_octave);
  if (mrv != MAR_ERROR_NONE)
  {
    mar_camera_free(camera_id);
    mar_mser_free();
    config_destroy(&mar_cfg);
    return mrv;
  }

  // Configure the SIFT filter
  config_lookup_float(&mar_cfg, "sift.peak_threshold", &sift_peak_threshold);
  mar_sift_set_peak_threshold(sift_peak_threshold);
  config_lookup_float(&mar_cfg, "sift.edge_threshold", &sift_edge_threshold);
  mar_sift_set_edge_threshold(sift_edge_threshold);

  // Mark as initialized
  mar_augment_initialized = 1;

  return MAR_ERROR_NONE;
}

/**
 * Initializes augmentation using default settings
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augment_init_from_defaults()
{
  return mar_augment_init(NULL);
}

/**
 * Starts the augmentation camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_start_capture()
{
  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  return mar_camera_start(camera_id);
}

/**
 * Stops the augmentation camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_stop_capture()
{
  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  return mar_camera_stop(camera_id);
}

/**
 * Get's the Euclidian distance between two SIFT keypoints
 *
 * @param k1 The first SIFT keypoint
 * @param k2 The second SIFT keypoint
 * 
 * @return The Euclidian distance between the two, equal to the sum of the difference between all descriptors
 */
MAR_PRIVATE
float get_keypoint_difference(mar_sift_keypoint *k1, mar_sift_keypoint *k2)
{
  float diff = 0;
  int i;

  // Calculate difference
  for (i = 0; i < MAR_SIFT_NBO * MAR_SIFT_NBP * MAR_SIFT_NBP; i++)
  {
    diff += fabs(k1->descriptor[i] - k2->descriptor[i]);
  }

  return diff;
}

/**
 * Checks if a point is within an ellipse
 *
 * @param px The X coordinate of the point
 * @param py The Y coordinate of the point
 * @param ellipse_x The X coordinate of the ellipse
 * @param ellipse_y The Y coordinate of the ellipse
 * @param ellipse_a The semimajor axis of the ellipse
 * @param ellipse_b The semiminor axis of the ellipse
 * @param ellipse_angle The angle of the ellipse
 *
 * @return 1 if the point is within the ellipse, otherwise 0
 */
MAR_PRIVATE
char is_point_in_ellipse(float px, float py, float ellipse_x, float ellipse_y, float ellipse_a, float ellipse_b, float ellipse_angle)
{
  float x = (px-ellipse_x);
  float y = (py-ellipse_y);
  float beta = ellipse_angle * (ellipse_a > ellipse_b ? 1 : -1);
  float sinbeta = sin(beta);
  float cosbeta = cos(beta);
  float rx = cosbeta * x - sinbeta * y;
  float ry = sinbeta * x + cosbeta * y;

  return ((rx*rx)/(ellipse_a*ellipse_a*4))+((ry*ry)/(ellipse_b*ellipse_b*4)) < 1;
}

/**
 * Finds the best match between a given SIFT keypoints and a set of SIFT keypoints.
 * Also checks if the keypoint is distinguishable and unique by checking that it only strongly matches one
 * other keypoint and not multiple keypoints.
 *
 * @param k The SIFT keypoint to match
 * @param keypoints An array of potentially matching keypoints
 * @param num_keypoints The number of keypoints and size of the array keypoints
 * @param best_difference Will be filled with the Euclidian distance of the keypoint k's descriptor and the best keypoint's descriptor
 *
 * @return The index of the best match if the keypoint is distinguishable and unique, meaning that the keypoint matches only one keypoint strongly
 */
MAR_PRIVATE
int get_best_keypoint_match(mar_sift_keypoint *k, mar_sift_keypoint *keypoints, int num_keypoints, float *best_difference)
{
  int i, best_i = -1;
  float difference, best_diff = FLT_MAX, second_best_diff = 0;
  
  // Check the distance between all keypoints and find the best and second best
  for (i = 0; i < num_keypoints; i++)
  {
    difference = get_keypoint_difference(k, &keypoints[i]);

    if (difference < best_diff)
    {
      best_i = i;
      second_best_diff = best_diff;
      best_diff = difference;
    }
    else if (difference < second_best_diff)
    {
      second_best_diff = difference;
    }
  }

  *best_difference = best_diff;

  // Check that the match is unique
  if (best_diff * MAR_UNIQUE_KEYPOINT_THRESHOLD <= second_best_diff)
  {
    return best_i;
  }
 
  return -1;
}

/**
 * Starts the augmentation algorithm.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_start_augmentation()
{
  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  mar_run_augmentation = 1;

  return MAR_ERROR_NONE;
}

/**
 * Stops the augmentation algorithm.
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_stop_augmentation()
{
  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  mar_run_augmentation = 0;

  return MAR_ERROR_NONE;
}

/**
 * Updates an augmentation frame
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 *
 * @todo Add unmatched SIFT keypoints after they appear consecutively so many times
 * @todo Try using all matched keypoints when calculating the transformation matrix
 */
MAR_PUBLIC
mar_error_code mar_augment_update()
{
  mar_error_code mrv;
  int i, j, k, l, m, num_keypoints, matched_keypoints;
  float ox, oy, best_difference = 0, differences[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS];
  float x[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS], y[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS], u[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS], v[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS];
  mar_sift_keypoint *contained_keypoints;

  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Reset state variables
  mar_mser_calculated_this_frame = 0;
  mar_sift_calculated_this_frame = 0;

  // Update the camera
  mrv = mar_camera_update(camera_id);
  if (mrv != MAR_ERROR_NONE)
  {
    return mrv;
  }

  // Check if any augmentations exists
  if (mar_run_augmentation)
  {
    // Update the SIFT filter
    mrv = mar_sift_get_keypoints(&mar_sift_keypoints, &mar_sift_num_keypoints, mar_camera_get_frame_buffer(camera_id));
    if (mrv != MAR_ERROR_NONE)
    {
      return mrv;
    }
    mar_sift_calculated_this_frame = 1;

    // Iterate through all possible augmentations
    for (i = 0; i < MAR_MAX_NUMBER_OF_AUGMENTATIONS; i++)
    {
      // Check if the augmentation exists
      if (mar_augmentation_initialized[i])
      {
        // Find keypoints within the given ellipse
        num_keypoints = 0;
        for (j = 0; j < mar_sift_num_keypoints; j++)
        {
          mar_augment_untransform_point(i, mar_sift_keypoints[j].x, mar_sift_keypoints[j].y, &ox, &oy);
          if (is_point_in_ellipse(ox, oy, mar_augmentations[i].mser.ellipse_x, mar_augmentations[i].mser.ellipse_y, 
                mar_augmentations[i].mser.ellipse_a, mar_augmentations[i].mser.ellipse_b, mar_augmentations[i].mser.ellipse_angle))
          {
            ++num_keypoints;
          }
        }
        
        // Copy the keypoints within the ellipse to a buffer
        contained_keypoints = (mar_sift_keypoint*)malloc(sizeof(mar_sift_keypoint) * num_keypoints);
        num_keypoints = 0;
        for (j = 0; j < mar_sift_num_keypoints; j++)
        {
          mar_augment_untransform_point(i, mar_sift_keypoints[j].x, mar_sift_keypoints[j].y, &ox, &oy);
          if (is_point_in_ellipse(ox, oy, mar_augmentations[i].mser.ellipse_x, mar_augmentations[i].mser.ellipse_y, 
                mar_augmentations[i].mser.ellipse_a, mar_augmentations[i].mser.ellipse_b, mar_augmentations[i].mser.ellipse_angle))
          {
            memcpy(&contained_keypoints[num_keypoints++], &mar_sift_keypoints[j], sizeof(mar_sift_keypoint));
          }
        }

        // Initialize matching variables
        matched_keypoints = 0;
        for (j = 0; j < MAR_MAX_NUM_OF_MATCHED_KEYPOINTS; j++)
        {
          differences[j] = MAR_MAX_KEYPOINT_DIFFERENCE;
        }

        // Iterate through every keypoint within the ellipse
        for (j = 0; j < num_keypoints; j++)
        {
          k = get_best_keypoint_match(&contained_keypoints[j], mar_augmentations[i].initial_keypoints, mar_augmentations[i].num_initial_keypoints, &best_difference);

          // Check if the keypoint uniquely matched an initial keypoint
          if (k != -1)
          {
            // Check if the match is one of the best matches so far
            if (best_difference < differences[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS-1])
            {
              ++matched_keypoints;

              // Find which slot it should be placed in and place it there
              for (l = 0; l < MAR_MAX_NUM_OF_MATCHED_KEYPOINTS; l++)
              {
                if (best_difference < differences[l])
                {
                  for (m = MAR_MAX_NUM_OF_MATCHED_KEYPOINTS-1; m > l; m--)
                  {
                    x[m] = x[m-1];
                    y[m] = y[m-1];
                    u[m] = u[m-1];
                    v[m] = v[m-1];
                    differences[m] = differences[m-1];
                  }

                  x[l] = mar_augmentations[i].initial_keypoints[k].x;
                  y[l] = mar_augmentations[i].initial_keypoints[k].y;
                  u[l] = contained_keypoints[j].x;
                  v[l] = contained_keypoints[j].y;
                  differences[l] = best_difference;
                  
                  break;
                }
              }
            }
          
            // Update the initial keypoints descriptor to the most recent match of it
            memcpy(mar_augmentations[i].initial_keypoints[k].descriptor, contained_keypoints[j].descriptor, 128);
          }
        }

        // If we can't find them in the augmented MSER region, then look in the whole frame to refocus
        if (matched_keypoints < MAR_MIN_NUM_OF_MATCHED_KEYPOINTS)
        {
          // Initialize matching variables
          matched_keypoints = 0;
          for (j = 0; j < MAR_MAX_NUM_OF_MATCHED_KEYPOINTS; j++)
          {
            differences[j] = MAR_MAX_KEYPOINT_DIFFERENCE;
          }

          // Iterate in all the keypoints within the frame
          for (j = 0; j < mar_sift_num_keypoints; j++)
          {
            k = get_best_keypoint_match(&mar_sift_keypoints[j], mar_augmentations[i].initial_keypoints, mar_augmentations[i].num_initial_keypoints, &best_difference);

            // Check if the keypoint uniquely matched an initial keypoint
            if (k != -1)
            {
              // Check if the match is one of the best matches so far
              if (best_difference < differences[MAR_MAX_NUM_OF_MATCHED_KEYPOINTS-1])
              {
                ++matched_keypoints;

                // Find which slot it should be placed in and place it there
                for (l = 0; l < MAR_MAX_NUM_OF_MATCHED_KEYPOINTS; l++)
                {
                  if (best_difference < differences[l])
                  {
                    for (m = MAR_MAX_NUM_OF_MATCHED_KEYPOINTS-1; m > l; m--)
                    {
                      x[m] = x[m-1];
                      y[m] = y[m-1];
                      u[m] = u[m-1];
                      v[m] = v[m-1];
                      differences[m] = differences[m-1];
                    }

                    x[l] = mar_augmentations[i].initial_keypoints[k].x;
                    y[l] = mar_augmentations[i].initial_keypoints[k].y;
                    u[l] = mar_sift_keypoints[j].x;
                    v[l] = mar_sift_keypoints[j].y;
                    differences[l] = best_difference;
                    
                    break;
                  }
                }
              }
            }
          }
        }

        // Free the buffer allocated for contained keypoints
        free(contained_keypoints);

        // Check if a sufficient number of keypoints has been matched
        if (matched_keypoints >= MAR_MIN_NUM_OF_MATCHED_KEYPOINTS)
        {
          fmat A(matched_keypoints*2, 6);
          fmat::fixed<6, 1> T;
          fmat b(matched_keypoints*2, 1);
          
          // Try all possible matrices to find one with acceptable skew and scaling ratio
          for (j = 0; j < matched_keypoints; j++)
          {
            // Fill in the initial matrix
            A(j*2, 0) =     x[j];
            A(j*2, 1) =     y[j];  
            A(j*2, 2) =        0;
            A(j*2, 3) =        0;  
            A(j*2, 4) =        1;
            A(j*2, 5) =        0;  
            A(j*2+1, 0) =      0;
            A(j*2+1, 1) =      0;  
            A(j*2+1, 2) =   x[j];
            A(j*2+1, 3) =   y[j];  
            A(j*2+1, 4) =      0;
            A(j*2+1, 5) =      1;
    
            // Fill in the transformed point matrix
            b(j*2, 0) =   u[0+j];
            b(j*2+1, 0) = v[0+j];
          }

          // Calculate and save transform
          T = pinv(A)*b;

          // Check that the skew is less than the maximum skew 
          // Note that this doesn't account for a large positive skew on one axis and a large negative skew on the other
          if (fabs(T(1, 0)+T(2, 0)) > MAR_AUGMENT_MAX_SKEW)
          {
            /// @todo set to a skew error code
            mar_augmentation_successful[i] = MAR_ERROR_TOO_FEW_MATCHING_KEYPOINTS;
            continue;
          }

          // Check that the scale difference is less than the maximum scale difference (scale ratio = fabs(scale_x - scale_y))
          if (fabs(T(0, 0)-T(3, 0)) > MAR_AUGMENT_MAX_SCALE_RATIO)
          {
            /// @todo set to a scale error code
            mar_augmentation_successful[i] = MAR_ERROR_TOO_FEW_MATCHING_KEYPOINTS;
            continue;
          }

          // Set the transformation matrix
          mar_augmentations[i].transform(0, 0) = T(0, 0);
          mar_augmentations[i].transform(0, 1) = T(1, 0); 
          mar_augmentations[i].transform(0, 2) = T(4, 0); 
          mar_augmentations[i].transform(1, 0) = T(2, 0); 
          mar_augmentations[i].transform(1, 1) = T(3, 0); 
          mar_augmentations[i].transform(1, 2) = T(5, 0); 
          mar_augmentations[i].transform(2, 0) = 0; 
          mar_augmentations[i].transform(2, 1) = 0; 
          mar_augmentations[i].transform(2, 2) = 1; 

          // Mark augmentation as successful
          mar_augmentation_successful[i] = MAR_ERROR_NONE;          
          /// @todo: allow the ability to config whether or not to add points continue;

          // Add new points
          num_keypoints = 0;
          for (j = 0; j < mar_sift_num_keypoints; j++)
          {
            mar_augment_untransform_point(i, mar_sift_keypoints[j].x, mar_sift_keypoints[j].y, &ox, &oy);
            if (is_point_in_ellipse(ox, oy, mar_augmentations[i].mser.ellipse_x, mar_augmentations[i].mser.ellipse_y, 
                  mar_augmentations[i].mser.ellipse_a, mar_augmentations[i].mser.ellipse_b, mar_augmentations[i].mser.ellipse_angle))
            {
              ++num_keypoints;
            }
          }

          // Copy the keypoints within the ellipse to a buffer
          contained_keypoints = (mar_sift_keypoint*)malloc(sizeof(mar_sift_keypoint) * num_keypoints);
          num_keypoints = 0;
          for (j = 0; j < mar_sift_num_keypoints; j++)
          {
            mar_augment_untransform_point(i, mar_sift_keypoints[j].x, mar_sift_keypoints[j].y, &ox, &oy);
            if (is_point_in_ellipse(ox, oy, mar_augmentations[i].mser.ellipse_x, mar_augmentations[i].mser.ellipse_y, 
                  mar_augmentations[i].mser.ellipse_a, mar_augmentations[i].mser.ellipse_b, mar_augmentations[i].mser.ellipse_angle))
            {
              memcpy(&contained_keypoints[num_keypoints++], &mar_sift_keypoints[j], sizeof(mar_sift_keypoint));
            }
          }

          // Iterate through every keypoint within the ellipse
          int new_potential_keypoints = 0;
          for (j = 0; j < num_keypoints && new_potential_keypoints < MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS; j++)
          {
            k = get_best_keypoint_match(&contained_keypoints[j], mar_augmentations[i].initial_keypoints, mar_augmentations[i].num_initial_keypoints, &best_difference);

            // Check if the keypoint uniquely matched an initial keypoint
            if (best_difference > MAR_MAX_KEYPOINT_DIFFERENCE)
            {
              k = get_best_keypoint_match(&contained_keypoints[j], mar_augmentations[i].potential_keypoints, mar_augmentations[i].num_potential_keypoints, &best_difference);

              if (k != -1 && best_difference < MAR_MAX_KEYPOINT_DIFFERENCE)
              {
                memcpy(&mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor], &contained_keypoints[j], sizeof(mar_sift_keypoint));
                mar_augment_untransform_point(i, mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].x, 
                  mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].y, &ox, &oy);
                mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].x = ox;
                mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].y = oy;
                num_keypoints = num_keypoints >= MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS ? MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS : num_keypoints + 1;
                mar_augmentations[i].new_keypoint_cursor = (mar_augmentations[i].new_keypoint_cursor + 1) % MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS;
              }
            }
            else
            {
              memcpy(&mar_augmentations[i].potential_keypoints[new_potential_keypoints++], &contained_keypoints[j], sizeof(mar_sift_keypoint));
            }
          }
          mar_augmentations[i].num_potential_keypoints = new_potential_keypoints;

          // Free the buffer allocated for contained keypoints
          free(contained_keypoints);
        }
        else 
        {
          mar_augmentation_successful[i] = MAR_ERROR_TOO_FEW_MATCHING_KEYPOINTS;
          continue;
        }
      }
    }
  }

  return MAR_ERROR_NONE;
}

/**
 * Loads the transformation matrix for a given augmentation in a 4x4 column major matrix
 *
 * @param id The augmentation's ID
 * @param id Will be filled with the matrices values in a column major ordering.  
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augment_get_transformation(mar_augmentation_id id, float mat[])
{
  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Check if augmentation has not been initialized
  if (!mar_augmentation_initialized[id])
  {
    return MAR_ERROR_AUGMENTATION_ID_DOES_NOT_EXIST;
  }

  // Fill the matrix
  mat[0]  = mar_augmentations[id].transform(0, 0);
  mat[1]  = mar_augmentations[id].transform(1, 0);
  mat[2]  = 0;
  mat[3]  = mar_augmentations[id].transform(2, 0);
  mat[4]  = mar_augmentations[id].transform(0, 1);
  mat[5]  = mar_augmentations[id].transform(1, 1);
  mat[6]  = 0;
  mat[7]  = mar_augmentations[id].transform(2, 1);
  mat[8]  = 0;
  mat[9]  = 0;
  mat[10] = 1;
  mat[11] = 0;
  mat[12] = mar_augmentations[id].transform(0, 2);
  mat[13] = mar_augmentations[id].transform(1, 2);
  mat[14] = 0;
  mat[15] = mar_augmentations[id].transform(2, 2);

  return MAR_ERROR_NONE;
}

/**
 * Returns the last error code for a specific augmentation
 *
 * @param id The augmentation's ID
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augmentation_get_error(mar_augmentation_id id)
{
  return mar_augmentation_successful[id];
}

/**
 * Creates a new augmentation
 *
 * @param id Will be willed in with the augmentation's ID
 * @param region The MSER to track for augmentation
 * 
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augment_new_augmentation(mar_augmentation_id *id, mar_mser *region)
{
  int i, j, num_keypoints;

  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Find a new spot for the augmentation
  for (i = 0; i < MAR_MAX_NUMBER_OF_AUGMENTATIONS; i++)
  {
    if (!mar_augmentation_initialized[i])
    {
      mar_augmentations[i].mser = *region;

      // Copy the keypoints within the ellipse to a buffer
      mar_augmentations[i].new_keypoint_cursor = 0;
      num_keypoints = 0;
      mar_augment_get_keypoints(&mar_sift_keypoints, &mar_sift_num_keypoints);
      for (j = 0; j < mar_sift_num_keypoints; j++)
      {
        if (is_point_in_ellipse(mar_sift_keypoints[j].x, mar_sift_keypoints[j].y, 
              region->ellipse_x, region->ellipse_y, region->ellipse_a, region->ellipse_b, region->ellipse_angle))
        {
          memcpy(&mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor], &mar_sift_keypoints[j], sizeof(mar_sift_keypoint));
          mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].x -= mar_augmentations[i].mser.ellipse_x;
          mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].y -= mar_augmentations[i].mser.ellipse_y;
          mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].x /= 
            (mar_augmentations[i].mser.ellipse_a + mar_augmentations[i].mser.ellipse_b) / 2;
          mar_augmentations[i].initial_keypoints[mar_augmentations[i].new_keypoint_cursor].y /= 
            (mar_augmentations[i].mser.ellipse_a + mar_augmentations[i].mser.ellipse_b) / 2;
          num_keypoints = num_keypoints >= MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS ? MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS : num_keypoints + 1;
          mar_augmentations[i].new_keypoint_cursor = (mar_augmentations[i].new_keypoint_cursor + 1) % MAR_MAX_NUMBER_OF_AUGMENTATION_KEYPOINTS;
        }
      }
      mar_augmentations[i].num_initial_keypoints = num_keypoints;

      // Check if enough keypoints exist to create an augmentation
      if (num_keypoints < MAR_MINIMUM_AUGMENTATION_KEYPOINTS)
      {
        return MAR_ERROR_TOO_FEW_KEYPOINTS;
      }

      // Initialize augmentation
      *id = i;
      mar_number_of_augmentations++;
      mar_augmentation_initialized[i] = 1;

      // Set transformation matrix to the zero matrix
      mar_augmentations[i].transform(0, 0) = 0;
      mar_augmentations[i].transform(0, 1) = 0;
      mar_augmentations[i].transform(0, 2) = 0;
      mar_augmentations[i].transform(1, 0) = 0;
      mar_augmentations[i].transform(1, 1) = 0;
      mar_augmentations[i].transform(1, 2) = 0;
      mar_augmentations[i].transform(2, 0) = 0;
      mar_augmentations[i].transform(2, 1) = 0;
      mar_augmentations[i].transform(2, 2) = 0; 
      mar_augmentations[i].transform_inverse = pinv(mar_augmentations[i].transform);

      return MAR_ERROR_NONE;
    }
  }

  return MAR_ERROR_NO_AUGMENTATION_RESOURCES_AVAILABLE; 
}

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
MAR_PUBLIC
mar_error_code mar_augment_transform_point(mar_augmentation_id id, float x, float y, float *tx, float *ty)
{
  static fmat::fixed<3, 1> xy_mat, uv_mat;

  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Check if augmentation has not been initialized
  if (!mar_augmentation_initialized[id])
  {
    return MAR_ERROR_AUGMENTATION_ID_DOES_NOT_EXIST;
  }

  // Create the XY matrix  
  xy_mat(0, 0) = x;
  xy_mat(1, 0) = y;
  xy_mat(2, 0) = 1;

  uv_mat = mar_augmentations[id].transform * xy_mat;

  *tx = uv_mat(0, 0);
  *ty = uv_mat(1, 0);

  return MAR_ERROR_NONE;
}

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
MAR_PUBLIC
mar_error_code mar_augment_untransform_point(mar_augmentation_id id, float x, float y, float *tx, float *ty)
{
  static fmat::fixed<3, 1> xy_mat, uv_mat;

  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Check if augmentation has not been initialized
  if (!mar_augmentation_initialized[id])
  {
    return MAR_ERROR_AUGMENTATION_ID_DOES_NOT_EXIST;
  }

  // Create the XY matrix  
  xy_mat(0, 0) = x;
  xy_mat(1, 0) = y;
  xy_mat(2, 0) = 1;

  uv_mat = mar_augmentations[id].transform_inverse * xy_mat;

  *tx = uv_mat(0, 0);
  *ty = uv_mat(1, 0);

  return MAR_ERROR_NONE;
}

/**
 * Frees resources for an augmentation
 *
 * @param id The augmentation ID
 */
MAR_PUBLIC
void mar_augment_free_augmentation(mar_augmentation_id id)
{
  if (mar_augmentation_initialized[id])
  {
    mar_augmentation_initialized[id] = 0;
    mar_number_of_augmentations--;
  }
}

/**
 * Returns the maximally stable extremal regions for a camera frame.
 *
 * @param regions A pointer to a pointer which will be modified to point to the array of regions.
 * @param num_regions The number of MSER, and the size of the regions array
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augment_get_regions(mar_mser **regions, int *num_regions)
{
  mar_error_code mrv;

  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Check if we have already calculated MSER this frame - if so then use the cached results
  if (mar_mser_calculated_this_frame)
  {
    *regions = mar_mser_regions;  
    *num_regions = mar_mser_num_regions;
    return MAR_ERROR_NONE;
  }
  else
  {
    // Calculate and return MSER
    mrv = mar_mser_get_regions(&mar_mser_regions, &mar_mser_num_regions, mar_camera_get_frame_buffer(camera_id));
    if (mrv == MAR_ERROR_NONE)
    {
      *regions = mar_mser_regions;  
      *num_regions = mar_mser_num_regions; 
      mar_mser_calculated_this_frame = 1;
    }
    return mrv;
  }
}

/**
 * Returns the SIFT keypoints for a camera frame.
 *
 * @param keypoints A pointer to a pointer which will be modified to point to the array of keypoints.
 * @param num_keypoints The number of SIFT keypoints, and the size of the regions array
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augment_get_keypoints(mar_sift_keypoint **keypoints, int *num_keypoints)
{
  mar_error_code mrv;

  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_ERROR_AUGMENTATION_NOT_INITIALIZED;
  }

  // Check if we have already updated the SIFT filter this frame - if so then use the cached results
  if (mar_sift_calculated_this_frame)
  {
    *keypoints = mar_sift_keypoints;
    *num_keypoints = mar_sift_num_keypoints;
    return MAR_ERROR_NONE;
  }
  else
  {
    // Calculate and return the SIFT keypoints
    mrv = mar_sift_get_keypoints(&mar_sift_keypoints, &mar_sift_num_keypoints, mar_camera_get_frame_buffer(camera_id));
    if (mrv == MAR_ERROR_NONE)
    {
      *keypoints = mar_sift_keypoints;
      *num_keypoints = mar_sift_num_keypoints;
      mar_sift_calculated_this_frame = 1;
    }
    return mrv;
  }
}

/**
 * Gets the camera ID used for augmentation.
 * 
 * @return The MAR library camera ID for the camera being used for augmentation
 */
MAR_PUBLIC
mar_camera_id mar_augment_get_camera()
{
  // Check if augmentation has not been initialized
  if (!mar_augment_initialized)
  {
    return MAR_CAM_NO_CAMERA;
  }

  return camera_id;
}

/**
 * Returns the camera's frame buffer in an RGB24 format.  
 * The frame buffer is 3 * width * height in size.
 * 
 * @return The camera frame buffer.
 */
MAR_PUBLIC
unsigned char *mar_augment_get_camera_frame_buffer()
{
  if (!mar_augment_initialized)
  {
    return NULL;
  }

  return mar_camera_get_frame_buffer(camera_id);
}

/**
 * Frees the augmentation resources
 *
 * @return MAR_ERROR_NONE on success, an error code on failure.
 */
MAR_PUBLIC
mar_error_code mar_augment_free()
{
  int i;

  if (mar_augment_initialized)
  {
    mar_augment_initialized = 0;
    mar_run_augmentation = 0;

    // Free all augmentations
    for (i = 0; i < MAR_MAX_NUMBER_OF_AUGMENTATIONS; i++)
    {
      mar_augment_free_augmentation(i);
    }

    // Free all resources
    config_destroy(&mar_cfg);
    mar_mser_free();
    mar_sift_free();
    mar_camera_stop(camera_id);
    return mar_camera_free(camera_id);
  }

  return MAR_ERROR_NONE;
}

