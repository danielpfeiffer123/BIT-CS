#ifndef DEF_H
#define DEF_H

#include<stdio.h>//标准库 
#include<time.h>
#include<string.h> 
#include<stdlib.h> 

#include<windows.h>//系统库 

//进程个数
#define PRO_NUM 3
#define CON_NUM 4

//重复次数
#define PRO_REP 4
#define CON_REP 3

//缓冲区大小 
#define BUF_LEN 1
#define BUF_CNT 4

//内存，信号量key 
#define SHM_KEY 1234
#define SEM_KEY 1235
#define MUTEX 0
#define EMPTY 1
#define FULL 2

//模式，可读可写 
#define MODE 0600 

// 定义共享内存相关信息
const TCHAR szFileMappingName[] = TEXT("PCFileMappingObject");
const TCHAR szMutexName[] = TEXT("PCMutex");
const TCHAR szSemaphoreEmptyName[] = TEXT("PCSemaphoreEmpty");
const TCHAR szSemaphoreFullName[] = TEXT("PCSemaphoreFull");

//缓冲区结构
struct MyBuffer
{
	char str[BUF_CNT][BUF_LEN];
	int head;
	int tail;
};

//时间变量 
LARGE_INTEGER start_time, end_time;
LARGE_INTEGER freq;
double running_time;

#endif //DEF_H
