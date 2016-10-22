#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv/ml.h>
#include <opencv/cxcore.h>
#include <jpeglib.h>
#include <jerror.h>
#include <jconfig.h>
#include <jmorecfg.h>

typedef unsigned char		byte;
typedef struct _IplImage	IplImage;

void ipl2Jpeg( IplImage *, byte **, long unsigned int *outlen );


