#include <iostream>
#include <windows.h>

#define BUFSIZE 4   //缓冲区大小
#define STRLEN 1   //字符串长度
#define PRO_TYPE 0  //生产者标识
#define PROERNUM 3  //生产者数量
#define PRONUM 4   //每个生产者生产的数据量
#define CON_TYPE 1  //消费者标识
#define CONERNUM 4  //消费者数量
#define CONNUM 3    //每个消费者消费的数据量

static LPCTSTR filemapping_name = "FileMapping";
char name[4] = "SYX";
HANDLE ProcessHandle[5];
using namespace std;

//缓存结构体
typedef struct BUF {
    char buf[BUFSIZE][STRLEN + 1];
    int head;
    int tail;
} buffer;

//返回随机休眠时间
int getRandomSleep() {
    return rand() % 2900 + 100;
}

//返回随机字符
char getRandomChar() {
    return name[(rand() % 3)] ;
}

//定义P操作
DWORD P(HANDLE S) {
    return WaitForSingleObject(S, INFINITE);
}
//定义V操作
BOOL V(HANDLE S) {
    return ReleaseSemaphore(S, 1, nullptr);
}

//创建子进程
void startChildProcess(const int type, const int id) {
    TCHAR filename[MAX_PATH];
    GetModuleFileName(nullptr, filename, MAX_PATH);

    TCHAR cmdLine[MAX_PATH];
    sprintf(cmdLine, "\"%s\" %d", filename, type);

    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi;

    //新建子进程
    BOOL bCreateOK = CreateProcess(
            filename,
            cmdLine,
            nullptr,
            nullptr,
            FALSE,
            CREATE_DEFAULT_ERROR_MODE,
            nullptr,
            nullptr,
            &si,
            &pi);
    if (bCreateOK) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        ProcessHandle[id] = pi.hProcess;
    } else {
        printf("child process error!\n");
        exit(0);
    }
}

//主进程程序
void useParentProcess() {
    //创建信号量
    HANDLE MUTEX = CreateSemaphore(nullptr,
                                   1,
                                   1,
                                   "mutex");
    HANDLE EMPTY = CreateSemaphore(nullptr,
                                   BUFSIZE,
                                   BUFSIZE,
                                   "empty");
    HANDLE FULL = CreateSemaphore(nullptr,
                                  0,
                                  BUFSIZE,
                                  "full");

    //创建共享内存
    HANDLE hMapping = CreateFileMapping(
            nullptr,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(struct BUF),
            filemapping_name);
    if (hMapping != INVALID_HANDLE_VALUE) {
        LPVOID pFile = MapViewOfFile(
                hMapping,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                0);
        if (pFile != nullptr) {
            ZeroMemory(pFile, sizeof(struct BUF));
        }
        //共享内存的初始化
        buffer *pBuf = reinterpret_cast<buffer *>(pFile);
        memset(pBuf->buf, 0, sizeof(pBuf->buf));
        pBuf->head = 0;
        pBuf->tail = 0;

        //解除映射
        UnmapViewOfFile(pFile);
        pFile = nullptr;
        CloseHandle(pFile);
    }

    //创建3个生产者，4个消费者
    int childProcessNum = PROERNUM + CONERNUM;
    int i;
    for (i = 0; i < childProcessNum; ++i) {
        int type = i < PROERNUM ? PRO_TYPE : CON_TYPE;
        if (i < PROERNUM){
            printf("Create Pro %d\n", i+1);
        }
        else{
            printf("Create Con %d\n", i-PROERNUM+1);
        }
        startChildProcess(type, i);
    }
    //等待子进程运行
    WaitForMultipleObjects(childProcessNum,
                           ProcessHandle,
                           TRUE,
                           INFINITE);
    //回收信号量
    CloseHandle(EMPTY);
    CloseHandle(FULL);
    CloseHandle(MUTEX);
}

//生产者
void useProducerProcess(HANDLE MUTEX, HANDLE EMPTY, HANDLE FULL, buffer *pbuf) {
    int i;
    int iter = 1;
    for (i = 0; i < PRONUM; ++i) {
        P(EMPTY);
        Sleep(getRandomSleep());
        P(MUTEX);
        //生产数据
        char string[STRLEN + 1];
        memset(string, 0, sizeof(string));
        for (int j = 0; j < STRLEN; ++j) {
            string[j] = getRandomChar();
        }
        strcpy(pbuf->buf[pbuf->tail], string);
        pbuf->tail = (pbuf->tail + 1) % BUFSIZE;

        //打印时间
        time_t t = time(nullptr);
        struct tm *ptm = localtime(&t);
        printf("\n iter Producer:%d", iter);
        iter++;

        //打印生产数据、时间和缓冲区映像
        printf("\nProducerID:%6d push %-2s\t|%-2s|%-2s|%-2s|%-2s|",
               (int) GetCurrentProcessId(),
               string, pbuf->buf[0], pbuf->buf[1], pbuf->buf[2],
               pbuf->buf[3]);

        //刷新缓冲区
        fflush(stdout);

        V(MUTEX);
        V(FULL);
    }
}

//消费者
void useCustomerProcess(HANDLE MUTEX, HANDLE EMPTY, HANDLE FULL, buffer *pbuf) {
    int i;
    int iter = 1;
    for (i = 0; i < CONNUM; ++i) {
        P(FULL);
        Sleep(getRandomSleep());
        P(MUTEX);

        //消费数据
        char string[STRLEN + 1];
        memset(string, 0, sizeof(string));
        strcpy(string, pbuf->buf[pbuf->head]);
        memset(pbuf->buf[pbuf->head], 0, sizeof(pbuf->buf[pbuf->head]));
        pbuf->head = (pbuf->head + 1) % BUFSIZE;

        //打印时间
        time_t t = time(nullptr);
        struct tm *ptm = localtime(&t);
        printf("\n iter Customer:%d", iter);
        iter++;

        //打印消费数据、时间和缓冲区映像
        printf("\nCustomerID:%6d pop %-2s\t|%-2s|%-2s|%-2s|%-2s|",
               (int) GetCurrentProcessId(),
               string, pbuf->buf[0], pbuf->buf[1], pbuf->buf[2],
               pbuf->buf[3]);

        //刷新缓冲区
        fflush(stdout);

        V(EMPTY);
        V(MUTEX);
    }
}

//子进程程序
void useChildProcess(int type) {
    //打开信号量
    HANDLE MUTEX = OpenSemaphore(SEMAPHORE_ALL_ACCESS,
                                 FALSE,
                                 "mutex");
    HANDLE EMPTY = OpenSemaphore(SEMAPHORE_ALL_ACCESS,
                                 FALSE,
                                 "empty");
    HANDLE FULL = OpenSemaphore(SEMAPHORE_ALL_ACCESS,
                                FALSE,
                                "full");

    //打开共享内存区，并加载到当前进程地址空间
    HANDLE hmap = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            filemapping_name);
    LPVOID pFile = MapViewOfFile(
            hmap,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            0);
    buffer *pBuf = reinterpret_cast<buffer *>(pFile);

    //根据参数判断子进程是生产者或消费者
    if (type == PRO_TYPE) {
        useProducerProcess(MUTEX, EMPTY, FULL, pBuf);
    } else if (type == CON_TYPE) {
        useCustomerProcess(MUTEX, EMPTY, FULL, pBuf);
    } else {
        printf("maybe child process error!\n");
        exit(-1);
    }

    //解除映射
    UnmapViewOfFile(pFile);
    pFile = nullptr;
    CloseHandle(pFile);
    //关闭信号量
    CloseHandle(MUTEX);
    CloseHandle(EMPTY);
    CloseHandle(FULL);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        //有参数则为子进程
        srand(GetCurrentProcessId());
        int type = atoi(argv[1]);
        useChildProcess(type);
    } else {
        //无参数则为主进程
        useParentProcess();
        system("pause");
    }
    return 0;
}
