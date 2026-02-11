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
  struct Image smol, smol_squash_1_1, smol_squash_3_1, smol_squash_1_3,
               smol_color_rot, smol_blur_0, smol_blur_3, smol_expand;

  // TODO: add additional test fixture data as needed
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
// TODO: add prototypes for additional test functions

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

  return objs;
}

void cleanup( TestObjs *objs ) {
  // Note that the test Images don't need to be cleaned
  // up, because their data isn't dynamically allocated

  // TODO: clean up any test fixture data

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

// TODO: define additional test functions
