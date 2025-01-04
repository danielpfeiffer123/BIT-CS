#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <utime.h>
#include <sys/time.h>
#include <fcntl.h>

#define KB 1024

void SyncFile(char* source,char* dest)
{
	struct stat statbuf;  
	struct utimbuf timeby;
	stat(source, &statbuf); 
	timeby.actime = statbuf.st_atime; 
	timeby.modtime = statbuf.st_mtime;
	utime(dest, &timeby);
}

void SyncSoftLink(char* source,char* dest)//同步软链接
{
	//同步软链接信息
	struct stat statbuf;
	lstat(source, &statbuf);
	struct timeval ftime[2];
	ftime[0].tv_usec = 0;
	ftime[0].tv_sec = statbuf.st_atime;
	ftime[1].tv_usec = 0;
	ftime[1].tv_sec = statbuf.st_mtime;
	lutimes(dest, ftime);
}

int Parse(int argc, char *argv[]) //Check
{

	if (argc != 3)
	{
		printf("Illegal parameters\n");
		printf("please input as regulated: ./mycp.exe <path> <path> \n");
		return -1;
	}

	DIR *dir=opendir(argv[1]);
	int file=open(argv[1],O_RDONLY);
	if(dir==NULL&&file==-1)
	{
		printf("Unknown path\n");
		close(file);
		closedir(dir);
		return -1;
	}
	//源文件存在，判断类型
	struct stat statbuf;
	lstat(argv[1], &statbuf);
	if (S_IFREG & statbuf.st_mode)
	{
		close(file);
		closedir(dir);
		return 1;
	}
	else//目录
	{
		if ((dir = opendir(argv[2])) == NULL)//保证目标目录存在
		{
			mkdir(argv[2], statbuf.st_mode);
			printf("创建%s目录\n",argv[2]);
		}
		close(file);
		closedir(dir);
		return 0;
	}
}

void LinkFileCopy(char *source, char *dest) //复制软链接
{
	//复制软链接
    char buffer[2 * KB];
    char oldpath[KB];
    getcwd(oldpath, sizeof(oldpath));
    strcat(oldpath, "/");
    memset(buffer, 0, sizeof(buffer));
    readlink(source, buffer, 2 * KB);//读取软链接到buffer
    symlink(buffer, dest);//将软链接赋给dest
}

void CopyFile(char *source, char *target) // 直接复制
{
    //打开与创建文件
	struct stat statbuf;
    stat(source, &statbuf);
	int _source = open(source, 0); //打开
    int _target = creat(target, statbuf.st_mode); //创建
	
	//传输文件
	char BUFFER[KB];
	int wordbit; 
    while ((wordbit = read(_source, BUFFER, KB)) > 0)
    {
        //写入
        if (write(_target, BUFFER, wordbit) != wordbit)
        {
            printf("error when writing buffer!\n");
            exit(-1);
        }
    }

	//关闭文件
    close(_source); 
    close(_target);
}

void CopyDir(char *source, char *dest) // 将源目录信息复制到目标目录下
{
    char original_path[KB / 2];//两个path是临时路径，用于构造各种路径。
    char destination[KB / 2];
	
    //打开源目录
    DIR *dir;
	if (NULL == (dir = opendir(source)))//打开目录,返回指向DIR结构的指针
	{
		printf("error when opening source\n");
		exit(-1);
	}

	//递归复制目录
	memset(destination,0,sizeof(destination));
    strcpy(destination, dest);
    strcat(destination, "/"); 
	struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)//遍历源目录
    {
		//根据类型进行处理
        if (entry->d_type == 4) // 目录文件
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
			//正常目录，构造路径
            memset(original_path, 0, sizeof(original_path));
            strcpy(original_path, source);
            strcat(original_path, "/");
            strcat(original_path, entry->d_name);
			memset(destination,0,sizeof(destination));
			strcpy(destination, dest);
			strcat(destination, "/");
            strcat(destination, entry->d_name);
			struct stat statbuf;
            stat(original_path, &statbuf);
            mkdir(destination, statbuf.st_mode); 

            CopyDir(original_path, destination);

            SyncFile(original_path,destination);
        }
        else if (entry->d_type == 10) // 软链接文件
        {
			//构造路径
            memset(original_path, 0, sizeof(original_path));
			strcpy(original_path, source);
			strcat(original_path, "/");
			strcat(original_path, entry->d_name);
			memset(destination,0,sizeof(destination));
			strcpy(destination, dest);
			strcat(destination, "/");
			strcat(destination, entry->d_name);
			//复制软链接
            LinkFileCopy(original_path, destination);
			//同步信息，使用软链接的同步函数
			SyncSoftLink(original_path,destination);
        }
        else // 普通文件
        {
            //构造路径
			memset(original_path, 0, sizeof(original_path));
			strcpy(original_path, source);
			strcat(original_path, "/");
			strcat(original_path, entry->d_name);
			memset(destination,0,sizeof(destination));
			strcpy(destination, dest);
			strcat(destination, "/");
			strcat(destination, entry->d_name);
			//复制软链接
			CopyFile(original_path, destination);
			//同步信息
			SyncFile(original_path,destination);
        }
    }
    closedir(dir);
}


int main(int argc, char *argv[])
{
	int copy_stat=Parse(argc, argv);
    if(copy_stat==-1)//异常
	{
		return -1;
	}
	else if(copy_stat==1)//标准文件
	{
		CopyFile(argv[1],argv[2]);
		SyncFile(argv[1],argv[2]);
		printf("File copying completed\n");
		
		return 1;
	}
	else if(copy_stat==0)//目录
	{
		CopyDir(argv[1], argv[2]); //cpoy
		SyncFile(argv[1],argv[2]); //synchronize
		printf("Direction copying completed\n");
		
		return 0;
	}
        
    return 0;    
}

