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
	 *   %edi - hold the pixel value for helper function calls (temp)

	 *   %r12d - target pixel value (temp)
	 *   %r13d  - original r component (temp)
	 *   %r14d  - original g component (temp)
	 *   %r15d  - original b component (temp)
	 *   %rbx  - original a component (temp)
	 *
	 *   %eax - rotted pixel (output)
	 */
	# save callee-saved registers we will use
    pushq %rbp
    movq  %rsp, %rbp

	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbx
    subq $8, %rsp          # align stack before calls

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

    addq $8, %rsp       

	popq %rbx
	popq %r15
	popq %r14
	popq %r13
	popq %r12

	popq %rbp
	ret

# TODO
	.global blur_pixel
blur_pixel:

	ret

/* 
	Helper function to compute the average of a group of pixels. 
	The number of pixels to average is determined by the input parameter 
	num_pixels. The pixel values to average are stored in an array 
	pointed to by pixel_values. 
	The output is the average pixel value.
	Alpha will not also be averaged
*/
	.global avg_pixels
avg_pixels:
	/*
	 * Register use:
	 *   %rdi: pointer to array of pixel values (input)
	 *   %esi: number of pixels to average (input)
	 *
	 *   %r12d: r total (temp)
	 *   %r13d: g total (temp)
	 *   %r14d: b total (temp)
	 *   %r15d: a total (temp)

	 *  %rbx: loop index (temp)
	 *
	 * Memory use:
	 *  -8(%rbp): pointer to array of pixel values (temp)
	 *  -16(%rbp): number of pixels to average (temp)
	 */

	pushq %rbp
	movq %rsp, %rbp
	subq $40, %rsp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbx

	# Store the input values on the stack
	movq %rdi, -8(%rbp) /* -8(%rbp) = pointer to array of pixel values */
	movq %rsi, -16(%rbp) /* -16(%rbp) = number of pixels to average */

	# Set up component total variables
	movl $0, %r12d /* r total */
	movl $0, %r13d /* g total */
	movl $0, %r14d /* b total */
	movl $0, %r15d /* a total */

	# Loop through the pixel values and add up the r, g, b, a components
	movl $0, %ebx /* loop index */
	.L_avg_loop:
		cmpl -16(%rbp), %ebx
		jge .L_avg_loop_end /* if loop index (ebx) >= number of pixels, exit loop */

		# Get the pixel value at index ebx
		movq -8(%rbp), %rdi /* get pointer to array of pixel values from stack */
		movl (%rdi, %rbx, 4), %eax /* eax = pixel value at index ebx */

		# Get the r, g, b, a and add to totals
		movl %eax, %edi /* edi = pixel value */
		call get_r
		addl %eax, %r12d /* add r to r total */
		call get_g
		addl %eax, %r13d /* add g to g total */
		call get_b
		addl %eax, %r14d /* add b to b total */
		call get_a
		addl %eax, %r15d /* add a to a total */

		incl %ebx /* increment loop index */
		jmp .L_avg_loop /* loop again */
	.L_avg_loop_end:

	# Compute average (use the same registers) 
	movl %r12d, %eax /* we want to do eax / <num_pixels> */
	xorl %edx, %edx /* edx = 0, will be used for division */
	divl -16(%rbp) /* eax now have the average r value */
	movl %eax, %r12d /* save average r to r12d */

	movl %r13d, %eax /* eax = g total */
	xorl %edx, %edx /* edx = 0 */
	divl -16(%rbp) /* eax now have the average g value */
	movl %eax, %r13d /* save average g to r13d */

	movl %r14d, %eax /* eax = b total */
	xorl %edx, %edx /* edx = 0 */
	divl -16(%rbp) /* eax now have the average b value */
	movl %eax, %r14d /* save average b to r14d */

	movl %r15d, %eax /* eax = a total */
	xorl %edx, %edx /* edx = 0 */
	divl -16(%rbp) /* eax now have the average a value */
	movl %eax, %r15d /* save average a to r15d */

	# Call the build pixel helper
	movl %r12d, %edi /* r component for make_pixel */
	movl %r13d, %esi /* g component for make_pixel */
	movl %r14d, %edx /* b component for make_pixel */
	movl %r15d, %ecx /* a component for make_pixel */
	call make_pixel /* returns average pixel in eax */

	popq %rbx
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	addq $40, %rsp
	popq %rbp
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
	/*
	 *   %rdi: pointer to input Image struct (input)
	 *   %rsi: pointer to output Image struct (input)
	 *   %edx: xfac (input)
	 *   %ecx: yfac (input)
	 *
	 *   %rdx input image data pointer (temp)
	 *   %rbx output image data pointer (temp)
	 *   %r12d row index for loop (temp)
	 *   %r13d column index for loop (temp)
	 *   %r14d selected input row (temp)
	 *   %r15d selected input col (temp)
	 *
	 *   reserved registers:
	 *   %rdi reserved to call compute index
	 *   %rsi reserved to call compute index
	 *   %rdx reserved to call compute index
	 *
	 *   Stack usage:
	 *   -4(%rbp): input image width (temp)
	 *   -8(%rbp): input image height (temp)
	 *   -12(%rbp): output image width (temp)
	 *   -16(%rbp): output image height (temp)
	 *   -20(%rbp): xfac (temp)
	 *   -24(%rbp): yfac (temp)
	 *   -32(%rbp): input image pointer (temp)
	 *   -40(%rbp): output image pointer (temp)
	 *   -48(%rbp): input image data pointer (temp)
	 *   -56(%rbp): output image data pointer (temp)
	 */

	pushq %rbp
	movq %rsp, %rbp
	subq $56, %rsp /* allocate space on stack for 5 callee-saved registers on stack */
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbx

	# save image dimensions from input/output to stack
	movl IMAGE_WIDTH_OFFSET(%rdi), %eax        /* eax = input_img->width */
	movl %eax, -4(%rbp)                         /* -4(%rbp) = input_img->width */
	movl IMAGE_HEIGHT_OFFSET(%rdi), %eax       
	movl %eax, -8(%rbp)		                   /* -8(%rbp) = input_img->height */
	movl IMAGE_WIDTH_OFFSET(%rsi), %eax        
	movl %eax, -12(%rbp)                       /* -12(%rbp) = output_img->width */
	movl IMAGE_HEIGHT_OFFSET(%rsi), %eax       
	movl %eax, -16(%rbp)                       /* -16(%rbp) = output_img->height */
	movl %edx, -20(%rbp)                       /* -20(%rbp) = xfac */
	movl %ecx, -24(%rbp)                       /* -24(%rbp) = yfac */

	# save the struct pointers on stack too
	movq %rdi, -32(%rbp) /* save input image pointer on stack */
	movq %rsi, -40(%rbp) /* save output image pointer on stack */

	# load data pointer from struct (memory read)
	movq IMAGE_DATA_OFFSET(%rdi), %rdx   
	movq %rdx, -48(%rbp) 	/* -48(%rbp) = input_img->data, still a pointer */
	movq IMAGE_DATA_OFFSET(%rsi), %rdx
	movq %rdx, -56(%rbp)    /* -56(%rbp) = output_img->data, still a pointer */


	# loop through output image pixels and select corresponding input pixels to copy over
	movl $0, %r12d  /* row index for loop */
	movl $0, %r13d  /* column index for loop */
	.L_row_loop:
		cmpl -16(%rbp), %r12d          
		jge .L_done_row_loop /* if row (r12d) >= output_img height (%-16(%rbp)) exit */

	.L_col_loop:
		cmpl -12(%rbp), %r13d          
		jge .L_done_col_loop /* if col (r13d) >= output_img width (%-12(%rbp)) exit */

		movl %r12d, %r14d 
		imull -24(%rbp), %r14d /* r14d (input row) = output row index * yfac */
		movl %r13d, %r15d 
		imull -20(%rbp), %r15d /* r15d (input column) = output column index * xfac */

		# check if selected input pixel is in bounds
		cmpl -8(%rbp), %r14d          
		jge .L_continue_if_out_of_bounds /* if input_row (r14d) >= input_img height (%-8(%rbp)) skip */
		cmpl -4(%rbp), %r15d    
		jge .L_continue_if_out_of_bounds /* if input_col (r15d) >= input_img width (%-4(%rbp)) skip */

		# Compute the index for selected input pixel 
		movq -32(%rbp), %rdi 
		movl %r14d, %esi 
		movl %r15d, %edx 
		call compute_index 
		movl %eax, %r14d /* r14d = input pixel index */

		# Compute the index for output pixel
		movq -40(%rbp), %rdi 
		movl %r12d, %esi 
		movl %r13d, %edx 
		call compute_index 
		movl %eax, %r15d /* r15d = output pixel index */

		# Get the data ptr from stack
		movq -48(%rbp), %rdx 
		movq -56(%rbp), %rbx 

		# Copy pixel over
		movl (%rdx, %r14, 4), %eax /* eax = input pixel */
		movl %eax, (%rbx, %r15, 4) /* output pixel index = input pixel value */

	.L_continue_if_out_of_bounds:
		incl %r13d 
		jmp .L_col_loop /* loop again if reach here */
	.L_done_col_loop:
		movl $0, %r13d /* col = 0 for next row */

		incl %r12d 
		jmp .L_row_loop /* loop again if reach here */
	.L_done_row_loop:


	popq %rbx
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	addq $56, %rsp
	popq %rbp
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
	/*
	 * Input registers:
	 *   %rdi: pointer to input Image struct (input)
	 *   %rsi: pointer to output Image struct (input)
	 *
	 * Temp registers:
	 *   %ebx: curr loop index  
	 *   %r12: last loop index 
	 *   %rdx: data pointer (base addr) for output img (at the end)
	 *
	 * Memory use:
	 *   -8(%rbp): input image pointer
	 *   -16(%rbp): output image pointer
	 *   -24(%rbp): image width 
	 *   -32(%rbp): image height 
	 *   -40(%rbp): input image data pointer
	 *   -48(%rbp): output image data pointer
	 */

	pushq %rbp
	movq %rsp, %rbp
	
	pushq %r12
	pushq %rbx
	subq $48, %rsp

	# This function is reletively simple, iterate through each pixel and run rot_pixel on it.

	# Save the struct pointers on stack
	movq %rdi, -8(%rbp) /* save input image pointer on stack */
	movq %rsi, -16(%rbp) /* save output image pointer on stack */

	# Save the width and height of one image (they are the same size) on stack
	movl IMAGE_WIDTH_OFFSET(%rdi), %eax        
	movl %eax, -24(%rbp)                       /* -24(%rbp) = image width */
	movl IMAGE_HEIGHT_OFFSET(%rdi), %eax	    
	movl %eax, -32(%rbp)                       /* -32(%rbp) = image height */

	# Save the data pointer from struct
	movq IMAGE_DATA_OFFSET(%rdi), %rdx   
	movq %rdx, -40(%rbp) 	/* -40(%rbp) = input_img->data */
	movq IMAGE_DATA_OFFSET(%rsi), %rdx   
	movq %rdx, -48(%rbp) 	/* -48(%rbp) = output_img->data */

	# Compute the last pixel index and store it in r12d
	movq -8(%rbp), %rdi /* rdi = input image */

	# row is height - 1
	movl -32(%rbp), %esi /* rsi = height */
	decl %esi

	# col is width - 1
	movl -24(%rbp), %edx
	decl %edx

	# Call compute_index to get the last pixel index
	call compute_index 
	movl %eax, %r12d /* r12d = last pixel index */

	movl $0, %ebx  /* curr pixel index */

	# loop through each pixel index and run rot_pixel on it
	.L_rot_loop: 
		cmpl %r12d, %ebx
		jg .L_rot_loop_end  /* if curr index (ebx) > last index (%r12d) exit */

		# Rot the pixel at index ebx and store it in output image
		movq -8(%rbp), %rdi /* input image (param 1) for rot_pixel */
		movl %ebx, %esi /* pixel index (param 2) for rot_pixel */
		call rot_pixel /* returns rotted pixel (a 32 bit number) in eax */

		# Store the rotted pixel in output image at same index
		movq -48(%rbp), %rdx /* get the output image data pointer from stack */
		movl %eax, (%rdx, %rbx, 4) /* Access output_img pixel (rdx) at ebx * 4, put rotted pixel in */

		# Increment and start loop again
		incl %ebx 
		jmp .L_rot_loop
		.L_rot_loop_end:

		addq $48, %rsp
		popq %rbx
		popq %r12

		popq %rbp
	
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
	/*
	 * Register use:
	 *   %rdi: pointer to input Image struct (input)
	 *   %rsi: pointer to output Image struct (input)
	 *
	 *   r12d: row index for loop (temp)
	 *   r13d: column index for loop (temp)
	 *   r14d: output pixel index for this iteration (one for each loop)
	 *
	 *   r8d: input pixel index j/2 and i/2 (temp)
	 *   r9d: input pixel index j/2 and i/2 + 1 (temp)
	 *   r10d: input pixel index j/2 + 1 and i/2 (temp)
	 *   r11d: input pixel index j/2 + 1 and i/2 + 1 (temp)
	 *
	 * Memory use:
	 *   -8(%rbp): input image pointer
	 *   -16(%rbp): output image pointer
	 *   -24(%rbp): input image height
	 *   -28(%rbp): input image width
	 *   -32(%rbp): output image height
	 *   -36(%rbp): output image width
	 *   -44(%rbp): input data pointer
	 *   -52(%rbp): output data pointer
	 *   -60(%rbp): temporary storage for pixel indices
	 *   -64(%rbp): temporary storage for pixel indices
	 *   -68(%rbp): temporary storage for pixel indices
	 *   -72(%rbp): temporary storage for pixel indices
	 */

	pushq %rbp
	movq %rsp, %rbp
	subq $80, %rsp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbx

	# Save the struct pointers on stack
	movq %rdi, -8(%rbp) /* save input image pointer on stack */
	movq %rsi, -16(%rbp) /* save output image pointer on stack */
	movl IMAGE_HEIGHT_OFFSET(%rdi), %eax
	movl %eax, -24(%rbp) /* -24(%rbp) = input image height */
	movl IMAGE_WIDTH_OFFSET(%rdi), %eax
	movl %eax, -28(%rbp) /* -28(%rbp) = input image width */
	movl IMAGE_HEIGHT_OFFSET(%rsi), %eax
	movl %eax, -32(%rbp) /* -32(%rbp) = output image height */
	movl IMAGE_WIDTH_OFFSET(%rsi), %eax
	movl %eax, -36(%rbp) /* -36(%rbp) = output image width */
	movq IMAGE_DATA_OFFSET(%rdi), %rax 
	movq %rax, -44(%rbp) /* -44(%rbp) = input data pointer */
	movq IMAGE_DATA_OFFSET(%rsi), %rax
	movq %rax, -52(%rbp) /* -52(%rbp) = output data pointer */

	movl $0, %r12d  /* row index for loop */
	movl $0, %r13d  /* column index for loop */
	.L_expand_row_loop:
		cmpl -32(%rbp), %r12d          
		jge .L_done_expand_row_loop /* if row (r12d) >= output_img height (%-32(%rbp)) exit */

	.L_expand_col_loop:
		cmpl -36(%rbp), %r13d          
		jge .L_done_expand_col_loop /* if col (r13d) >= output_img width (%-36(%rbp)) exit */

		# Compute index for output pixel
		movq -16(%rbp), %rdi 
		movl %r12d, %esi 
		movl %r13d, %edx 
		call compute_index
		movl %eax, %r14d /* r14d = output pixel index */

		/* Input indices !! For simplicity, we know very clearly that comput_index won't modify rdi, rsi, or rdx */
		# Compute (floor of row/2 and col/2) 
		movq -8(%rbp), %rdi 
		movl %r12d, %esi 
		shrl $1, %esi /* r12d (row index) / 2 */
		movl %r13d, %edx 
		shrl $1, %edx /* r13d (col index) / 2 */
		call compute_index
		movl %eax, %r8d 

		# Compute (floor of row/2 and col/2 + 1)
		addl $1, %edx /* edx = col/2 + 1 */
		call compute_index
		subl $1, %edx /* edx = col/2 */
		movl %eax, %r9d

		# Compute (floor of row/2 + 1 and col/2)
		addl $1, %esi /* esi = row/2 + 1 */
		call compute_index
		movl %eax, %r10d

		# Compute (floor of row/2 + 1 and col/2 + 1)
		addl $1, %edx /* edx = col/2 + 1 */
		call compute_index
		movl %eax, %r11d

		# Check row = even or odd
		testl $1, %r12d /* r12d(row) & 1, jmp to even if zero */
		jz .L_row_even
		jnz .L_row_odd 

		# Amount of pixels to avg (differ based on edge cases)
		movl $0, %r15d 

		# Case 1: both even: At row floor(i/2) and column floor(j/2)
		.L_row_even:
			# Move the pixel value from input(r8d) to output(r14d)
			movq -44(%rbp), %rdx /* input data pointer from stack */
			movl (%rdx, %r8, 4), %eax /* eax = input pixel value */
			jmp .L_done_conditional 

		# Case 2: row even, col odd: At row floor(i/2) and column floor(j/2) + 1
		.L_row_even_col_odd:
			# Two pixels: r8d and maybe r9d
			movl %r8d, -60(%rbp) 
			movl %r9d, -64(%rbp) 

			# call avg_pixels
			movq -60(%rbp), %rdi /* ptr to array of pixel indices for avg_pixels */
			movl %r15d, %esi /* number of pixels to average */
			call avg_pixels /* returns average pixel in eax */
			jmp .L_done_conditional 

		# Case 3: row odd, col even
		.L_row_odd:
			# Jump to case 4 if col is odd
			testl $1, %r13d /* r13d(col) & 1, jmp to odd if non-zero */
			jnz .L_row_odd_col_odd /* if col index is odd jump to row odd col odd */
		
			# Two pixels: r8d and r10d
			movl %r8d, -60(%rbp) 
			movl %r10d, -64(%rbp) 

			# call avg_pixels
			movq -60(%rbp), %rdi /* ptr to array of pixel indices for avg_pixels */
			movl %r15d, %esi /* number of pixels to average */
			call avg_pixels /* returns average pixel in eax */
			jmp .L_done_conditional 

		# Case 4: row odd, col odd
		.L_row_odd_col_odd:
			# Four pixels: r8d, r9d, r10d, r11d
			movl %r8d, -60(%rbp) 
			movl %r9d, -64(%rbp) 
			movl %r10d, -68(%rbp) 
			movl %r11d, -72(%rbp) 

			# call avg_pixels
			movq -60(%rbp), %rdi /* ptr to array of pixel indices for avg_pixels */
			movl %r15d, %esi /* number of pixels to average */
			call avg_pixels /* returns average pixel in eax */

	.L_done_conditional:
		# Store eax (result pixel) to output pixel index (r14d)
		movq -52(%rbp), %rbx /* output data pointer from stack */
		movl %eax, (%rbx, %r14, 4) /* output pixel value = input pixel value */

		incl %r13d 
		jmp .L_expand_col_loop /* loop again if reach here */

	.L_done_expand_col_loop:
		movl $0, %r13d /* col = 0 for next row */
		incl %r12d 
		jmp .L_expand_row_loop /* outer loop again if reach here */
	.L_done_expand_row_loop:

	popq %rbx
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	addq $80, %rsp
	popq %rbp
	ret

/*
vim:ft=gas:
*/
