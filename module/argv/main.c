 ///
 /// @file    main.c
 /// @author  faith(kh_faith@qq.com)
 /// @date    2018-06-19 14:00:15
 ///
 
#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;

void query(char *a,char *b)
{
	if(0 == strcmp(a,b))
		strcpy(a,"hello");
}

int main()
{
	char buf1[1]={0};
	char buf2[]="123456";

	memcpy(buf1,buf2+2,2);
	printf("%s\n",buf1);
	printf("%s\n",buf2);
	return 0;
}
