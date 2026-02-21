/*
 * x86-64 assembly language implementations of functions
 */

	.section .text

/* Offsets of struct Image fields */
#define IMAGE_WIDTH_OFFSET   0
#define IMAGE_HEIGHT_OFFSET  4
#define IMAGE_DATA_OFFSET    8

/** Helper functions **/
/* Get rgba components */

	.global get_r
get_r:
	/* Register use:
	 *   %edi: pixel value (input)
	 *   %eax: red component value (output)
	 */
	movl %edi, %eax		/* copy pixel to return register, edi is the lower 32 bit*/
	shrl $24, %eax      /* shift right 24 bits */
	andl $0xFF, %eax    /* mask to lowest 8 bits */
	ret

	.global get_g
get_g:
	/* Register use:
	 *   %edi: pixel value (input)
	 *   %eax: green component value (output)
	 */
	movl %edi, %eax		/* copy pixel to return register */
	shrl $16, %eax 		/* shift right 16 bits */
	andl $0xFF, %eax	/* mask to lowest 8 bits */
	ret

	.global get_b
get_b:
	/* Register use:
	 *   %edi: pixel value (input)
	 *   %eax: blue component value (output)
	 */
	movl %edi, %eax		/* copy pixel to return register */
	shrl $8, %eax 		/* shift right 8 bits */
	andl $0xFF, %eax	/* mask to lowest 8 bits */
	ret

	.global get_a
get_a:
	/* Register use:
	 *   %edi: pixel value (input)
	 *   %eax: alpha component value (output)
	 */
	movl %edi, %eax		/* copy pixel to return register */
	andl $0xFF, %eax	/* mask to lowest 8 bits */
	ret

	.global make_pixel
make_pixel:
	/* Register use:
	 *   %edi: red component value (input)
	 *   %esi: green component value (input)
	 *   %edx: blue component value (input)
	 *   %ecx: alpha component value (input)
	 *   %eax: pixel value (output)
	 */
	movl %edi, %eax     /* copy red component to return register */
	shll $8, %eax 		/* shift left 8 bits */
	orl %esi, %eax		/* bitwise or green component to return register */
	shll $8, %eax 		/* shift left 8 bits */
	orl %edx, %eax		/* bitwise or blue component to return register */
	shll $8, %eax 		/* shift left 8 bits */
	orl %ecx, %eax		/* bitwise or alpha component to return register */	
	ret

	.global compute_index
compute_index:

	/* Register use:
	 *   %rdi: image pointer (input)
	 *   %esi: row (input)
	 *   %edx: column (input)
	 *   %eax: pixel index (output)
	 */
	movl IMAGE_WIDTH_OFFSET(%rdi), %eax	/* access image width (4 bytes into rdi) into eax */
	imull %esi, %eax			/* multiply row by image width */
	addl %edx, %eax			/* add column to get pixel index */	
	ret



	.global rot_pixel
rot_pixel:
	/* Register use:
	 *   %rdi: pointer to Image struct (input)
	 *   %rsi: pixel index (input)
	 *
	 *   %rdx  - base pointer to pixel array (pointer to first pixel)
	 *   %r12d - target pixel value (temp)
	 *   %edi - hold the pixel value for helper function calls (temp)
	 *
	 *   %r13d  - original r component (temp)
	 *   %r14d  - original g component (temp)
	 *   %r15d  - original b component (temp)
	 *   %rbx  - original a component (temp)
	 *
	 *   %eax - rotted pixel (output)
	 */
	# save callee-saved registers we will use
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbx
	# Not using stack, and this 5 * 8 + 8 is aligned

	# load pointer-to-data from struct (memory read)
	movq IMAGE_DATA_OFFSET(%rdi), %rdx    /* rdx = img->data, still a pointer */

	# fetch pixel value (4 bytes) at index rsi
	movl (%rdx,%rsi,4), %r12d             /* r12d = pixel, access the value */

	# extract components 
	movl %r12d, %edi /* move pixel value to edi for helper call */
	call get_r
	movl %eax, %r13d  /* save r to r13d */

	movl %r12d, %edi
	call get_g
	movl %eax, %r14d  /* save g to r14d */

	movl %r12d, %edi
	call get_b
	movl %eax, %r15d  /* save b to r15d */

	movl %r12d, %edi
	call get_a
	movl %eax, %ebx  /* save a to ebx */

	# build rotted pixel
	movl %r15d, %edi /* r = old b */
	movl %r13d, %esi  /* g = old r */
	movl %r14d, %edx  /* b = old g */
	movl %ebx, %ecx /* a = still a */
	call make_pixel /* takes make_pixel(edi, esi, edx, ecx) */

	popq %rbx
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	ret

# TODO
	.global blur_pixel
blur_pixel:

	ret

	.global avg_pixels
avg_pixels:

	ret


/*
 * Definitions of image transformation functions
 */

/*
 *  Transform the entire image by shrinking it down both 
 *  horizontally and vertically (by potentially different
 *  factors). This is equivalent to sampling the orignal image
 *  for every pixel that is in certain rows and columns as 
 *  specified in the function inputs.
 * 
 *  Take the image below where each letter corresponds to a pixel
 * 
 *                  XAAAYBBB
 *                  AAAABBBB
 *                  ZCCCWDDD
 *                  CCCCDDDD
 * 
 *  If the user specified to shrink it horazontally by a factor 
 *  of 4 and shrink it vertically by a factor of 2, you would 
 *  sample pixel that had a row index such that 
 * 
 *              row index % 2 = 0 
 * 
 *  and a column index such that
 * 
 *              column index % 4 = 0
 * 
 *  in the above example, this would mean pixels that are in 
 *  rows 0 and 2 with columns 0 and 4. 
 *  The resultant image is:
 * 
 *                  XY
 *                  ZW
 *  
 *  @param input_img pointer to the input Image
 *  @param output_img pointer to the output Image (in which the
 *                    transformed pixels should be stored)
 *  @param xfac factor to downsize the image horizontally; guaranteed to be positive
 *  @param yfac factor to downsize the image vertically; guaranteed to be positive
 */
	.globl imgproc_squash
imgproc_squash:
	/* TODO: implement */
	ret

/*
 *  Transform the color component values in each input pixel
 *  by applying a rotation on the values of the color components
 *  I.e. the old pixel's red component value will be used for
 *  the new pixel's green component value, the old pixel's green
 *  component value will be used new pixel's blue component value
 *  and the old pixel's blue component value will be used new 
 *  pixel's red component value. The alpha value should not change.
 *  For instance, if a pixel had the hex value 0xAABBCCDD, the 
 *  transformed pixel would become 0xCCAABBDD
 * 
 *  @param input_img pointer to the input Image
 *  @param output_img pointer to the output Image (in which the
 *                    transformed pixels should be stored)
 */
	.globl imgproc_color_rot
imgproc_color_rot:
	/* TODO: implement */
	ret


/*
 *  Transform the input image using a blur effect.
 * 
 *  Each pixel of the output image should have its color components
 *  determined by taking the average of the color components of pixels
 *  within blur_dist number of pixels horizontally and vertically from
 *  the pixel's location in the original image. For example, if
 *  blur_dist is 0, then only the original pixel is considered, and the
 *  the output image should be identical to the input image. If blur_dist
 *  is 1, then the original pixel and the 8 pixels immediately surrounding
 *  it would be considered, etc.  Pixels positions not within the bounds of
 *  the image should be ignored: i.e., their color components aren't
 *  considered in the computation of the result pixels.
 * 
 *  The alpha value each output pixel should be identical to the
 *  corresponding input pixel.
 * 
 *  Averages should be computed using purely integer arithmetic with
 *  no rounding.
 * 
 *  @param input_img pointer to the input Image
 *  @param output_img pointer to the output Image (in which the
 *                    transformed pixels should be stored)
 *  @param blur_dist all pixels whose x/y coordinates are within
 *                   this many pixels of the x/y coordinates of the
 *                   original pixel should be included in the color
 *                   component averages used to determine the color
 *                   components of the output pixel
 */
	.globl imgproc_blur
imgproc_blur:
	/* TODO: implement */
	ret

/*
 *  The `expand` transformation doubles the width and height of the image.
 *  
 *  Let's say that there are n rows and m columns of pixels in the
 *  input image, so there are 2n rows and 2m columns in the output
 *  image.  The pixel color and alpha value of the output pixel at row i and column
 *  j should be computed as follows.
 *  
 *  If both i and j are even, then the color and alpha value of the output
 *  pixel are exactly the same as the input pixel at row i/2 and column j/2.
 *  
 *  If i is even but j is odd, then the color components and alpha value
 *  of the output pixel are computed as the average of those in the input pixels
 *  in row i/2 at columns floor(j/2) and floor(j/2) + 1.
 *  
 *  If i is odd and j is even, then the color components and alpha value
 *  of the output pixel are computed as the average of those in the input pixels
 *  in column j/2 at rows floor(i/2) and  floor(i/2) + 1.
 *  
 *  If both i and j are odd then the color components and alpha value
 *  of the output pixel are computed as the average of the input pixels
 *  
 *  1. At row floor(i/2) and column floor(j/2)
 *  2. At row floor(i/2) and column floor(j/2) + 1
 *  3. At row floor(i/2) + 1 and column floor(j/2)
 *  4. At row floor(i/2) + 1 and column floor(j/2) + 1
 *  
 *  Note that in the cases where either i or j is odd, it is not
 *  necessarily the case that either row floor(i/2) + 1 or
 *  column floor(j/2) + 1 are in bounds in the input image.
 *  Only input pixels that are properly in bounds should be incorporated into
 *  the averages used to determine the color components and alpha value
 *  of the output pixel.
 *  
 *  Averages should be computed using purely integer arithmetic with
 *  no rounding.
 * 
 *  @param input_img pointer to the input Image
 *  @param output_img pointer to the output Image (in which the
 *                    transformed pixels should be stored)
 */
	.globl imgproc_expand
imgproc_expand:
	/* TODO: implement */
	ret

/*
vim:ft=gas:
*/
