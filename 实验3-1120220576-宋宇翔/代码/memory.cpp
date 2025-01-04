#include <Windows.h>
#include<TlHelp32.h>
#include<Psapi.h>
#include<iostream>
#include <algorithm>
#define MAX_NAME_SIZE 1000
#define KILO_BYTES (double)1024
#define MEGA_BYTES (double)1048576
using namespace std;

//已知Issue：MSVC使用O2编译会导致进程快照无法正常拍摄（对应debug可以跑，但是release不能跑），因此在编译release版本时需要将/O2参数改为/Od 或者/O1

//流程：拍内存快照->遍历进程->通过名字找到对应的进程id->通过id打开进程对象->检查其内存信息

//CreateToolhelp32Snapshot + Process32First/Next可以获得进程中的信息

//物理内存使用情况
//系统地址空间的布局
//虚拟地址空间的布局和工作集信息
//显示实验二的虚拟地址空间布局和工作集信息//如何显示一个进程的工作集 ->得到进程对象后  GetProcessWorkingSetSizeEx https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-getprocessworkingsetsizeex
//GetSystemInfo -> 和GetPerformanceInfo差不多 https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsysteminfo
//VirtualQueyEx -> 查看某进程的页空间 https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualqueryex
//GetPerformanceInfo -> 获得系统的信息、CPU数量等 https://docs.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-performance_information
//GlobalMemoryStatusEx -> 得到系统当前的物理和虚拟空间使用情况 https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-globalmemorystatusex


//如果传参是NULL，则展示所有进程的信息，如果传参不是NULL，则查找目标进程的pid。如果pid是DOWRD_MAX则证明没找到
DWORD WatchProcess(const char* target_processname)
{
	HANDLE hProcessesShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessesShot == INVALID_HANDLE_VALUE)
	{
		cout << "open shot handles error" << endl;
		return -2;
	}

	PROCESSENTRY32 now_process;

	now_process.dwSize = sizeof(PROCESSENTRY32);//传入变量地址前还需要声明变量的大小。并且这个大小是在变量自己内部保存的，而并不是在传入函数内部时使用

	//遍历进程
	if (!Process32First(hProcessesShot, &now_process))
	{
		cout << "Find Process Failed" << endl;
	}
	DWORD pid = MAXDWORD;
	do
	{

        //如果要找指定进程的话，使用strncpy比较一下可执行文件即可找到进程id，再通过进程id打开进程对象，再通过进程对象访问对应进程的各种详细信息
        if (strncmp(target_processname, now_process.szExeFile, MAX_NAME_SIZE) == 0)
        {
            pid = now_process.th32ProcessID;
        }

	} while (Process32Next(hProcessesShot, &now_process));

	return pid;
}

void DisplaySystemInfo()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	_PERFORMANCE_INFORMATION performance_info;
	GetPerformanceInfo(&performance_info, sizeof(performance_info));
	cout << "Page Size:" << system_info.dwPageSize << "B" << endl;
    cout << "Maximun Address:" << system_info.lpMaximumApplicationAddress << "    Minimun Address" << system_info.lpMinimumApplicationAddress << endl;


	SIZE_T page_size = performance_info.PageSize;
	cout << "Process Number:" << performance_info.ProcessCount << "    Thread Number:" << performance_info.ThreadCount << "    Sentence Number:" << performance_info.HandleCount << '\n' ;
    cout << "Available Memory:" << (double)(performance_info.PhysicalAvailable * page_size) / (double)(1024 * 1024 * 1024) << "GB    Total Memory:" << (double)(performance_info.PhysicalTotal * page_size) / (double)(1024 * 1024 * 1024) << "GB    Memory Usage:" << (1 - ((double)performance_info.PhysicalAvailable / (double)performance_info.PhysicalTotal)) * 100 << "%    System Usage:" << performance_info.SystemCache << endl;
}

void DisplayVirtualMemoryInfo(HANDLE hProcess)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);//查询页面的基地址的指针
	LPVOID pStart_addr = system_info.lpMinimumApplicationAddress;

	MEMORY_BASIC_INFORMATION memory_basic_information;
	while (VirtualQueryEx(hProcess, pStart_addr, &memory_basic_information, sizeof(memory_basic_information)))
	{
		cout << "Base Address of The Page: " << memory_basic_information.BaseAddress << endl << "Starting Address of Allocated Memory: " << memory_basic_information.AllocationBase << endl;
		/* 一开始分配的内存:
			|AllocBase=BaseAddress\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\|

		然后对中间某块区域修改，假设在BaseAddress处的一篇区域添加了保护属性：
			|AllocBase=BaseAddress0///|BaseAddress1\\\\\\|BaseAddress2///////|

		BaseAddress1和BaseAdress2的AllocBase和BaseAddress0的AllocaBase都是一样的，因为它们一开始来自于同一个VirtualAlloc，但是由于BaseAddress1属性的改变，导致它们不再是属性相同的一整片内存区，因此被拆出了多个BaseAddress
		*/


		//检查页面在不在内存中的硬编码
		cout << "Page Status: ";
		switch (memory_basic_information.State)
		{
		default:
			cout << "Udefined";
			break;
		case MEM_COMMIT:
			cout << "Submitted";//这些页面是有实际的物理地址的
			break;
		case MEM_FREE:
			cout << "Released";//当前进程不能访问，但是可以分配
			break;
		case MEM_RESERVE:
			cout << "Kept";//页面被保留，但是没有分配到任何真实的物理内存中
			break;
		}

        cout << endl;

		cout << "Page Size:" << memory_basic_information.RegionSize << " B" << endl;

		//内存保护类型的硬编码
		cout << "Protection Type: ";
		switch (memory_basic_information.AllocationProtect)
		{
		default:
			cout << "Undefined";
			break;
		case PAGE_EXECUTE:
			cout << "PAGE_EXECUTE";
			break;
		case PAGE_EXECUTE_READ:
			cout << "PAGE_EXECUTE_READ";
			break;
		case PAGE_EXECUTE_READWRITE:
			cout << "PAGE_EXECUTE_READWRITE";
			break;
		case PAGE_EXECUTE_WRITECOPY:
			cout << "PAGE_EXECUTE_WRITECOPY";
			break;
		case PAGE_NOACCESS:
			cout << "PAGE_NOACCESS";
			break;
		case PAGE_READONLY:
			cout << "PAGE_READONLY";
			break;
		case PAGE_READWRITE:
			cout << "PAGE_READWRITE";
			break;
		case PAGE_WRITECOPY:
			cout << "PAGE_WRITECOPY";
			break;
			cout << "未定义";
		}


		cout << endl << "Page Type: ";
		switch (memory_basic_information.Type)
		{
		default:
			cout << "Undefined";
			break;
		case MEM_IMAGE:
			cout << "Memory Imaged";//dll之类的
			break;
		case MEM_MAPPED:
			cout << "Memory Mapped";
			break;
		case MEM_PRIVATE:
			cout << "Private";
			break;
		}
		cout << endl;
		pStart_addr = (PBYTE)pStart_addr + memory_basic_information.RegionSize;
	}

	//检索有关指定进程的虚拟地址空间内的页面范围的信息

	//cout<<memory_basic_information.
}

int main(int argc, char const* argv[])
{

    errno_t	err = 0;
    char	fileName[100] = { 0 };
    char    processFullName[_MAX_PATH] = { 0 };
    char    processName[0x40] = { 0 };
    DWORD   dwpid = 0;
    char *  tmp1 = NULL;
    char *  tmp2 = NULL;

    dwpid = GetCurrentProcessId();
    GetModuleFileNameA(NULL, processFullName, _MAX_PATH); //进程完整路径
//
//    tmp1 = strrchr((char *)processFullName, '\\');
//    tmp2 = strrchr((char *)processFullName, '.');
//    memcpy(processName, tmp1 + 1, std::min(tmp2 - tmp1 - 1, 0x40)); //截取得进程名


	char PROCESS_TO_QUERY[MAX_NAME_SIZE];

    HANDLE handle = GetCurrentProcess();



	while (true)
	{
		DisplaySystemInfo();
		cout << "Type exit to exit the program, or input memory.exe to check the current status of the process:" << endl;
		cin >> PROCESS_TO_QUERY;
		if (strncmp(PROCESS_TO_QUERY, "exit", 6) == 0)
		{
			break;
		}
        std::cout << "Current Process ID: " << dwpid << std::endl;
        std::cout << "Current Process Handle: " << handle << std::endl;
		DWORD pid = WatchProcess(PROCESS_TO_QUERY);

		if (pid != MAXDWORD)
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

			//展示内存页面的分布情况
			DisplayVirtualMemoryInfo(hProcess);
			cout << "Search Completed" << endl;

			SIZE_T min = 0, max = 0;
			DWORD workingset_flag;
			workingset_flag = QUOTA_LIMITS_USE_DEFAULT_LIMITS;//怪事，这个东西的flag传入的是一个指针地址而不是直接传入一个数
			GetProcessWorkingSetSizeEx(hProcess, &min, &max, &workingset_flag);//控制最小和最大工作集的强制执行标志
			//进程的“工作集”是当前在物理RAM内存中对该进程可见的存储页面集。这些页面是常驻页面，可供应用程序使用而不会缺页中断。最小和最大工作集大小会影响进程的虚拟内存分页行为。
			cout << "Process " << PROCESS_TO_QUERY << " (pid " << pid << ")    Mininum Work Set:" << (double)min / KILO_BYTES /*进程处于活动状态时，最少会保存这么多内存，单位为字节*/ << "KB    Maximun Work Set:" << (double)max / KILO_BYTES << "KB"/*进程处于活动状态时，不会保存超过这个大小的工作集*/ << endl;

		}
		system("pause");

	}
	return 0;
}




