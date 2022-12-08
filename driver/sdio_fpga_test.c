#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "time_utils.h"
#include <sys/mman.h>
#include <sys/ioctl.h>

const char* device_name = "/dev/HeygearsSDIO0";
#define HEYGEARS_FPGA_SDIO 'h' //魔数
#define FPGA_SDIO_WRITE   _IOWR(HEYGEARS_FPGA_SDIO,0x1010,unsigned long)
#define FPGA_SDIO_READ    _IOWR(HEYGEARS_FPGA_SDIO,0x1011,unsigned long)
#define FPGA_MEM_ALLOC    _IOWR(HEYGEARS_FPGA_SDIO,0x1012,unsigned long)
 cv::Mat LCDConversion(const cv::Mat& src, const std::string& projectorType)
 {
 	int srcCols = src.cols;
 	int srcRows = src.rows;

 	if (srcCols % 3 || src.channels() == 3)
 	{
 		std::cout << "Image Width should be 3X" << srcRows << ", " << srcCols << std::endl;
 		return cv::Mat();
 	}

 	int dstCols = srcCols / 3;
 	int dstRows = srcRows;
 	cv::Mat dstImg = cv::Mat::zeros(dstRows, dstCols, CV_8UC3);

	if (projectorType == "pj623")
 	{
 		std::cout << __FUNCTION__ << __LINE__ << "Projector Type:" << projectorType << ", " << srcRows << ", " << srcCols << std::endl;

 		typedef uchar Pixel;
 		src.forEach<Pixel>([&](Pixel& pixel, const int* position) -> void
 			{
 				auto px = position[1];
 				auto py = position[0];

 				int ch = (px + 3) % 3;
 				int cIdx = px / 3;

 				dstImg.at<cv::Vec3b>(py, cIdx )[ch] = pixel;
 			});


 		return dstImg;
 	}


 	std::cout << __FUNCTION__ << "Projector Type UNKNOWN:" << projectorType << std::endl;
 	return cv::Mat();
 }


 cv::Mat img_transform(cv::Mat src)
 {
    
 	src = LCDConversion(src, "pj623");
 	return src;

 }



int main()
{
    int i;
   // char *src;
	  cv::Mat dts ,src;
    int fd;
    unsigned char *buf;
    src = cv::imread("test2.png", cv::IMREAD_GRAYSCALE);
//    dts = cv::imread("test1.png", cv::IMREAD_GRAYSCALE);
    src = img_transform(src);
 //   dts = img_transform(dts); 
    printf("SDIO Port Write\n");

    fd = open(device_name,O_RDWR);

    if(fd == -1)                        /* Error Checking */
        printf("  Error! in Opening %s  \n", device_name);
    else
        printf("  %s Opened Successfully \n", device_name);

    ioctl(fd,FPGA_MEM_ALLOC,1920*3600*3);
    printf("%d %s \n",__LINE__,__func__);
    buf= (unsigned char *)mmap(NULL,src.rows*src.cols*3,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    if(buf == NULL){
	    printf("mmap filed \n");
    }
 //   memcpy(buf, "hello world!!!", 15);
    //msleep(100);

    uint64_t start_usec = TimeUsec();
   // write(fd, (char *)src.data, src.rows*src.cols*3);
    memcpy(buf+2444, src.data,4096);
    uint64_t end_usec = TimeUsec();
    printf("Delta tume %i usec\n", (int)(end_usec-start_usec));
     start_usec = TimeUsec();
   // write(fd, (char *)src.data, src.rows*src.cols*3);
//    memcpy(buf, src.data,src.rows*src.cols*3);
//     write(fd, (char *)src.data, src.rows*src.cols*3);
 //   ioctl(fd,FPGA_SDIO_WRITE,1);
     end_usec = TimeUsec();
    printf("2Delta tume %i usec\n", (int)(end_usec-start_usec));


    

//    printf("Bytes write=%d\n", cv::src.size());

//    write(fd, (char *)dts.data, dts.rows*src.cols*3);

//    close(fd);






    
	return 0;
}

