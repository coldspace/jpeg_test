#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "jpeglib.h"
#include <setjmp.h>
#include "BmpLib\BmpLib.h"

JSAMPLE * image_buffer;	/* Points to large array of R,G,B-order data JSAMPLE = unsigned char */
size_t image_buffer_size = 0;
int image_height;	/* Number of rows in image */
int image_width;		/* Number of columns in image */


GLOBAL(void)
init_image_buffer(size_t i_BuffSize)
{
	image_buffer_size = i_BuffSize;
	image_buffer = (JSAMPLE *)malloc(image_buffer_size);
}

GLOBAL(void)
free_image_buffer()
{
	if (image_buffer == NULL)
		return;

	free(image_buffer);
}

GLOBAL(void)
append_image_buffer(j_decompress_ptr cinfo,JSAMPARRAY buffer, int row_stride)
{
	size_t lo = (cinfo->output_scanline - 1) * cinfo->image_width * cinfo->num_components;
	size_t li = lo + row_stride;
	int i = 0;

	if (li > image_buffer_size)
	{
		li = image_buffer_size;
	}

	for (lo; lo < li; lo++)
	{
		image_buffer[lo] = buffer[0][i];
		i++;
	}
}

GLOBAL(void)
put_scanline_someplace(j_decompress_ptr cinfo, JSAMPARRAY buffer, int row_stride)
{
	append_image_buffer(cinfo, buffer, row_stride);
}

GLOBAL(void)
write_JPEG_file(char * filename, int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	/* More stuff */
	FILE * outfile;		/* target file */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride;		/* physical row width in image buffer */

	/* Step 1: 申请并初始化jpeg压缩对象，同时要指定错误处理器 */

	cinfo.err = jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: 指定压缩后的图像所存放的目标文件，注意目标文件应以二进制模式打开	*/

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return;
	}
	jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: 设置压缩参数 */

	cinfo.image_width = image_width; 	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 1;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_GRAYSCALE; 	/* colorspace of input image */

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = image_width;	/* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
}

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

GLOBAL(int)
read_JPEG_file(char * filename)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	/* More stuff */
	FILE * infile;		/* source file */
	JSAMPARRAY buffer;		/* Output row buffer JSAMPARRAY -> unsigned char **p */
	int row_stride;		/* physical row width in output buffer */

	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return 0;
	}
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return 0;
	}
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, infile);

	(void)jpeg_read_header(&cinfo, TRUE);

	(void)jpeg_start_decompress(&cinfo);  //  研究具体实现功能

	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */

	image_width = cinfo.image_width;
	image_height = cinfo.image_height;
	init_image_buffer(image_width * image_height * cinfo.num_components);

	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	while (cinfo.output_scanline < cinfo.output_height) {
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		put_scanline_someplace(&cinfo, buffer, row_stride);
	}

	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	return 1;
}


void main()
{
	//for (int i = 0; i < 20; i++)
	//{
	//	read_JPEG_file("image/Lena.jpg");

	//	write_JPEG_file("image/aaa.jpg", 100);
	//}
	FileInfo fi;
	FILE *fp;
	fp = fopen("image/Lena.bmp","rb");
	if (fp == NULL)
		return;

	Bmp_Read_FileInfo(fp,&fi);

	image_width = fi.InfoHeader.biWidth;
	image_height = fi.InfoHeader.biHeight;

	init_image_buffer(image_width * image_height);

	for (int i = 0; i < image_width; i++)
	{
		for (int n = 0; n < image_height; n++)
		{
			image_buffer[i*image_width + n] = fi.pBuff[(image_height - i - 1)*image_width + n];
		}
	}

	write_JPEG_file("image/new.jpg", 100);
}