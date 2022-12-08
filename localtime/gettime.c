#include<stdio.h>
#include<time.h>
#include<sys/time.h>




int  main()
{
    
	time_t timep;

	time(&timep);//获取UTC秒数
 
	//ctime转换为本地时间字符串形式，有时区转换
	printf("timep = %ld, local time = %s", timep, ctime(&timep));
 
	struct timeval tv;
	gettimeofday(&tv, NULL);//获取微秒精度时间结构,UTC无时区转换
	printf("tv.sec = %ld, tv.usec = %ld\n", tv.tv_sec, tv.tv_usec);
 
	struct tm result_gmtime, result_localtime;
	gmtime_r(&timep, &result_gmtime);//获取UTC时间结构，无时区转换
	localtime_r(&timep, &result_localtime);//获取本地时间结构，有时区转换
 
	//时间结构转换为字符串显示，无时区转换
	printf("gmtime_r = %s", asctime(&result_gmtime));
	printf("localtime_r = %s", asctime(&result_localtime));
 
	//mktime将本地时间转换为UTC秒数，有时区转换
	printf("mktime localtime to utc = %ld\n", mktime(&result_localtime));
	//错误值示范，本身是UTC，再减时区结果是错误的
	printf("mktime localtime to utc = %ld\n", mktime(&result_gmtime));
	
	return 0;




}
