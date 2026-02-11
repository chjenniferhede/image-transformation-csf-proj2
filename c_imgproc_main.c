// C main function for image processing program

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "imgproc.h"

struct Transformation {
  const char *name;
  int (*apply)( struct Image *input_img, struct Image *output_img, int argc, char **argv );
  int (*out_dimensions)( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h );
};

int apply_squash( struct Image *input_img, struct Image *output_img, int argc, char **argv );
int apply_rot( struct Image *input_img, struct Image *output_img, int argc, char **argv );
int apply_blur( struct Image *input_img, struct Image *output_img, int argc, char **argv );
int apply_expand( struct Image *input_img, struct Image *output_img, int argc, char **argv );

int out_dimensions_squash( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h );
int out_dimensions_expand( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h );
int out_dimensions_same( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h );

static const struct Transformation s_transformations[] = {
  { "squash", apply_squash, out_dimensions_squash },
  { "color_rot", apply_rot, out_dimensions_same },
  { "blur", apply_blur, out_dimensions_same },
  { "expand", apply_expand, out_dimensions_expand },
  { NULL, NULL },
};

void usage( const char *progname ) {
  fprintf( stderr, "Error: invalid command-line arguments\n" );
  fprintf( stderr, "Usage: %s <transform> <input img> <output img> [args...]\n", progname );
  exit( 1 );
}

// For the squash transformation, get the yfac and xfac
// values from the command line arguments. Returns 1 if successful
// (i.e., they are present and valid), 0 otherwise.
int squash_get_factors( int argc, char **argv, int32_t *xfac, int32_t *yfac ) {
  if ( argc != 6
       || sscanf( argv[4], "%d", xfac ) != 1
       || sscanf( argv[5], "%d", yfac ) != 1 )
    return 0;

  if ( *xfac < 1 || *yfac < 1 )
    return 0;

  return 1;
}

// Make a new empty output Image.
// Calls the out_dimensions function of the Transformation
// to determine the dimensions of the output Image.
// Returns a poiinter to the empty output Image if successful,
// NULL otherwise.
struct Image *create_output_img( struct Image *input_img, int argc, char **argv,
                                 const struct Transformation *xform ) {
  struct Image *out_img;
  int32_t out_w, out_h;

  // Determine dimensions of output image based on the
  // transformation being done
  if ( !xform->out_dimensions( input_img, argc, argv, &out_w, &out_h ) )
    return NULL;

  // Allocate Image object
  out_img = (struct Image *) malloc( sizeof( struct Image ) );
  if ( out_img == NULL )
    return NULL;

  // Set data to NULL for now
  out_img->data = NULL;

  // Attempt to initialize the Image object by calling img_init
  if ( img_init( out_img, out_w, out_h ) != IMG_SUCCESS ) {
    free( out_img );
    return NULL;
  }

  // Success!
  return out_img;
}

// Free memory allocated to given Image object
void cleanup_image( struct Image *img ) {
  if ( img != NULL ) {
    img_cleanup( img );
    free( img );
  }
}

int main( int argc, char **argv ) {
  if ( argc < 4 )
    usage( argv[0] );

  const char *transformation = argv[1];
  const char *input_filename = argv[2];
  const char *output_filename = argv[3];

  // find transformation
  const struct Transformation *xform = NULL;
  for ( int i = 0; s_transformations[i].name != NULL; ++i ) {
    if ( strcmp( s_transformations[i].name, transformation ) == 0 ) {
      xform = &s_transformations[i];
      break;
    }
  }

  if ( xform == NULL ) {
    fprintf( stderr, "Error: unknown transformation '%s'\n", transformation );
    return 1;
  }

  // Allocate and read the input image
  struct Image *input_img = (struct Image *) malloc( sizeof( struct Image ) );
  if ( input_img == NULL ) {
    fprintf( stderr, "Error: couldn't allocate input image\n" );
    return 1;
  }
  if ( img_read( input_filename, input_img ) != IMG_SUCCESS ) {
    fprintf( stderr, "Error: couldn't read input image\n" );
    free( input_img );
    return 1;
  }

  // Create output Image object
  struct Image *output_img = create_output_img( input_img, argc, argv, xform );
  if ( output_img == NULL ) {
    fprintf( stderr, "Error: couldn't create output image object\n" );
    cleanup_image( input_img );
    return 1;
  }

  int success;

  // apply the transformation!
  success = xform->apply( input_img, output_img, argc, argv ) != 0;

  if ( success ) {
    // Write output image
    if ( img_write( output_filename, output_img ) != IMG_SUCCESS ) {
      fprintf( stderr, "Error: couldn't write output image\n" );
      success = 0;
    }
  }

  cleanup_image( input_img );
  cleanup_image( output_img );

  return success ? 0 : 1;
}

int apply_squash( struct Image *input_img, struct Image *output_img, int argc, char **argv ) {
  int32_t xfac, yfac;

  // In theory, out_dimensions_squash() has already verified
  // that the xfac/yfac command line arguments are present and
  // valid, but no harm in being paranoid.
  int rc;
  rc = squash_get_factors( argc, argv, &xfac, &yfac );
  assert( rc != 0 );
  (void) rc;

  imgproc_squash( input_img, output_img, xfac, yfac );

  return 1;
}

int apply_rot( struct Image *input_img, struct Image *output_img, int argc, char **argv ) {
  (void) argc;
  (void) argv;
  imgproc_color_rot( input_img, output_img );
  return 1;
}

int apply_blur( struct Image *input_img, struct Image *output_img, int argc, char **argv ) {
  int blur_dist;
  if ( argc != 5 || sscanf( argv[4], "%d", &blur_dist ) != 1 )
    // invalid arguments
    return 0;
  imgproc_blur( input_img, output_img, blur_dist );
  return 1;
}

int apply_expand( struct Image *input_img, struct Image *output_img, int argc, char **argv ) {
  (void) argc;
  (void) argv;
  imgproc_expand( input_img, output_img );
  return 1;
}

int out_dimensions_squash( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h ) {
  // In the squash transformation, the x (width) and y (height) dimensions
  // are divided by an integer factor.
  int32_t xfac, yfac;
  if ( !squash_get_factors( argc, argv, &xfac, &yfac ) )
    return 0;
  *out_w = input_img->width / xfac;
  *out_h = input_img->height / yfac;
  return 1;
}

int out_dimensions_expand( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h ) {
  // In the expand transformation, the width and height
  // are both doubled.
  *out_w = input_img->width * 2;
  *out_h = input_img->height * 2;
  return 1;
}

int out_dimensions_same( struct Image *input_img, int argc, char **argv, int32_t *out_w, int32_t *out_h ) {
  // This function is used for transformations where the output image size
  // is the same as the input image size.
  *out_w = input_img->width;
  *out_h = input_img->height;
  return 1;
}
