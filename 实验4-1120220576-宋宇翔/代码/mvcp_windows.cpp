

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAXN 1024
WIN32_FIND_DATA lpfindfiledata;

void CopyFile(char* source_file, char* dest_file) //复制文件
{
	WIN32_FIND_DATA lpfindfiledata;
	HANDLE hfindfile = FindFirstFile(source_file, &lpfindfiledata);
	HANDLE hsource = CreateFile(source_file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hdest_file = CreateFile(dest_file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	// 句柄获取

	LONG size = lpfindfiledata.nFileSizeLow - lpfindfiledata.nFileSizeHigh;
	int* buffer = new int[size];
	DWORD temp;
	bool tmp = ReadFile(hsource, buffer, size, &temp, NULL);
	WriteFile(hdest_file, buffer, size, &temp, NULL);
	// 复制文件时间

	SetFileTime(hdest_file, &lpfindfiledata.ftCreationTime, &lpfindfiledata.ftLastAccessTime, &lpfindfiledata.ftLastWriteTime);
	
	SetFileAttributes(dest_file, GetFileAttributes(source_file));
	// 设置文件属性

	CloseHandle(hfindfile);
	CloseHandle(hsource);
	CloseHandle(hdest_file);
	// 关闭句柄
}

void CopyDir(char* source_file, char* dest_file) //复制
{
	WIN32_FIND_DATA lpfindfiledata;
	char dir_source[MAXN], destination[MAXN];
	strcpy_s(dir_source, source_file);
	strcpy_s(destination, dest_file);
	strcat_s(dir_source, "\\*.*");
	strcat_s(destination, "\\");
	HANDLE hfindfile = FindFirstFile(dir_source, &lpfindfiledata);
	while (FindNextFile(hfindfile, &lpfindfiledata) != 0) //遍历
	{
		if (lpfindfiledata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //目录
		{
			if (strcmp(lpfindfiledata.cFileName, ".") != 0 && strcmp(lpfindfiledata.cFileName, "..") != 0)
			{
				memset(dir_source, 0, sizeof(dir_source));
				strcpy_s(dir_source, source_file);
				strcat_s(dir_source, "\\");
				strcat_s(dir_source, lpfindfiledata.cFileName);
				strcat_s(destination, lpfindfiledata.cFileName);
				CreateDirectory(destination, NULL);
				CopyDir(dir_source, destination);
				//复制文件时间
				HANDLE handle_source = CreateFile(dir_source, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				HANDLE handle_destination = CreateFile(destination, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				FILETIME createtime, accesstime, writetime;
				GetFileTime(handle_source, &createtime, &accesstime, &writetime);
				SetFileTime(handle_destination, &createtime, &accesstime, &writetime);
				SetFileAttributes(destination, GetFileAttributes(dir_source));
				strcpy_s(destination, dest_file);
				strcat_s(destination, "\\");
			}
		}
		else
		{
			memset(dir_source, 0, sizeof(dir_source));
			strcpy_s(dir_source, source_file);
			strcat_s(dir_source, "\\");
			strcat_s(dir_source, lpfindfiledata.cFileName);
			strcat_s(destination, lpfindfiledata.cFileName);
			CopyFile(dir_source, destination);
			strcpy_s(destination, dest_file);
			strcat_s(destination, "\\");
		}
	}
	CloseHandle(hfindfile);
}

int Legal(int argc, char* argv[]) // 
{
	if (argc != 3)
	{
		printf("Illegal Input.\n");
		printf("Format: .\\mycp.exe <path> <path> \n");
		return -1;
	} // 参数出错

	if (FindFirstFile(argv[1], &lpfindfiledata) == INVALID_HANDLE_VALUE)
	{
		printf("Cannot Find Path!\n");
		return -1;
	} // 找不到路径

	int result;
	struct _stat buf;
	result = _stat(argv[1], &buf);

	if (_S_IFREG & buf.st_mode)
	{
		printf("Wrong Name!\n");
		return -1;
	}
	// 检查是否为文件夹

	if (FindFirstFile(argv[2], &lpfindfiledata) == INVALID_HANDLE_VALUE)
	{
		CreateDirectory(argv[2], NULL); //创建目标文件目录
		printf("Creating File Completed!\n");
	}
	// 创建新目录
	return 0;
}

int main(int argc, char* argv[])
{
	if (Legal(argc, argv))
		return -1;
	// 输入不合法

	CopyDir(argv[1], argv[2]);
	//设置目录时间属性一致

	HANDLE handle_source = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	HANDLE handle_destination = CreateFile(argv[2], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	// 文件句柄与目录句柄
	
	FILETIME createtime, accesstime, writetime;
	GetFileTime(handle_source, &createtime, &accesstime, &writetime);
	SetFileTime(handle_destination, &createtime, &accesstime, &writetime);
	// 修改文件时间

	SetFileAttributes(argv[2], GetFileAttributes(argv[1]));
	// 设置属性
	
	printf("Complete\n");
	return 0;
}
