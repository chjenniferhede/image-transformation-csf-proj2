// C implementations of image processing functions

#include <stdlib.h>
#include <assert.h>
#include "imgproc.h"

// Helper functions

// pixel >> 24 leaves r in the lowest 8 bits
// pixel >> 16 leaves g in the lowest 8 bits
// pixel >> 8 leaves b in the lowest 8 bits
// pixel & 0xFF leaves a in the lowest 8 bits
uint32_t get_r( uint32_t pixel ) {
  return (pixel >> 24) & 0xFF;
}
uint32_t get_g( uint32_t pixel ) {
  return (pixel >> 16) & 0xFF;
}
uint32_t get_b( uint32_t pixel ) {
  return (pixel >> 8) & 0xFF;
}
uint32_t get_a( uint32_t pixel ) {
  return pixel & 0xFF;
}

// Given rgba values, make a pixel by putting r in the highest 
// 8 bits, g in the next highest 8 bits, b in the next highest 8 bits
uint32_t make_pixel( uint32_t r, uint32_t g, uint32_t b, uint32_t a ) {
  return (r << 24) | (g << 16) | (b << 8) | a;
}

// compute the index into the data array for the pixel at
// row and col in img
int32_t compute_index( struct Image *img, int32_t row, int32_t col ) {
  return row * img->width + col;
}

// blur a pixel at (row, col) in img with the given blur_dist
uint32_t blur_pixel( struct Image *img, int32_t row, int32_t col, int32_t blur_dist ) {
  // Get the pixels within the bound
  int32_t r_start = row - blur_dist;
  int32_t r_end = row + blur_dist;
  int32_t c_start = col - blur_dist;
  int32_t c_end = col + blur_dist;

  // Clamp image bounds
  if (r_start < 0) { r_start = 0; }
  if (r_end >= img->height) { r_end = img->height - 1; }
  if (c_start < 0) { c_start = 0;}
  if (c_end >= img->width) { c_end = img->width - 1; }

  // Setup RGB variables
  uint32_t r_total = 0;
  uint32_t g_total = 0;
  uint32_t b_total = 0;
  uint32_t count = 0;

  // Get the pixel and add the RGB values to the totals, keep counts
  for (int32_t r = r_start; r <= r_end; r++) {
    for (int32_t c = c_start; c <= c_end; c++) {
      uint32_t pixel = img->data[compute_index(img, r, c)];
      r_total += get_r(pixel);
      g_total += get_g(pixel);
      b_total += get_b(pixel);
      count++;
    }
  }

  // Calculate averages
  uint32_t r_avg = r_total / count;
  uint32_t g_avg = g_total / count;
  uint32_t b_avg = b_total / count;

  // Get the alpha value of the original pixel
  uint32_t a = get_a(img->data[compute_index(img, row, col)]);
  // Make the new pixel with the average RGB values and original alpha value
  return make_pixel(r_avg, g_avg, b_avg, a);
}

uint32_t rot_colors( struct Image *img, int32_t index ) { 
  // Get the pixel at the index
  uint32_t pixel = img->data[index];
  // Get the r, g, b, a values of the pixel
  uint32_t r = get_r(pixel);
  uint32_t g = get_g(pixel);
  uint32_t b = get_b(pixel);
  uint32_t a = get_a(pixel);
  // return the new pixel with rotated colors
  return make_pixel(b, r, g, a);
}

// Average the the pixels 
uint32_t avg_pixels( uint32_t *pixels, int num_pixels) {
  uint32_t r_total = 0;
  uint32_t g_total = 0;
  uint32_t b_total = 0;
  uint32_t a_total = 0;
    // Accumulate r, g, b, a values
  for (int i = 0; i < num_pixels; i++) {
    r_total += get_r(pixels[i]);
    g_total += get_g(pixels[i]);
    b_total += get_b(pixels[i]);
    a_total += get_a(pixels[i]);
  }
  uint32_t r_avg = r_total / num_pixels;
  uint32_t g_avg = g_total / num_pixels;
  uint32_t b_avg = b_total / num_pixels;
  uint32_t a_avg = a_total / num_pixels;
  return make_pixel(r_avg, g_avg, b_avg, a_avg);
}

//! Transform the entire image by shrinking it down both 
//! horizontally and vertically (by potentially different
//! factors). This is equivalent to sampling the orignal image
//! for every pixel that is in certain rows and columns as 
//! specified in the function inputs.
//!
//! Take the image below where each letter corresponds to a pixel
//!
//!                 XAAAYBBB
//!                 AAAABBBB
//!                 ZCCCWDDD
//!                 CCCCDDDD
//!
//! If the user specified to shrink it horazontally by a factor 
//! of 4 and shrink it vertically by a factor of 2, you would 
//! sample pixel that had a row index such that 
//!
//!             row index % 2 = 0 
//!
//! and a column index such that
//!
//!             column index % 4 = 0
//!
//! in the above example, this would mean pixels that are in 
//! rows 0 and 2 with columns 0 and 4. 
//! The resultant image is:
//!
//!                 XY
//!                 ZW
//! 
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
//! @param xfac factor to downsize the image horizontally; guaranteed to be positive
//! @param yfac factor to downsize the image vertically; guaranteed to be positive
void imgproc_squash( struct Image *input_img, struct Image *output_img, int32_t xfac, int32_t yfac ) {
  // Loop through the output image and sample from the input image
  for (int32_t row = 0; row < output_img->height; row++) {
    for (int32_t col = 0; col < output_img->width; col++) {
      // To be divisible, they are just multiples of the factors. 
      int32_t input_row = row * yfac; 
      int32_t input_col = col * xfac;

      // Check if the input row and column are in bounds
      if (input_row < input_img->height && input_col < input_img->width) {
        // rol and col are output image size, input row and col are computed, skipped indices of input)
        output_img->data[compute_index(output_img, row, col)] = input_img->data[compute_index(input_img, input_row, input_col)];
      }
    }
  }

}

//! Transform the color component values in each input pixel
//! by applying a rotation on the values of the color components
//! I.e. the old pixel's red component value will be used for
//! the new pixel's green component value, the old pixel's green
//! component value will be used new pixel's blue component value
//! and the old pixel's blue component value will be used new 
//! pixel's red component value. The alpha value should not change.
//! For instance, if a pixel had the hex value 0xAABBCCDD, the 
//! transformed pixel would become 0xCCAABBDD
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
void imgproc_color_rot( struct Image *input_img, struct Image *output_img) {
  // With helper function for each pixel, just loop through and output.
  for (int32_t row = 0; row < input_img->height; row++) {
    for (int32_t col = 0; col < input_img->width; col++) {
      int32_t index = compute_index(input_img, row, col);
      output_img->data[index] = rot_colors(input_img, index);
    }
  }
}

//! Transform the input image using a blur effect.
//!
//! Each pixel of the output image should have its color components
//! determined by taking the average of the color components of pixels
//! within blur_dist number of pixels horizontally and vertically from
//! the pixel's location in the original image. For example, if
//! blur_dist is 0, then only the original pixel is considered, and the
//! the output image should be identical to the input image. If blur_dist
//! is 1, then the original pixel and the 8 pixels immediately surrounding
//! it would be considered, etc.  Pixels positions not within the bounds of
//! the image should be ignored: i.e., their color components aren't
//! considered in the computation of the result pixels.
//!
//! The alpha value each output pixel should be identical to the
//! corresponding input pixel.
//!
//! Averages should be computed using purely integer arithmetic with
//! no rounding.
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
//! @param blur_dist all pixels whose x/y coordinates are within
//!                  this many pixels of the x/y coordinates of the
//!                  original pixel should be included in the color
//!                  component averages used to determine the color
//!                  components of the output pixel
void imgproc_blur( struct Image *input_img, struct Image *output_img, int32_t blur_dist ) {
  // With helper function for each pixel, just loop through and output.
  for (int32_t row = 0; row < input_img->height; row++) {
    for (int32_t col = 0; col < input_img->width; col++) {
      int32_t index = compute_index(input_img, row, col);
      output_img->data[index] = blur_pixel(input_img, row, col, blur_dist);
    }
  }
}

//! The `expand` transformation doubles the width and height of the image.
//! 
//! Let's say that there are n rows and m columns of pixels in the
//! input image, so there are 2n rows and 2m columns in the output
//! image.  The pixel color and alpha value of the output pixel at row i and column
//! j should be computed as follows.
//! 
//! If both i and j are even, then the color and alpha value of the output
//! pixel are exactly the same as the input pixel at row i/2 and column j/2.
//! 
//! If i (row) is even but j is odd, then the color components and alpha value
//! of the output pixel are computed as the average of those in the input pixels
//! in row i/2 at columns floor(j/2) and floor(j/2) + 1.
//! 
//! If i is odd and j is even, then the color components and alpha value
//! of the output pixel are computed as the average of those in the input pixels
//! in column j/2 at rows floor(i/2) and  floor(i/2) + 1.
//! 
//! If both i and j are odd then the color components and alpha value
//! of the output pixel are computed as the average of the input pixels
//! 
//! 1. At row floor(i/2) and column floor(j/2)
//! 2. At row floor(i/2) and column floor(j/2) + 1
//! 3. At row floor(i/2) + 1 and column floor(j/2)
//! 4. At row floor(i/2) + 1 and column floor(j/2) + 1
//! 
//! Note that in the cases where either i or j is odd, it is not
//! necessarily the case that either row floor(i/2) + 1 or
//! column floor(j/2) + 1 are in bounds in the input image.
//! Only input pixels that are properly in bounds should be incorporated into
//! the averages used to determine the color components and alpha value
//! of the output pixel.
//! 
//! Averages should be computed using purely integer arithmetic with
//! no rounding.
//!
//! @param input_img pointer to the input Image
//! @param output_img pointer to the output Image (in which the
//!                   transformed pixels should be stored)
void imgproc_expand( struct Image *input_img, struct Image *output_img) {
  for (int32_t row = 0; row < output_img->height; row++) {
    for (int32_t col = 0; col < output_img->width; col++) {

      // Case 1: both even (row and col are output size)
      if (row % 2 == 0 && col % 2 == 0) {
        int32_t indexOut = compute_index(output_img, row, col);
        int32_t indexInput = compute_index(input_img, row/2, col/2);
        output_img->data[indexOut] = input_img->data[indexInput];
      }
      // Case 2: i (row) even, j odd
      else if (row % 2 == 0 && col % 2 == 1) {
        int32_t indexOut = compute_index(output_img, row, col);
        uint32_t pixels[2];
        size_t count = 0;
        pixels[count++] = input_img->data[compute_index(input_img, row/2, col/2)];
        if (col/2 + 1 < input_img->width) {
          pixels[count++] = input_img->data[compute_index(input_img, row/2, col/2 + 1)];
        } 
        output_img->data[indexOut] = avg_pixels(pixels, count);
      }
      // Case 3: i odd, j (col) even
      else if (row % 2 == 1 && col % 2 == 0) {
        int32_t indexOut = compute_index(output_img, row, col);
        uint32_t pixels[2];
        size_t count = 0;
        pixels[count++] = input_img->data[compute_index(input_img, row/2, col/2)];
        if (row/2 + 1 < input_img->height) {
          pixels[count++] = input_img->data[compute_index(input_img, row/2 + 1, col/2)];
        } 
        output_img->data[indexOut] = avg_pixels(pixels, count);    
      }
      // Case 4: both odd
      else {
        int32_t indexOut = compute_index(output_img, row, col);
        // average of four pixels
        uint32_t pixels[4];
        size_t count = 0; 
        pixels[count++] = input_img->data[compute_index(input_img, row/2, col/2)];
        if (col/2 + 1 < input_img->width) {
          pixels[count++] = input_img->data[compute_index(input_img, row/2, col/2 + 1)];
        } 
        if (row/2 + 1 < input_img->height) {
          pixels[count++] = input_img->data[compute_index(input_img, row/2 + 1, col/2)];
        } 
        if (col/2 + 1 < input_img->width && row/2 + 1 < input_img->height) {
          pixels[count++] = input_img->data[compute_index(input_img, row/2 + 1, col/2 + 1)];
        } 
        output_img->data[indexOut] = avg_pixels(pixels, count);
      }
    }
  }
}
