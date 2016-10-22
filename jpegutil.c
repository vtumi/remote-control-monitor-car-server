#include "jpegutil.h"

void ipl2Jpeg( IplImage *frame, byte **outbuffer, long unsigned int *outlen )
{
	struct jpeg_compress_struct cinfo = { 0 };
	struct jpeg_error_mgr jerr;
	JSAMPROW row_ptr[1];
	int row_stride;

	IplImage* temp = cvCreateImage( cvGetSize( frame ), frame->depth, frame->nChannels );
	cvCvtColor( frame, temp, CV_BGR2RGB );

	byte *outdata = (byte *) temp->imageData;
	*outbuffer	= NULL;
	*outlen		= 0;

	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress( &cinfo );
	jpeg_mem_dest( &cinfo, outbuffer, outlen );

	cinfo.image_width	= temp->width;
	cinfo.image_height	= temp->height;
	cinfo.input_components	= temp->nChannels;
	cinfo.in_color_space	= JCS_RGB;

	jpeg_set_defaults( &cinfo );
	/* jpeg_set_quality( &cinfo, 20, TRUE ); */
	jpeg_start_compress( &cinfo, TRUE );


	while ( cinfo.next_scanline < cinfo.image_height )
	{
		row_ptr[0] = &outdata[cinfo.next_scanline * temp->widthStep];
		jpeg_write_scanlines( &cinfo, row_ptr, 1 );
	}

	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	cvReleaseImage( &temp );
}


