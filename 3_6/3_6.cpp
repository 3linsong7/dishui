// 3_6.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

char arr[] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x07,0x09,					
	0x00,0x20,0x10,0x03,0x03,0x0C,0x00,0x00,0x44,0x00,					
	0x00,0x33,0x00,0x47,0x0C,0x0E,0x00,0x0D,0x00,0x11,					
	0x00,0x00,0x00,0x02,0x64,0x00,0x00,0x00,0xAA,0x00,					
	0x00,0x00,0x64,0x10,0x00,0x00,0x00,0x00,0x00,0x00,					
	0x00,0x00,0x02,0x00,0x74,0x0F,0x41,0x00,0x00,0x00,					
	0x01,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x0A,0x00,					
	0x00,0x02,0x74,0x0F,0x41,0x00,0x06,0x08,0x00,0x00,					
	0x00,0x00,0x00,0x64,0x00,0x0F,0x00,0x00,0x0D,0x00,					
	0x00,0x00,0x23,0x00,0x00,0x64,0x00,0x00,0x64,0x00					

};

int fun(){
	int (*px)[2]; 
	int (*py)[2][3]; 
	char (*pz)[2];
	char (*pk)[2][3];

	px = (int (*)[2])arr;
	//*(*(px+0)+0)
	printf("%x\n",*(*(px + 0) + 0));  //0x03020100
	//*(*(px+1)+0)
	printf("%x\n",*(*(px + 1) + 0));	//4 * 2 * 1 + 0 = 8    0x20000907
	//*(*(px+2)+3)
	printf("%x\n",*(*(px + 2) + 3));	//4 * 2 * 2 + 4 * 3 =28    0x00001100

	py = (int (*)[2][3])arr;
	//*(*(*(py+1)+2)+3)
	printf("%x\n",*(*(*(py + 1) + 2) + 3));	//4 * 2 * 3 * 1 + 4 * 3 * 2 + 4 * 3 =60    0x00000001

	pz = (char (*)[2])arr;
	//*(*(pz+2)+3)
	printf("%x\n",*(*(pz + 2) + 3));	//1 * 2 * 2 + 1 * 3 = 7   0x07

	pk = (char (*)[2][3])arr;
	//*(*(*(pk+2)+3)+4)
	printf("%x\n",*(*(*(pk + 2) + 3) + 4));		//1 * 2 * 3 * 2 + 1 * 3 * 3 + 1 * 4 = 25	0x0e
	return 0;
}

int main(int argc, char* argv[])
{
	fun();	
	return 0;
}

