#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv)
{
	time_t now_time;
	struct tm *info;
	char buffer[80];

	time( &now_time );

	info = localtime( &now_time );

	strftime(buffer, 80, "%Y-%m-%d %H:%M:%S %Z", info);
	printf("格式化的日期和时间 : %s \n", buffer );

	return 0;
}
