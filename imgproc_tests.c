#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tctest.h"
#include "imgproc.h"

// Maximum number of pixels in a test image
#define MAX_NUM_PIXELS 1500

// Test image data: used to represent small test
// images that can be directly embedded in the test
// source code
struct TestImageData {
  int32_t width, height;
  uint32_t pixels[MAX_NUM_PIXELS];
};

// Include test image data
#include "test_image_data.h"

// Data type for the test fixture object.
// This contains data (including Image objects) that
// can be accessed by test functions. This is useful
// because multiple test functions can access the same
// data (so you don't need to create/initialize that
// data multiple times in different test functions.)
typedef struct {
  // Test images (for basic image transform tests)
  struct Image small, smol, smol_squash_1_1, smol_squash_3_1, smol_squash_1_3,
               smol_color_rot, smol_blur_0, smol_blur_3, smol_expand;

} TestObjs;

// Functions to create and clean up a test fixture object
TestObjs *setup( void );
void cleanup( TestObjs *objs );

// Helper functions used by the test code
void init_image_from_testdata(struct Image *img, struct TestImageData *test_data);
struct Image *create_output_image( const struct Image *src_img );
bool images_equal( struct Image *a, struct Image *b );
void destroy_img( struct Image *img );

// Test functions
void test_squash_basic( TestObjs *objs );
void test_color_rot_basic( TestObjs *objs );
void test_blur_basic( TestObjs *objs );
void test_expand_basic( TestObjs *objs );

// My tests
void test_pixel_getters( TestObjs *objs );
void test_pixel_maker( TestObjs *objs );
void test_compute_index( TestObjs *objs );
void test_blur_pixel( TestObjs *objs );
void test_color_rot_pixel( TestObjs *objs );

int main( int argc, char **argv ) {
  // allow the specific test to execute to be specified as the
  // first command line argument
  if ( argc > 1 )
    tctest_testname_to_execute = argv[1];

  TEST_INIT();

  // Run tests.
  // Make sure you add additional TEST() macro invocations
  // for any additional test functions you add.
  TEST( test_squash_basic );
  TEST( test_color_rot_basic );
  TEST( test_blur_basic );
  TEST( test_expand_basic );

  // My tests
  TEST( test_pixel_getters ); 
  TEST( test_pixel_maker );
  TEST( test_compute_index );
  TEST( test_blur_pixel );
  TEST( test_color_rot_pixel );

  TEST_FINI();
}

////////////////////////////////////////////////////////////////////////
// Test fixture setup/cleanup functions
////////////////////////////////////////////////////////////////////////

TestObjs *setup( void ) {
  TestObjs *objs = (TestObjs *) malloc( sizeof(TestObjs) );

  // Initialize test Images from test image data
  init_image_from_testdata( &objs->smol, &smol );
  init_image_from_testdata( &objs->smol_squash_1_1, &smol_squash_1_1 );
  init_image_from_testdata( &objs->smol_squash_3_1, &smol_squash_3_1 );
  init_image_from_testdata( &objs->smol_squash_1_3, &smol_squash_1_3 );
  init_image_from_testdata( &objs->smol_color_rot, &smol_color_rot );
  init_image_from_testdata( &objs->smol_blur_0, &smol_blur_0 );
  init_image_from_testdata( &objs->smol_blur_3, &smol_blur_3 );
  init_image_from_testdata( &objs->smol_expand, &smol_expand );
  init_image_from_testdata( &objs->small, &small );

  return objs;
}

void cleanup( TestObjs *objs ) {
  // Note that the test Images don't need to be cleaned
  // up, because their data isn't dynamically allocated
  free( objs );
}

////////////////////////////////////////////////////////////////////////
// Test code helper functions
////////////////////////////////////////////////////////////////////////

// Helper function to initialize an Image from
// a TestImageData instance. Note that the Image will
// point directly to the TestImageData's pixel data,
// so don't call img_cleanup() on the resulting Image.
void init_image_from_testdata(struct Image *img, struct TestImageData *test_data) {
  img->width = test_data->width;
  img->height = test_data->height;
  img->data = test_data->pixels;
}

// Helper function to create a temporary output Image
// the same size as a given one
struct Image *create_output_image( const struct Image *src_img ) {
  struct Image *img;
  img = malloc( sizeof( struct Image ) );
  img_init( img, src_img->width, src_img->height );
  return img;
}

// Returns true IFF both Image objects are identical
bool images_equal( struct Image *a, struct Image *b ) {
  if ( a->width != b->width || a->height != b->height )
    return false;

  for ( int i = 0; i < a->height; ++i )
    for ( int j = 0; j < a->width; ++j ) {
      int index = i*a->width + j;
      if ( a->data[index] != b->data[index] )
        return false;
    }

  return true;
}

void destroy_img( struct Image *img ) {
  if ( img != NULL )
    img_cleanup( img );
  free( img );
}

////////////////////////////////////////////////////////////////////////
// Test functions
////////////////////////////////////////////////////////////////////////

#define SQUASH_TEST( xfac, yfac ) \
do { \
  struct Image *out_img = create_output_image( &objs->smol_squash_##xfac##_##yfac ); \
  imgproc_squash( &objs->smol, out_img, xfac, yfac ); \
  ASSERT( images_equal( out_img, &objs->smol_squash_##xfac##_##yfac ) ); \
  destroy_img( out_img ); \
} while (0)

#define XFORM_TEST( xform ) \
do { \
  struct Image *out_img = create_output_image( &objs->smol_##xform ); \
  imgproc_##xform( &objs->smol, out_img ); \
  ASSERT( images_equal( out_img, &objs->smol_##xform ) ); \
  destroy_img( out_img ); \
} while (0)

#define BLUR_TEST( blur_dist) \
do { \
  struct Image *out_img = create_output_image( &objs->smol_blur_##blur_dist ); \
  imgproc_blur( &objs->smol, out_img, blur_dist ); \
  ASSERT( images_equal( out_img, &objs->smol_blur_##blur_dist ) ); \
  destroy_img( out_img ); \
} while (0)

// objs->smol is the og test image, does not matter here, just to have sth indexed
#define INDEX_TEST( row, col, expected ) \
do { \
  int32_t index = compute_index( &objs->small, row, col ); \
  ASSERT( index == expected ); \
} while (0)

#define SINGLE_PIXEL_BLUR_TEST( row, col, blur_dist, expected ) \
do { \
  uint32_t pixel = blur_pixel( &objs->small, row, col, blur_dist ); \
  ASSERT( pixel == expected ); \
} while (0)

#define SINGLE_PIXEL_ROT_TEST( row, col, expected ) \
do { \
  uint32_t pixel = rot_pixel( &objs->smol, compute_index( &objs->smol, row, col ) ); \
  ASSERT( pixel == expected ); \
} while (0)

/* Functions that run the tests using multiple test images. */
void test_squash_basic( TestObjs *objs ) {
  SQUASH_TEST( 1, 1 );
  SQUASH_TEST( 3, 1 );
  SQUASH_TEST( 1, 3 );
}

void test_color_rot_basic( TestObjs *objs ) {
  XFORM_TEST( color_rot );
}

void test_blur_basic( TestObjs *objs ) {
  BLUR_TEST( 0 );
  BLUR_TEST( 3 );
}

void test_expand_basic( TestObjs *objs ) {
  XFORM_TEST( expand );
}

// Test pixel getter 
void test_pixel_getters( TestObjs *objs ) { 
  uint32_t pixel = make_pixel(0xAA, 0xBB, 0xCC, 0xDD);
  ASSERT( get_r(pixel) == 0x000000AA );
  ASSERT( get_g(pixel) == 0x000000BB );
  ASSERT( get_b(pixel) == 0x000000CC );
  ASSERT( get_a(pixel) == 0x000000DD );
}

// Test pixel maker
void test_pixel_maker( TestObjs *objs ) {
  uint32_t pixel = make_pixel(0xAA, 0xBB, 0xCC, 0xDD);
  ASSERT( pixel == 0xAABBCCDD );
  uint32_t pixel2 = make_pixel(0xFF, 0x00, 0x00, 0xFF);
  ASSERT( pixel2 == 0xFF0000FF );
} 

// Test compute index
void test_compute_index( TestObjs *objs ) {
  INDEX_TEST( 0, 0, 0 );
  INDEX_TEST( 0, 1, 1 );
  INDEX_TEST( 0, 2, 2 );
  INDEX_TEST( 1, 0, 3 );
  INDEX_TEST( 1, 1, 4 );
  INDEX_TEST( 2, 0, 6 );
}

// Test blur_pixel on a single pixel 
void test_blur_pixel( TestObjs *objs ) {
  // blur with zero, same pixel 
  SINGLE_PIXEL_BLUR_TEST( 0, 0, 0, 0x000000FF );
  SINGLE_PIXEL_BLUR_TEST( 1, 1, 0, 0xFFFFFFFF );

  SINGLE_PIXEL_BLUR_TEST( 0, 0, 1, 0x3F3F3FFF ); // avg of 4 pixels (0,0), (0,1), (1,0), (1,1), 0xFF/4 = 0x3F
  SINGLE_PIXEL_BLUR_TEST( 1, 1, 1, 0x1C1C1CFF ); // FF/9 is 1C

  // test clamping
  SINGLE_PIXEL_BLUR_TEST( 1, 1, 3, 0x1C1C1CFF );
  SINGLE_PIXEL_BLUR_TEST( 0, 0, 4, 0x1C1C1CFF ); 
}

// Test rot_pixel on a single pixel
void test_color_rot_pixel( TestObjs *objs ) {
  SINGLE_PIXEL_ROT_TEST( 0, 0, 0x90ac9dff ); // 0xac9d90ff to 0x90ac9dff
  SINGLE_PIXEL_ROT_TEST( 0, 1, 0x90a89bff ); // 0xa89b90ff to 0x90a89bff
}
