/**
 * @file lighthouse.c
 *
 * Contains a visualization program which lets users visualize the
 * steps of the MAR library.  It is designed specifically for development,
 * testing, and calibration.
 *
 * @author Greg Eddington
 */

#include <mar/camera/mar_camera.h>
#include <mar/vision/mar_mser.h>
#include <mar/vision/mar_sift.h>
#include <mar/augment/mar_augment.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <sys/time.h>
#include <math.h>

/** The GLUT window's width */
static int window_width;
/** The GLUT window's height */
static int window_height;
/** The texture for storing and drawing camera frames */
static GLuint camera_texture;

/** The camera's frame width */
static int camera_width = 320;
/** The camera's frame height */
static int camera_height = 240;
/** The time of the last display, used for calculating animations and FPS */
static struct timeval last_display_time;
/** Show MSER ellipses or not */
static char show_ellipses = 0;
/** Show SIFT keypoints or not */
static char show_keypoints = 0;
/** Show frames per second or not */
static char show_fps = 0;
/** Show selectable regions or not */
static char show_selectable_regions = 1;

/** The mouse X position */
static int mouse_x = 0;
/** The mouse Y position */
static int mouse_y = 0;

/** The augmentation ID */
mar_augmentation_id augmentation_id = MAR_NO_AUGMENTATION;
/** Augmentation X coordinate */
int augmentation_x;
/** Augmentation Y coordinate */
int augmentation_y;
/** Whether or not the augmentation is successful */
char augmentation_successful = 0;

/**
 * A function which subtracts a timeval from another.
 * result = x - y
 *
 * @param result Will be filled in with the difference
 * @param x The minuend
 * @param y The subtrahend
 *
 * @return 1 if the result is negative, 0 if positive
 */
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
  // Perform the carry for the later subtraction by updating y
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  // Compute the time remaining to wait.  tv_usec is certainly positive
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  // Return 1 if result is negative
  return x->tv_sec < y->tv_sec;
}

/**
 * Draws the camera frame.
 *
 * @param texture The ID of the OpenGL texture to use for drawing
 */
void draw_camera_frame(int texture)
{
  // Create the texture
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, camera_width, camera_height, 0, GL_RGB, GL_UNSIGNED_BYTE, mar_augment_get_camera_frame_buffer());
  
  // Draw the texture
  glEnable(GL_TEXTURE_2D);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0); glVertex2d(0,            0);
    glTexCoord2d(1.0, 0.0); glVertex2d(camera_width, 0);
    glTexCoord2d(1.0, 1.0); glVertex2d(camera_width, -camera_height);
    glTexCoord2d(0.0, 1.0); glVertex2d(0,            -camera_height);
  glEnd();
}

/**
 * Draws an ellipse.
 *
 * @param x The center's X coordinate
 * @param y The center's Y coordinate
 * @param a The major axis
 * @param b The minor axis
 * @param angle The angle of rotation
 */
void draw_ellipse(float x, float y, float a, float b, float angle)
{
  int i;

  glPushMatrix();
    glTranslatef(x, y, 0.01f);

    float beta = angle * (a > b ? 1 : -1);
    float sinbeta = sin(beta);
    float cosbeta = cos(beta);

    glDisable(GL_TEXTURE_2D);
    glLineWidth(2.0f);  
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    for (i = 0; i < 360; i += 10) 
    {
      float alpha = i * 3.14159/180.0;
      float sinalpha = sin(alpha);
      float cosalpha = cos(alpha);

      glVertex2f( (a * 2 * cosalpha * cosbeta - b * 2 * sinalpha * sinbeta),
                  (a * 2 * cosalpha * sinbeta + b * 2 * sinalpha * cosbeta) );
    }
    glEnd();
  glPopMatrix();
}

/**
 * Draws a selectable region.
 *
 * @param x The center's X coordinate
 * @param y The center's Y coordinate
 */
void draw_selectable_region(float x, float y)
{
  int i;

  glPushMatrix();
    glTranslatef(x, y, 0.1f);

    glDisable(GL_TEXTURE_2D);
    glLineWidth(3.0f);  
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    for (i = 0; i < 360; i += 10) 
    {
      glVertex2f(cos(i * 3.14159/180.0), sin(i * 3.14159/180.0));
    }
    glEnd();
  glPopMatrix();
}

/**
 * Draws a SIFT keypoint
 *
 * @param x The center's X coordinate
 * @param y The center's Y coordinate
 * @param r The radius
 * @param angle The angle of rotation
 */
void draw_keypoint(float x, float y, float r, float angle)
{
  int i;

  glPushMatrix();
    glTranslatef(x, y, 0.005f);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (i = 0; i < 360; i += 10) 
    {
      glVertex2f(cos(i * 3.14159/180.0) * r, sin(i * 3.14159/180.0) * r);
    }
    glEnd();
    glBegin(GL_LINE);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(cos(angle) * r, sin(angle) * r);
    glEnd();
  glPopMatrix();
}

/**
 * Draw the MSER ellipses
 */
void draw_mser_ellipses()
{
  mar_error_code mrv;
  int i;
  int num_regions = 0;
  mar_mser *regions;

  mrv = mar_augment_get_regions(&regions, &num_regions);
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < num_regions; i++)
  {
    draw_ellipse(regions[i].ellipse_x, -regions[i].ellipse_y, regions[i].ellipse_a, regions[i].ellipse_b, regions[i].ellipse_angle);
  }
}

/**
 * Draw the Selectable Regions
 */
void draw_selectable_regions()
{
  mar_error_code mrv;
  int i;
  int num_regions = 0;
  mar_mser *regions;

  mrv = mar_augment_get_regions(&regions, &num_regions);
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < num_regions; i++)
  {
    draw_selectable_region(regions[i].ellipse_x, -regions[i].ellipse_y);
  }
}

/** 
 * Draw the SIFT keypoints 
 */
void draw_sift_keypoints()
{
  mar_error_code mrv;
  int i;
  mar_sift_keypoint *keypoints;
  int num_keypoints = 0;

  mrv = mar_augment_get_keypoints(&keypoints, &num_keypoints);
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < num_keypoints; i++)
  {
    draw_keypoint(keypoints[i].x, -keypoints[i].y, keypoints[i].radius, keypoints[i].angle);
  }
}

/**
 * Draw the current frames per second
 */
void draw_fps()
{
  static char fps_buffer[512];
  char *p;
  int time_difference_us;
  struct timeval time_difference, cur_display_time;

  glPushMatrix();
    gettimeofday(&cur_display_time, NULL);
    timeval_subtract(&time_difference, &cur_display_time, &last_display_time);
    time_difference_us = time_difference.tv_sec * 1000000 + time_difference.tv_usec;
    last_display_time = cur_display_time;
    snprintf(fps_buffer, 512, "FPS: %5.2f", 1000000.0f / time_difference_us);
    glTranslatef(10.0f, -camera_height + 10.0f, 0.1f);
    glScalef(0.1f, 0.1f, 0.1f);
    glLineWidth(2.0);
    glDisable (GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (p = fps_buffer; *p; p++)
    {
      glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    }
  glPopMatrix();
}

/**
 * Draw a preview of selectable regions
 */
void draw_region_preview()
{
  mar_error_code mrv;
  int i;
  int num_regions = 0;
  mar_mser *regions;

  mrv = mar_augment_get_regions(&regions, &num_regions);
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < num_regions; i++)
  {
    if ((mouse_x - regions[i].ellipse_x) * (mouse_x - regions[i].ellipse_x) + (mouse_y - regions[i].ellipse_y) * (mouse_y - regions[i].ellipse_y) < 200)
    {
      draw_ellipse(regions[i].ellipse_x, -regions[i].ellipse_y, regions[i].ellipse_a, regions[i].ellipse_b, regions[i].ellipse_angle);
      break;
    }
  }
}

/**
 * Draw the augmented virtual image
 */
void draw_augmentation()
{
  float transform_matrix[16];

  if (augmentation_id != MAR_NO_AUGMENTATION && augmentation_successful)
  {
    glPushMatrix();
      mar_augment_get_transformation(augmentation_id, transform_matrix);
      glScalef(1, -1, 1);
      glMultMatrixf(transform_matrix);
      glTranslatef(0, 0, 0.5f);
      glDisable(GL_TEXTURE_2D);
      glLineWidth(5.0f);
      glBegin(GL_LINE_LOOP);
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glVertex2f(-1, -1);
        glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
        glVertex2f( 1, -1);
        glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
        glVertex2f( 1,  1);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex2f(-1,  1);
      glEnd();
      glBegin(GL_LINE);
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glVertex2f(-1, -1);
        glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
        glVertex2f( 1,  1);
      glEnd();
      glBegin(GL_LINE);
        glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
        glVertex2f( 1, -1);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex2f(-1,  1);
      glEnd();
    glPopMatrix();
  }
}

/**
 * Initiates graphics
 */
void initialize_graphics(void)
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE);
  glEnable(GL_COLOR);
  glDepthFunc(GL_LESS);

  // Create the texture
  glGenTextures(1, &camera_texture);
  glBindTexture(GL_TEXTURE_2D, camera_texture);

  // Select texture settings
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

/**
 * GLUT callback called to update the state and draw graphics.
 */
void update_and_display(void)
{
  mar_error_code mrv;

  // Clear frame buffer and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Capture frames
  mrv = mar_augment_update();
  if (mrv != MAR_ERROR_NONE && mrv != MAR_ERROR_AGAIN && mrv != MAR_ERROR_INTERRUPTED && mrv != MAR_ERROR_TOO_FEW_MATCHING_KEYPOINTS)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  augmentation_successful = mrv == MAR_ERROR_NONE;

  // Set up viewing transformation
  glLoadIdentity();
  gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);

  // Transforms to camera coordinates
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(-1.0f, 1.0f, 0.0f);
  glScalef(1.0f / camera_width * 2 , 1.0f / camera_height * 2, 1.0f);

  // Display camera frame
  draw_camera_frame(camera_texture);

  // Draw the MSER filter
  if (show_ellipses)
  {
    draw_mser_ellipses();
  }

  // Draw the MSER filter
  if (show_selectable_regions)
  {
    draw_selectable_regions();
    draw_region_preview();
  }

  // Draw the SIFT filter
  if (show_keypoints)
  {  
    draw_sift_keypoints();
  }

  // Calculate and Draw FPS
  if (show_fps)
  {  
    draw_fps();
  }

  // Show augmentation
  if (mar_augmentation_get_error(augmentation_id) == MAR_ERROR_NONE)
  {
    draw_augmentation();
  }

  // Swap to screen
  glutSwapBuffers();
}

/**
 * GLUT callback called when the window is resized
 *
 * @param width The new window width
 * @param height The new window height
 */
void reshape(GLint width, GLint height)
{
   window_width = width;
   window_height = height;

   glViewport(0, 0, window_width, window_height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, 0, 100);
   glMatrixMode(GL_MODELVIEW);
}

/**
 * GLUT callback for when a mouse button is clicked
 *
 * @param button The mouse button
 * @param state The mouse state
 * @param x The mouse cursor X position
 * @param y The mouse cursor Y position
 */
void mouse_button(int button, int state, int x, int y)
{
  mar_error_code mrv;
  int i;
  int num_regions = 0;
  mar_mser *regions;

  x = (float)x / window_width * camera_width;
  y = (float)y / window_height * camera_height;

  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    mrv = mar_augment_get_regions(&regions, &num_regions);
    if (mrv != MAR_ERROR_NONE)
    {
      fprintf(stderr, "error: ");
      mar_print_error(mrv);
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < num_regions; i++)
    {
      if ((x - regions[i].ellipse_x) * (x - regions[i].ellipse_x) + (y - regions[i].ellipse_y) * (y - regions[i].ellipse_y) < 200)
      {
        if (mar_augment_new_augmentation(&augmentation_id, &regions[i]) == MAR_ERROR_NONE)
        {
          show_selectable_regions = 0;
          augmentation_x = x;
          augmentation_y = y;
          mar_start_augmentation();
          break;
        }
      }
    }
  }
}

/**
 * GLUT callback for when the mouse is moved
 *
 * @param x The mouse cursor X position
 * @param y The mouse cursor Y position
 */
void mouse_motion(int x, int y)
{
  mouse_x = (float)x / window_width * camera_width;
  mouse_y = (float)y / window_height * camera_height;
}

/**
 * GLUT callback for when a keyboard key is pressed
 *
 * @param key The keyboard key pressed
 * @param x The mouse cursor X position
 * @param y The mouse cursor Y position
 */
void keyboard(unsigned char key, int x, int y)
{
  static char mode = ' ';
  float temp_float;
  int temp_int_1, temp_int_2, temp_int_3;

  switch (key)
  {
    case 'v':
      show_selectable_regions = !show_selectable_regions;
      break;
    case 'b':
      show_fps = !show_fps;
      break;
    case 'n':
      show_ellipses = !show_ellipses;
      break;
    case 'm':
      show_keypoints = !show_keypoints;
      break;
    case 'q':
      printf("Editing MSER Delta...\n");
      mode = 'q';
      break;
    case 'w':
      printf("Editing MSER Min Area...\n");
      mode = 'w';
      break;
    case 'e':
      printf("Editing MSER Max Area...\n");
      mode = 'e';
      break;
    case 'r':
      printf("Editing MSER Max Variation...\n");
      mode = 'r';
      break;
    case 't':
      printf("Editing MSER Min Diversity...\n");
      mode = 't';
      break;
    case 'a':
      printf("Editing SIFT Number of Octaves...\n");
      mode = 'a';
      break;
    case 's':
      printf("Editing SIFT Number of Levels...\n");
      mode = 's';
      break;
    case 'd':
      printf("Editing SIFT First Octave...\n");
      mode = 'd';
      break;
    case 'f':
      printf("Editing SIFT Peak Threshold...\n");
      mode = 'f';
      break;
    case 'g':
      printf("Editing SIFT Edge Threshold...\n");
      mode = 'g';
      break;
    case 'j':
      augmentation_x -= 4;
      break;
    case 'l':
      augmentation_x += 4;
      break;
    case 'k':
      augmentation_y += 4;
      break;
    case 'i':
      augmentation_y -= 4;
      break;
    case '-':
      switch (mode)
      {
         case 'q':
          mar_mser_get_delta(&temp_float);
          mar_mser_set_delta(temp_float-1);
          printf("MSER Delta: %f\n", temp_float-1);
          break;
        case 'w':
          mar_mser_get_min_area(&temp_float);
          mar_mser_set_min_area(temp_float-0.01);
          printf("MSER Min Area: %f\n", temp_float-0.01);
          break;
        case 'e':
          mar_mser_get_max_area(&temp_float);
          mar_mser_set_max_area(temp_float-0.01);
          printf("MSER Max Area: %f\n", temp_float-0.01);
          break;
        case 'r':
          mar_mser_get_max_variation(&temp_float);
          mar_mser_set_max_variation(temp_float-0.1);
          printf("MSER Max Variation: %f\n", temp_float-0.1);
          break;
        case 't':
          mar_mser_get_min_diversity(&temp_float);
          mar_mser_set_min_diversity(temp_float-0.1);
          printf("MSER Min Diversity: %f\n", temp_float-0.1);
          break;
        case 'a':
          mar_sift_get_number_of_octaves(&temp_int_1);
          mar_sift_get_number_of_levels(&temp_int_2);
          mar_sift_get_first_octave(&temp_int_3);
          mar_sift_free();
          mar_sift_new(camera_width, camera_height, temp_int_1 - 1, temp_int_2, temp_int_3);
          printf("SIFT Number of Octaves: %d (-1 is MAX)\n", temp_int_1 - 1);
          break;
        case 's':
          mar_sift_get_number_of_octaves(&temp_int_1);
          mar_sift_get_number_of_levels(&temp_int_2);
          mar_sift_get_first_octave(&temp_int_3);
          mar_sift_free();
          mar_sift_new(camera_width, camera_height, temp_int_1, temp_int_2 - 1, temp_int_3);
          printf("SIFT Number of Levels: %d\n", temp_int_2 - 1);
          break;
        case 'd':
          mar_sift_get_number_of_octaves(&temp_int_1);
          mar_sift_get_number_of_levels(&temp_int_2);
          mar_sift_get_first_octave(&temp_int_3);
          mar_sift_free();
          mar_sift_new(camera_width, camera_height, temp_int_1, temp_int_2, temp_int_3 - 1);
          printf("SIFT First Octave: %d\n", temp_int_3 - 1);
          break;
        case 'f':
          mar_sift_get_peak_threshold(&temp_float);
          mar_sift_set_peak_threshold(temp_float - 0.1);
          printf("SIFT Peak Threshold: %f\n", temp_float - 0.1);
          break;
        case 'g':
          mar_sift_get_edge_threshold(&temp_float);
          mar_sift_set_edge_threshold(temp_float - 0.1);
          printf("SIFT Edge Threshold: %f\n", temp_float - 0.1);
          break;
      }
      break;
    case '=':
      switch (mode)
      {
         case 'q':
          mar_mser_get_delta(&temp_float);
          mar_mser_set_delta(temp_float+1);
          printf("MSER Delta: %f\n", temp_float+1);
          break;
        case 'w':
          mar_mser_get_min_area(&temp_float);
          mar_mser_set_min_area(temp_float+0.01);
          printf("MSER Min Area: %f\n", temp_float+0.01);
          break;
        case 'e':
          mar_mser_get_max_area(&temp_float);
          mar_mser_set_max_area(temp_float+0.01);
          printf("MSER Max Area: %f\n", temp_float+0.01);
          break;
        case 'r':
          mar_mser_get_max_variation(&temp_float);
          mar_mser_set_max_variation(temp_float+0.1);
          printf("MSER Max Variation: %f\n", temp_float+0.1);
          break;
        case 't':
          mar_mser_get_min_diversity(&temp_float);
          mar_mser_set_min_diversity(temp_float+0.1);
          printf("MSER Min Diversity: %f\n", temp_float+0.1);
          break;
        case 'a':
          mar_sift_get_number_of_octaves(&temp_int_1);
          mar_sift_get_number_of_levels(&temp_int_2);
          mar_sift_get_first_octave(&temp_int_3);
          mar_sift_free();
          mar_sift_new(camera_width, camera_height, temp_int_1 + 1, temp_int_2, temp_int_3);
          printf("SIFT Number of Octaves: %d (-1 is MAX)\n", temp_int_1 + 1);
          break;
        case 's':
          mar_sift_get_number_of_octaves(&temp_int_1);
          mar_sift_get_number_of_levels(&temp_int_2);
          mar_sift_get_first_octave(&temp_int_3);
          mar_sift_free();
          mar_sift_new(camera_width, camera_height, temp_int_1, temp_int_2 + 1, temp_int_3);
          printf("SIFT Number of Levels: %d\n", temp_int_2 + 1);
          break;
        case 'd':
          mar_sift_get_number_of_octaves(&temp_int_1);
          mar_sift_get_number_of_levels(&temp_int_2);
          mar_sift_get_first_octave(&temp_int_3);
          mar_sift_free();
          mar_sift_new(camera_width, camera_height, temp_int_1, temp_int_2, temp_int_3 + 1);
          printf("SIFT First Octave: %d\n", temp_int_3 + 1);
          break;
        case 'f':
          mar_sift_get_peak_threshold(&temp_float);
          mar_sift_set_peak_threshold(temp_float + 0.1);
          printf("SIFT Peak Threshold: %f\n", temp_float + 0.1);
          break;
        case 'g':
          mar_sift_get_edge_threshold(&temp_float);
          mar_sift_set_edge_threshold(temp_float + 0.1);
          printf("SIFT Edge Threshold: %f\n", temp_float + 0.1);
          break;
      }
      break;
    case 27:  // Escape key
      exit (EXIT_SUCCESS);
  }
}

/**
 * Cleans up the program
 */
void cleanup_lighthouse()
{
  mar_error_code mrv;
  mrv = mar_augment_free();
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
  }
}

/**
 * Entry point of the MAR visualization application
 *
 * @param argc The number of arguments
 * @param argv The arguments
 * 
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise
 */
int main(int argc, char *argv[])
{
  mar_error_code mrv;

  // Initialize the window
  window_width = camera_width;
  window_height = camera_height;

  // GLUT Window Initialization
  glutInit(&argc, argv);
  glutInitWindowSize(window_width, window_height);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow ("Lighthouse");

  // Initialize OpenGL graphics state
  initialize_graphics();

  // Register callbacks
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse_button);
  glutPassiveMotionFunc(mouse_motion);
  glutIdleFunc(update_and_display);
  atexit(cleanup_lighthouse);
 
  // Create the augmentation
  mrv = mar_augment_init("res/lighthouse.cfg");
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  // Start capturing
  mrv = mar_start_capture();
  if (mrv != MAR_ERROR_NONE)
  {
    fprintf(stderr, "error: ");
    mar_print_error(mrv);
    exit(EXIT_FAILURE);
  }

  // Get the initial time
  gettimeofday(&last_display_time, NULL);

  // Turn the flow of control over to GLUT
  glutMainLoop();

  return EXIT_SUCCESS;
}
