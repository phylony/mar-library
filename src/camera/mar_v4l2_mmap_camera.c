/**
 * @file mar_v4l2_mmap_camera.c
 *
 * Contains camera interfacing code for the MAR library for cameras which are
 * V4L2 compliant and are capable of memory mapping.
 *
 * @author Greg Eddington
 */

#include "mar_v4l2_mmap_camera.h"
#include "../common/mar_common.h"
#include "../common/mar_error.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

/**
 * Initializes the camera to use memory mapping.
 *
 * @param camera The camera to initialize
 *
 * @return An MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PRIVATE
mar_error_code mar_v4l2_mmap_camera_mmap_init(mar_v4l2_mmap_camera *camera)
{
  struct v4l2_requestbuffers req;

  MAR_CLEAR (req);
  req.count               = MAR_V4L2_MMAP_CAMERA_MAX_MMAP_BUFFER_NUMBER;
  req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory              = V4L2_MEMORY_MMAP;

  if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_REQBUFS, &req) == -1) 
  {
    if (errno == EINVAL)
    {
      return MAR_ERROR_MMAP_NOT_SUPPORTED;
    }
    else
    {
      return MAR_ERROR_BUFFER_REQUEST;
    }
  }

  if (req.count < 2) 
  {
    return MAR_ERROR_INSUFFICIENT_DEVICE_MEMORY;
  }

  /* Map every element of the array buffers to the shared memory */
  for (camera->num_mmap_buffers = 0; camera->num_mmap_buffers < req.count; ++camera->num_mmap_buffers) 
  {
    struct v4l2_buffer buf;

    MAR_CLEAR (buf);
    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = camera->num_mmap_buffers;

    if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_QUERYBUF, &buf) == -1)
    {
      return MAR_ERROR_QUERY_BUF;
    }

    camera->mmap_buffer_length = buf.length;
    camera->mmap_buffers[camera->num_mmap_buffers] = mmap (NULL,
              camera->mmap_buffer_length, PROT_READ | PROT_WRITE, MAP_SHARED, camera->dev_fd, buf.m.offset);

    if (camera->mmap_buffers[camera->num_mmap_buffers] == MAP_FAILED)
    {
      return MAR_ERROR_MMAP;
    }
  }

  return MAR_ERROR_NONE;
}

/**
 * Frees camera memory mapped resources
 *
 * @param camera The camera to free memory mapped resources from
 *
 * @return An MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PRIVATE
mar_error_code mar_v4l2_mmap_camera_mmap_free(mar_v4l2_mmap_camera *camera)
{
  int i;

  for (i = 0; i < camera->num_mmap_buffers; ++i)
  {
    if (munmap((void *)(camera->mmap_buffers[i]), camera->mmap_buffer_length) == -1)
    {
      return MAR_ERROR_MUNMAP;
    }
  }

  return MAR_ERROR_NONE;
}

/**
 * Initializes the camera portion of MAR.
 *
 * @param cam A pointer to a pointer which will be modified to point at a new camera
 * @param dev_name The camera device
 * @param format The camera pixel format
 * @param width The camera resolution width
 * @param height The camera resolution height
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PUBLIC
mar_error_code mar_v4l2_mmap_camera_new(mar_v4l2_mmap_camera **cam, char *dev_name, mar_camera_format format, int width, int height)
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;
  struct stat st;
  unsigned int min;
  mar_v4l2_mmap_camera *camera;
  mar_error_code retval;

  *cam = camera = malloc(sizeof(mar_v4l2_mmap_camera));
  
  if (camera == NULL)
  {
    return MAR_ERROR_MALLOC;
  }

  // Check if device name exists
  if (stat(dev_name, &st) == -1)
  {
    free(camera);
    return MAR_ERROR_DEVICE_NOT_FOUND;
  }

  // Check if it is a device
  if (!S_ISCHR(st.st_mode)) 
  {
    free(camera);
    return MAR_ERROR_NOT_CHARACTER_DEVICE;
  }

  // Open the device
  camera->dev_fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

  // Check for errors opening the file
  if (camera->dev_fd == -1) 
  {
    free(camera);
    return MAR_ERROR_DEVICE_OPEN;
  }

  // Check for capabilities
  if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_QUERYCAP, &cap) == -1) 
  {
    close(camera->dev_fd);
    free(camera);
    return MAR_ERROR_NOT_V4L2_DEVICE;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) 
  {
    close(camera->dev_fd);
    free(camera);
    return MAR_ERROR_NOT_VIDEO_CAPTURE_DEVICE;
  }

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) 
  {
    close(camera->dev_fd);
    free(camera);
    return MAR_ERROR_NOT_STREAMING_DEVICE;
  }

  /* Select video input, video standard and tune. */
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  mar_block_ioctl(camera->dev_fd, (int)VIDIOC_CROPCAP, &cropcap);
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  crop.c = cropcap.defrect;
  mar_block_ioctl(camera->dev_fd, (int)VIDIOC_S_CROP, &crop);

  /* Set camera resolution and format */
  MAR_CLEAR(fmt);
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = width;
  fmt.fmt.pix.height      = height;
  camera->width           = width;
  camera->height          = height;
  camera->format = format;

  switch (format) 
  {    
    case MAR_CAM_FMT_YUYV:
      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
      break;
    default:
      close(camera->dev_fd);
      free(camera);
      return MAR_ERROR_PIXEL_FORMAT_NOT_SUPPORTED;
  }

  if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_S_FMT, &fmt) == -1)
  {
      close(camera->dev_fd);
      free(camera);
      return MAR_ERROR_PIXEL_FORMAT_NOT_SUPPORTED;
  }

  /* Check the configuration data */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
      fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
      fmt.fmt.pix.sizeimage = min;
  
  retval = mar_v4l2_mmap_camera_mmap_init(camera);
  if (retval != MAR_ERROR_NONE)
  {
    close(camera->dev_fd);
    free(camera);
    return retval;
  }

  camera->frame_buffer_length = camera->width * camera->height * 3;
  camera->frame_buffer = malloc(camera->frame_buffer_length);
  if (camera->frame_buffer == NULL)
  {
    close(camera->dev_fd);
    free(camera);
    return MAR_ERROR_MALLOC;
  }

  return MAR_ERROR_NONE;
}

/**
 * Frees a camera created by the MAR library.
 *
 * @param camera The camera to free
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PUBLIC
mar_error_code mar_v4l2_mmap_camera_free(mar_v4l2_mmap_camera *camera)
{
  mar_error_code retval;
  
  retval = mar_v4l2_mmap_camera_mmap_free(camera);

  if (close(camera->dev_fd) == -1)
  {
    return MAR_ERROR_DEVICE_CLOSE;
  }

  free(camera->frame_buffer);
  free(camera);

  return retval;
}

/**
 * Starts camera capturing
 *
 * @param camera The camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PUBLIC
mar_error_code mar_v4l2_mmap_camera_start(mar_v4l2_mmap_camera *camera)
{
  int i;
  enum v4l2_buf_type type;

  for (i = 0; i < camera->num_mmap_buffers; ++i) 
  {
    struct v4l2_buffer buf;
    MAR_CLEAR(buf);
    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = i;

    if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_QBUF, &buf) == -1)
    {
      return MAR_ERROR_NO_BUFFER_QUEUED;
    }
  }
  
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_STREAMON, &type) == -1)
  {
    return MAR_ERROR_NO_STREAM_ON;
  }

  return MAR_ERROR_NONE;
}

/**
 * Converts a YUV color value to RGB.
 *
 * @param y The Y color component
 * @param u The U color component
 * @param v The V color component
 * @param r The red color component
 * @param g The green color component
 * @param b The blue color component
 */
MAR_PRIVATE
void mar_v4l2_mmap_camera_yuv_to_rgb(unsigned char y, unsigned char u, unsigned char v,
      unsigned char* r, unsigned char* g, unsigned char* b)
{
  int amp = 255;
  double R, G, B;
  
  // Convert YUV to RGB
  R = amp * (0.004565 * y + 0.000001 * u + 0.006250 * v - 0.872);
  G = amp * (0.004565 * y - 0.001542 * u - 0.003183 * v + 0.531);
  B = amp * (0.004565 * y + 0.007935 * u - 1.088);

  // Clamp RGB between 0 and 255  
  if (R < 0) R = 0;
  if (G < 0) G = 0;
  if (B < 0) B = 0;
  
  if (R > 255) R = 255;
  if (G > 255) G = 255;
  if (B > 255) B = 255;

  *r = (unsigned char)(R);
  *g = (unsigned char)(G);
  *b = (unsigned char)(B);
}

/** 
 * Updates the camera frame buffer from the camera's value buffer.
 *
 * @param camera The camera
 * @param mmap_buffer_index The index of the current mmap buffer
 */
MAR_PRIVATE
void mar_v4l2_mmap_camera_yuyv_fill_frame(mar_v4l2_mmap_camera *camera, int mmap_buffer_index)
{
  int i;
  uint8_t y1, y2, u, v, r, g, b;
  int frame_index = 0;

  for (i = 0; i < camera->mmap_buffer_length; i += 4)
  {
    y1 = camera->mmap_buffers[mmap_buffer_index][i + 0];
    u  = camera->mmap_buffers[mmap_buffer_index][i + 1];
    y2 = camera->mmap_buffers[mmap_buffer_index][i + 2];
    v  = camera->mmap_buffers[mmap_buffer_index][i + 3];

    mar_v4l2_mmap_camera_yuv_to_rgb(y1, u, v, &r, &g, &b);
    camera->frame_buffer[frame_index + 0] = r;
    camera->frame_buffer[frame_index + 1] = g;
    camera->frame_buffer[frame_index + 2] = b;

    mar_v4l2_mmap_camera_yuv_to_rgb(y2, u, v, &r, &g, &b);
    camera->frame_buffer[frame_index + 3] = r;
    camera->frame_buffer[frame_index + 4] = g;
    camera->frame_buffer[frame_index + 5] = b;

    frame_index += 6;
  }
}

/**
 * Updates the camera and captuers a new frame.
 * 
 * @param camera The camera to update
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PUBLIC
mar_error_code mar_v4l2_mmap_camera_update(mar_v4l2_mmap_camera *camera)
{
  fd_set fds;
  struct timeval tv;
  int r;
  struct v4l2_buffer buf;

  /* Select files */
  FD_ZERO (&fds);
  FD_SET (camera->dev_fd, &fds);

  /* Select Timeout */
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  /* Wait for the a frame */
  r = select(camera->dev_fd + 1, &fds, NULL, NULL, &tv);
  if (r == -1) 
  {
    if (EINTR == errno)
    {
      return MAR_ERROR_INTERRUPTED;
    }

    return MAR_ERROR_DEVICE_SELECT;
  }
  else if (r == 0) 
  {
    return MAR_ERROR_CAMERA_TIMEOUT;
  }

  /* Read a frame */
  MAR_CLEAR(buf);
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_DQBUF, &buf))
  {
    switch (errno) 
    {
      case EAGAIN:
        return MAR_ERROR_AGAIN;
      case EIO:
        return MAR_ERROR_AGAIN;
      default:
        return MAR_ERROR_NO_BUFFER_DEQUEUED;
    }
  }
      
  assert (buf.index < camera->num_mmap_buffers);

  /* Calculate bytes per frame */
  switch (camera->format) 
  {  
    case MAR_CAM_FMT_YUYV:
      mar_v4l2_mmap_camera_yuyv_fill_frame(camera, buf.index);
      break;
  }

  /* Get buffer ready */
  if (mar_block_ioctl(camera->dev_fd, (int)VIDIOC_QBUF, &buf) == -1)
  {
    return MAR_ERROR_NO_BUFFER_QUEUED;
  }

  return MAR_ERROR_NONE;
}

/**
 * Stops camera capturing
 *
 * @param camera The camera
 *
 * @return MAR_ERROR_NONE on success, an error code on failure
 */
MAR_PUBLIC
mar_error_code mar_v4l2_mmap_camera_stop(mar_v4l2_mmap_camera *camera)
{
  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (mar_block_ioctl(camera->dev_fd, VIDIOC_STREAMOFF, &type) == -1)
  {
    return MAR_ERROR_STREAM_NOT_OFF;
  }

  return MAR_ERROR_NONE;
}

/**
 * Returns the currently set camera pixel format.
 *
 * @param camera The camera to get the pixel format from
 *
 * @return The camera pixel format.
 */
MAR_PUBLIC
mar_camera_format mar_v4l2_mmap_camera_get_pixel_format(mar_v4l2_mmap_camera *camera)
{
  return camera->format;
}

/**
 * Returns the currently set camera resolution.
 *
 * @param camera The camera to get the resolution from
 * @param width Will be filled with the current resolution width.
 * @param height Will be filled with the current resolution height.
 */
MAR_PUBLIC
void mar_v4l2_mmap_camera_get_resolution(mar_v4l2_mmap_camera *camera, int *width, int *height)
{
  *width = camera->width;
  *height = camera->height;
}

/**
 * Returns the camera's frame buffer in an RGB24 format.  
 * The frame buffer is 3 * width * height in size.
 *
 * @param camera The camera to get the frame buffer of.
 * 
 * @return The camera buffer.
 */
unsigned char *mar_v4l2_mmap_camera_get_frame_buffer(mar_v4l2_mmap_camera *camera)
{
  return (unsigned char *)camera->frame_buffer;
}

