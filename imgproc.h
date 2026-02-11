// Header for image processing API functions (imgproc_complement, etc.)
// as well as any helper functions they rely on.

#ifndef IMGPROC_H
#define IMGPROC_H

#include "image.h" // for struct Image and related functions

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
void imgproc_squash( struct Image *input_img, struct Image *output_img, int32_t xfac, int32_t yfac );

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
void imgproc_color_rot( struct Image *input_img, struct Image *output_img );

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
void imgproc_blur( struct Image *input_img, struct Image *output_img, int32_t blur_dist );

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
//! If i is even but j is odd, then the color components and alpha value
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
void imgproc_expand( struct Image *input_img, struct Image *output_img);

// TODO: add prototypes for your helper functions

#endif // IMGPROC_H
