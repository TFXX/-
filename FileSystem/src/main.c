#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 文件名最大
#define NAME_MAX 32

// 用户文件目录大小
#ifdef _DEBUG
#define MSD 3
#else
#define MSD 16
#endif

// 主文件目录大小
#define MFD_SIZE MSD

#define MFD_MEM_SIZE (sizeof(FCB) * MFD_SIZE)

// 用户文件目录大小
#define UFD_SIZE MSD

#define UFD_MEM_SIZE (sizeof(FCB) * UFD_SIZE)

// 磁盘块大小(byte)
#define DISK_SIZE 1024

// 磁盘块数目
#ifdef _DEBUG
#define DISK_NUM 8
#else
#define DISK_NUM 1024
#endif

#define INVALID_DISK -1

#define INVALID_FILE_DESC 0

#define FAT_SIZE DISK_NUM

#ifdef _DEBUG
#define MOFN 2
#else
#define MOFN 1024
#endif

#define OPEN_TABLE_SIZE MOFN + 1

// Fat表项与FAT表
typedef struct stFATItem
{
	// 指向下一个磁盘的指针
	int nextDisk;

	// 磁盘块是否空闲标志位
	_Bool emFlag;
} FATItem, * FAT;

// FAT表
FAT g_FAT = NULL;

void static init_FAT()
{
	g_FAT = (FAT)calloc(DISK_NUM, sizeof(FATItem));
	if (!g_FAT)
	{
		exit(-2);
	}

	for (size_t i = 0; i < DISK_NUM; i++)
	{
		g_FAT[i].emFlag = 1;
		g_FAT[i].nextDisk = -1;
	}
}

void static uninit_FAT()
{
	if (g_FAT)
	{
		free(g_FAT);
	}
}

// 虚拟磁盘起始地址
void* g_pDiskMem = NULL;

void static init_virtual_disk_mem()
{
	g_pDiskMem = calloc(DISK_NUM, DISK_SIZE);
	if (!g_pDiskMem)
	{
		exit(-2);
	}
#ifdef _DEBUG
	printf("虚拟磁盘创建成功\n");
#endif 
}

void static uninit_virtual_disk_mem()
{
	if (g_pDiskMem)
	{
		free(g_pDiskMem);
	}
}

// 申请空闲盘块
int alloc_disk(int startDisk)
{
	// 找到空闲盘块号
	for (; startDisk < DISK_NUM; startDisk++)
	{
		if (1 == g_FAT[startDisk].emFlag)
		{
			break;
		}
	}

	// 没有空闲盘块
	if (DISK_NUM == startDisk)
	{
		return -1;
	}

	// 将FAT对应项进行修改
	g_FAT[startDisk].emFlag = 0;
	g_FAT[startDisk].nextDisk = -1;

	return startDisk;
}

// 连续释放盘块
void free_disk(const int firstDisk)
{
	for (int disk = firstDisk; disk != -1; disk = g_FAT[disk].nextDisk)
	{
		// 将FAT对应项进行修改
		g_FAT[firstDisk].emFlag = 1;
	}
}

// 文件控制块
typedef struct stFCB
{
	// 文件/目录名
	char name[NAME_MAX];

	// 属性：1为目录 0为普通文件
	//_Bool dirFlag;

	// 文件大小
	size_t size;

	// 文件/目录起始盘块号
	int firstDisk;
} FCB;

// 向盘块中写入数据
size_t write_disk(const int disk, const size_t start, const void *buf, size_t len)
{
	if (len > DISK_SIZE - start)
	{
		len = DISK_SIZE - start;
	}

	memcpy((char *)g_pDiskMem + disk * DISK_SIZE + start, buf, len);

	// 返回成功写入的长度
	return len;
}

// 从盘块中读取数据
size_t read_disk(const int disk, const size_t start, void *buf, size_t len)
{
	if (len > DISK_SIZE - start)
	{
		len = DISK_SIZE - start;
	}

	memcpy(buf, (char *)g_pDiskMem + disk * DISK_SIZE + start, len);

	// 返回成功读取的长度
	return len;
}

// 申请一个空闲FCB
size_t alloc_FCB(FCB fileDir[], size_t dirSize)
{
	size_t FCBIndex = 0;
	for (; FCBIndex < dirSize; FCBIndex++)
	{
		if (-1 == fileDir[FCBIndex].firstDisk)
		{
			break;
		}
	}

	return FCBIndex;
}

// 回收FCB
void free_FCB(FCB *pFCB)
{
	pFCB->firstDisk = -1;
}

// 填写FCB
void write_FCB(
	FCB *pFCB, 
	const char name[], 
	const char prop, 
	const int size, 
	const int firstDisk
)
{
	strcpy(pFCB->name, name);
	//fcb->prop = prop;
	pFCB->size = size;
	pFCB->firstDisk = firstDisk;
}

// 当前用户
char* g_szUser = NULL;

// 文件目录
typedef FCB* FileDir;

// 检查文件/UFD是否重名
size_t is_name_exist(const char name[], const FCB fileDir[], const size_t dirSize)
{
	size_t FCBIndex = 0;
	for (; FCBIndex < dirSize; FCBIndex++)
	{
		if (-1 == fileDir[FCBIndex].firstDisk)
		{
			continue;
		}

		if (0 == strcmp(fileDir[FCBIndex].name, name))
		{
			break;
		}
	}

	return FCBIndex;
}

// 主文件目录
FileDir g_MFD = NULL;

// 当前用户文件目录
FileDir g_UFD = NULL;

void static init_MFD()
{
	g_MFD = calloc(MFD_SIZE, sizeof(FCB));
}

void static uninit_MFD()
{
	if (g_MFD)
	{
		free(g_MFD);
	}
}

int load_MFD()
{
	return MFD_MEM_SIZE == read_disk(0, 0, g_MFD, MFD_MEM_SIZE) ? (0) : (-14);
}

int store_MFD()
{
	//MFD过大，无法用一个盘块存储
	return MFD_MEM_SIZE == write_disk(0, 0, g_MFD, MFD_MEM_SIZE) ? (0) : (-13);
}

void static init_UFD()
{
	g_UFD = calloc(MFD_SIZE, sizeof(FCB));
}

void static uninit_UFD()
{
	if (g_UFD)
	{
		free(g_UFD);
	}
}

void static install_MFD()
{
	g_FAT[0].emFlag = 0;
	g_FAT[0].nextDisk = -1;

	for (size_t i = 0; i < MFD_SIZE; i++)
	{
		g_MFD[i].firstDisk = -1;
	}

	int res = store_MFD();

#ifdef _DEBUG
	if (res != 0)
	{
		printf("install_MFD错误%d\n", res);
	}
#endif
}

// 创建UFD
int create_UFD(const char name[])
{
	// 检查文件名长度
	if (strlen(name) > NAME_MAX)
	{
		// 文件名过长
		return -1;
	}

	// 检查MFD是否有空闲FCB
	size_t UFDFCBIndex = alloc_FCB(g_MFD, MFD_SIZE);
	if (MFD_SIZE == UFDFCBIndex)
	{
		// 没有空闲FCB
		return -2;
	}

	// 检查目录是否重名
	if (MFD_SIZE != is_name_exist(name, g_MFD, MFD_SIZE))
	{
		// 目录重名
		return -3;
	}

	// 分配盘块用于存储UFD
	g_MFD[UFDFCBIndex].firstDisk = alloc_disk(0);
	if (-1 == g_MFD[UFDFCBIndex].firstDisk)
	{
		// 磁盘空间不足
		return -21;
	}

	strncpy(g_MFD[UFDFCBIndex].name, name, NAME_MAX);

	FCB *temp = (FCB *)calloc(UFD_SIZE, sizeof(FCB));
	if(!temp)
	{
		exit(-2);
	}

	for (size_t i = 0; i < UFD_SIZE; i++)
	{
		temp[i].firstDisk = -1;
	}

	write_disk(g_MFD[UFDFCBIndex].firstDisk, 0, temp, MFD_MEM_SIZE);

	free(temp);

	// 更新MFD
	store_MFD();

	return 0;
}

int load_UFD()
{
	size_t i = is_name_exist(g_szUser, g_MFD, MFD_SIZE);

	if (MFD_SIZE == i)
	{
		// UFD不存在
		//return (-15);

		int res = create_UFD(g_szUser);
		if (0 != res)
		{
			return res;
		}

		i =	is_name_exist(g_szUser, g_MFD, MFD_SIZE);
	}

	return UFD_MEM_SIZE == read_disk(g_MFD[i].firstDisk, 0, g_UFD, UFD_MEM_SIZE) ? (0) : (-16);
}

int store_UFD()
{
	size_t i;

	for (i = 0; i < UFD_SIZE; i++)
	{
		if (0 == strcmp(g_MFD[i].name, g_szUser))
		{
			break;
		}
	}

	if (UFD_SIZE == i)
	{
		// UFD不存在
		return (-15);
	}
	return UFD_MEM_SIZE == write_disk(g_MFD[i].firstDisk, 0, g_UFD, UFD_MEM_SIZE) ? (0) : (-17);
}

// 文件描述符
typedef size_t FILE_DESC;

// 文件打开表项
typedef FCB* OpenItem, ** OpenTable;

// 文件打开表
OpenTable g_openTable = NULL;

void init_open_table()
{
	g_openTable = calloc(OPEN_TABLE_SIZE, sizeof(OpenItem));

	for (size_t i = 0; i < OPEN_TABLE_SIZE; i++)
	{
		g_openTable[i] = NULL;
	}
}

void uninit_open_table()
{
	if(g_openTable)
	{
		free(g_openTable);
	}
}

// 检查文件是否打开
size_t is_file_open(const FCB* pFCB)
{
	size_t open = OPEN_TABLE_SIZE - 1;
	for (; open > 0; open--)
	{
		if (pFCB == g_openTable[open])
		{
			break;
		}
	}

	return open;
}

// 创建文件
int create_file(const char name[])
{
	if (strlen(name) > NAME_MAX)
	{
		return -1;
	}

	// 检查要创建的文件是否和已经存在的文件重名
	if(UFD_SIZE != is_name_exist(name, g_UFD, UFD_SIZE))
	{
		return -4;
	}

	size_t emFCBIndex = alloc_FCB(g_UFD, UFD_SIZE);
	if (UFD_SIZE == emFCBIndex)
	{
		return -2;
	}

	// 分配空闲盘块
	int emDiskIndex = alloc_disk(0);
	if (DISK_NUM == emDiskIndex)
	{
		return -5;
	}

	// 填写目录项
	write_FCB(g_UFD + emFCBIndex, name, '0', 0, emDiskIndex);

	// 更新UFD
	store_UFD();

	return 0;
}

// 删除文件
int del_file(const char name[])
{
	// 检查被删除的文件是否在目录中
	size_t FCBIndex = is_name_exist(name, g_UFD, UFD_SIZE);
	if (UFD_SIZE == FCBIndex)
	{
		// 文件不在当前目录中
		return -1;
	}

	// 检查被删除的文件是否打开
	if (INVALID_FILE_DESC != is_file_open(g_UFD + FCBIndex))
	{
		// 如果文件打开则不能删除
		return -2;
	}

	// 释放磁盘上的对应空间
	free_disk(g_UFD[FCBIndex].firstDisk);

	// 释放目录项
	free_FCB(g_UFD + FCBIndex);

	// 更新UFD
	store_UFD();

	return 0;
}

// 打开文件
FILE_DESC open_file(const char name[])
{
	// 检查要打开的文件是否存在
	size_t FCBIndex = is_name_exist(name, g_UFD, UFD_SIZE);
	if (UFD_SIZE == FCBIndex)
	{
		return 0;
	}

	// 检查文件是否打开
	if (INVALID_FILE_DESC != is_file_open(g_UFD + FCBIndex))
	{
		// 文件已经打开
		return INVALID_FILE_DESC;
	}

	// 查找一个空闲打开表项
	FILE_DESC emOpen;
	for (emOpen = OPEN_TABLE_SIZE - 1; emOpen > 0; emOpen--)
	{
		if (NULL == g_openTable[emOpen])
		{
			break;
		}
	}

	// 检查打开文件的数量是否达到上限
	if (INVALID_FILE_DESC == emOpen)
	{
		// 打开的文件数量达到上限
		return -3;
	}

	// 填写打开表项的相关信息
	g_openTable[emOpen] = g_UFD + FCBIndex; 

	return emOpen;
}

// 关闭文件
int close_file(const FILE_DESC fd)
{
	// 检查文件描述符是否有效
	if (INVALID_FILE_DESC == fd)
	{
		// 无效的文件描述符
		return -11;
	}

	// 检查文件是否打开
	if (NULL == g_openTable[fd])
	{
		// 文件未打开
		return -1;
	}

	// 清理被关闭的文件打开表项
	g_openTable[fd] = NULL;

	return 0;
}

// 写文件
int write_file(const FILE_DESC fd, const char buf[], const size_t len)
{
	// 检查文件描述符是否有效
	if (INVALID_FILE_DESC == fd)
	{
		// 无效的文件描述符
		return -11;
	}

	// 检查文件是否打开
	if (NULL == g_openTable[fd])
	{
		// 文件未打开
		return -1;
	}

	// 读取用户打开表对应表项的首个磁盘块号
	int writeDisk = g_openTable[fd]->firstDisk;

	// 遍历到文件的最后一块磁盘
	for (; g_FAT[writeDisk].nextDisk != -1; writeDisk = g_FAT[writeDisk].nextDisk);

	const char *pCurBuf = buf;
	size_t curLen = len;

	int emDisk = 0;

	// 循环申请盘块并向其中写入内容直到缓冲区全部被写入
	while (1)
	{
		// 向空闲磁盘中写数据
		size_t writeLen = write_disk(
			writeDisk, 
			g_openTable[fd]->size % DISK_SIZE, 
			buf, 
			curLen
		);

		// 计算剩余缓冲区的指针和长度
		curLen -= writeLen;
		pCurBuf += writeLen;

		// 检查缓冲区是否被全部写入
		if (0 == curLen)
		{
			break;
		}

		// 申请空闲盘块
		emDisk = alloc_disk(emDisk);
		if (INVALID_DISK == emDisk)
		{
			break;
		}

		// 修改FAT，使空闲盘块链接到上面被写满的盘块的尾后
		g_FAT[writeDisk].nextDisk = emDisk;
		
		writeDisk = emDisk;
	}

	g_FAT[writeDisk].nextDisk = INVALID_DISK;
	

	// 修改长度
	g_openTable[fd]->size = len - curLen;

	return (INVALID_DISK == emDisk)?(-1):(0);
}

//读文件
int read_file(const FILE_DESC fd, char **ppBuf, size_t *pBufSize)
{
	// 检查文件描述符是否有效
	if (INVALID_FILE_DESC == fd)
	{
		// 无效的文件描述符
		return -11;
	}

	// 检查文件是否打开
	if (NULL == g_openTable[fd])
	{
		// 文件未打开
		return -1;
	}
	

	// 读取用户打开表对应表项的首个磁盘块号
	int readDisk = g_openTable[fd]->firstDisk;

	size_t curLen = g_openTable[fd]->size;

	*ppBuf = calloc(curLen, sizeof(char));
	if (!*ppBuf)
	{
		exit(-2);
	}
	
	char *pBuf = *ppBuf;

	while (1)
	{
		size_t readLen = read_disk(readDisk, 0, pBuf, curLen);
		
		curLen -= readLen;
		pBuf += readLen;

		if (0 == curLen)
		{
			break;
		}

		readDisk = g_FAT[readDisk].nextDisk;

		if (INVALID_DISK == readDisk)
		{
			break;
		}
	}

	*pBufSize = pBuf - *ppBuf;

	return 0;
}

void init_file_sys()
{
	// 初始化用于模拟磁盘的内存
	init_virtual_disk_mem();

	// 初始化FAT表
	init_FAT();

	// 初始化打开表
	init_open_table();

	// 初始化当前MFD和当前UFD
	init_MFD();
	init_UFD();

	// 安装MFD（安装文件系统）
	install_MFD();
}

void uninit_file_sys()
{
	// 反初始化当前MFD和UFD
	uninit_UFD();
	uninit_MFD();

	// 反初始化打开表
	uninit_open_table();

	// 反初始化FAT表
	uninit_FAT();

	// 反初始化用于模拟磁盘的内存
	uninit_virtual_disk_mem();
}

#define CMD_SIZE_MAX 256

#define PARAM_NUM_MAX 3

typedef struct stCmdProcPair
{
	const char szCmd[CMD_SIZE_MAX];
	void (*fnProc)(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt);
} CmdProcPair;

// 用户登录逻辑
void proc_login(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 2)
	{
		printf("]命令格式错误\n");
		return;
	}

	if (g_szUser)
	{
		printf("%s]你已经登陆了\n", g_szUser);
		return;
	}

	g_szUser = calloc(NAME_MAX, sizeof(char));
	if (!g_szUser)
	{
		exit(-2);
	}
	strncpy(g_szUser, szParam[1], NAME_MAX);

	int res = load_UFD();
	if (0 != res)
	{
		printf("]登录失败\n");
#ifdef _DEBUG
		printf("res:%d\n", res);
#endif
		return;
	}

	printf("%s]登录成功\n", g_szUser);
}

// 用户注销逻辑
void proc_logout(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	free(g_szUser);
	g_szUser = NULL;

	printf("]注销成功\n");
}

// 新建文件逻辑
void proc_create(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 2)
	{
		printf("]命令格式错误");
		return;
	}

	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	int res = create_file(szParam[1]);
	if(0 != res)
	{
		printf("%s]新建文件失败\n", g_szUser);
#ifdef _DEBUG
		printf("res:%d\n", res);
#endif
		return;
	}


	printf("%s]新建文件成功\n", g_szUser);
}

// 删除文件逻辑
void proc_delete(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{

	if (paramCnt < 2)
	{
		printf("]命令格式错误");
		return;
	}

	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	int res = del_file(szParam[1]);
	if (0 != res)
	{
		printf("%s]删除文件失败\n", g_szUser);
#ifdef _DEBUG
		printf("res:%d\n", res);
#endif
		return;
	}

	printf("%s]删除文件成功\n", g_szUser);
}

// 打开文件逻辑
void proc_open(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{

	if (paramCnt < 2)
	{
		printf("命令格式错误");
		return;
	}

	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	FILE_DESC fd = open_file(szParam[1]);
	if (INVALID_FILE_DESC == fd)
	{
		printf("%s]打开文件失败\n", g_szUser);
		return;
	}

	printf("%s]打开文件成功，文件描述符为%zu\n", g_szUser, fd);
}


// 关闭文件逻辑
void proc_close(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{

	if (paramCnt < 2)
	{
		printf("]命令格式错误");
		return;
	}

	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	FILE_DESC fd = atoi(szParam[1]);
	if (0 != close_file(fd))
	{
		printf("%s]关闭文件失败\n", g_szUser);
		return;
	}

	printf("%s]关闭文件成功\n", g_szUser);
}

// 读文件逻辑
void proc_read(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 2)
	{
		printf("]命令格式错误");
		return;
	}


	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	char *pBuf = NULL;
	size_t bufSize = 0;

	FILE_DESC fd = atoi(szParam[1]);
	if (0 != read_file(fd, &pBuf, &bufSize))
	{
		printf("%s]读取文件失败\n", g_szUser);
		return;
	}

	printf("%s]读取文件成功\n", g_szUser);

	for (size_t i = 0; i < bufSize; i++)
	{
		fputc(pBuf[i], stdout);
	}

	fputc('\n', stdout);
}

// 写文件逻辑
void proc_write(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 3)
	{
		printf("]命令格式错误");
		return;
	}


	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	FILE_DESC fd = atoi(szParam[1]);
	if (0 != write_file(fd, szParam[2], strlen(szParam[2])))
	{
		printf("%s]写文件失败\n", g_szUser);
		return;
	}

	printf("%s]写文件成功\n", g_szUser);
}

// 列目录逻辑
void proc_dir(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (!g_szUser)
	{
		printf("]你还没有登录\n");
		return;
	}

	printf("%s\n+ |%-10s|%10s|%10s|\n", g_szUser, "文件名", "物理位置", "长度");
	for (size_t FCBIndex = 0; FCBIndex < UFD_SIZE; FCBIndex++)
	{
#ifndef _DEBUG
		if (g_UFD[FCBIndex].firstDisk == -1)
		{
			continue;
		}
#endif
		printf("+-|%-10s|%10d|%10zu|\n", g_UFD[FCBIndex].name, g_UFD[FCBIndex].firstDisk, g_UFD[FCBIndex].size);
	}
}

void proc_main()
{
	const CmdProcPair cmdProcTable[] = {
		{"login", proc_login},
		{"logout", proc_logout},
		{"create", proc_create},
		{"delete", proc_delete},
		{"open", proc_open},
		{"close", proc_close},
		{"read", proc_read},
		{"write", proc_write},
		{"dir", proc_dir}
	};

	const cmdProcTableSize = sizeof(cmdProcTable) / sizeof(cmdProcTable[0]);

	char cmdLine[CMD_SIZE_MAX];

	while (1)
	{
		printf(">");
		fgets(cmdLine, CMD_SIZE_MAX, stdin);
		const size_t cmdLineLen = strlen(cmdLine);
		cmdLine[cmdLineLen - 1] = '\0';

#ifdef _DEBUG
		printf("%s\n", cmdLine);
#endif // 

		const char* szParam[PARAM_NUM_MAX];

		size_t paramCnt = 0;
		szParam[paramCnt++] = cmdLine;

		for (size_t i = 1; i < cmdLineLen; i++)
		{
			if (cmdLine[i - 1] == ' ')
			{
				cmdLine[i - 1] = '\0';
				szParam[paramCnt++] = cmdLine + i;

				if (PARAM_NUM_MAX == paramCnt)
				{
					break;
				}
			}
		}

#ifdef _DEBUG
		printf("paramCnt:%zu\n", paramCnt);
		for (size_t i = 0; i < paramCnt; i++)
		{
			printf("%s\n", szParam[i]);
		}
#endif // DEBUG


		size_t pairIndex = 0;
		for (; pairIndex < cmdProcTableSize; pairIndex++)
		{
			if (0 == strcmp(cmdProcTable[pairIndex].szCmd, szParam[0]))
			{
				break;
			}
		}

		if (cmdProcTableSize == pairIndex)
		{
			printf("]输入的命令错误\n");
			continue;
		}

		cmdProcTable[pairIndex].fnProc(szParam, paramCnt);
	}

	uninit_file_sys();
}

int main(int argc, char **argv)
{
	// 初始化文件系统
	init_file_sys();

	// 逻辑主体
	proc_main();

	// 反初始化文件系统
	uninit_file_sys();
	return 0;
}


/*

// 模拟磁盘所需的内存大小
#define DISK_MEM_SIZE (DISK_NUM * DISK_SIZE)

// FAT的大小
#define FAT_MEM_SIZE (DISK_NUM * sizeof(FATItem))

// 根目录起始盘块号
#define ROOT_DISK ((FAT_MEM_SIZE / DISK_SIZE) + 1)
// 文件打开表的打开文件数目
size_t g_openTableSize = 0;

// 目录项
typedef struct stDirectory
{
	// 目录项内部实现
	FCB dirItem[MSD + 2];
} Directory;

// 当前目录
Directory *g_pCurDir = NULL;

// 文件打开表项
typedef struct stOpenItem
{
	// 文件名
	char name[NAME_MAX];

	// 文件大小
	int size;

	// 文件起始盘块号
	int firstDisk
} OpenItem, *OpenTable;

// 清空文件打开表项
void clear_open_item(OpenItem* open)
{
	open->firstDisk = -1;
}

// 填写文件打开表项
void write_open_item_by_FCB(OpenItem *open, const FCB *fcb)
{
	strcpy(open->name, fcb->name);
	open->size = fcb->size;
	open->firstDisk = fcb->firstDisk;
}

// 清空文件打开表项
void clear_open_item(OpenItem *open)
{
	*open = NULL;
}

// 填写文件打开表项
void write_open_item(OpenItem* open, FCB *fcb)
{
	*open = fcb;
}

// 打开文件
size_t open_file(const char name[])
{
	// 检查要打开的文件是否存在
	size_t FCBIndex = is_file_exist(name);
	if (MSD + 2 == FCBIndex)
	{
		return 0;
	}

	// 检查文件是否为目录
	if ('1' == g_pCurDir->dirItem[FCBIndex].prop)
	{
		return 0;
	}

	// 检查文件是否打开
	if (MOFN != is_file_open(name))
	{
		// 文件已经打开
		return 0;
	}

	// 检查打开文件的数量是否达到上限
	if (MOFN == g_openTableSize)
	{
		return 0;
	}

	// 查找一个空闲打开表项
	size_t emOpenIndex;
	for (emOpenIndex = 1; emOpenIndex < MOFN; emOpenIndex++)
	{
		if (NULL == g_openTable[emOpenIndex].firstDisk)
		{
			break;
		}
	}

	// 填写打开表项的相关信息
	write_open_item(g_openTable + emOpenIndex, g_pCurDir->dirItem + FCBIndex);

	// 递增打开文件表项数目
	g_openTableSize++;

	return emOpenIndex;
}

// 关闭文件
int close_file(const char name[])
{
	// 检查文件是否打开
	size_t openIndex = is_file_open(name);

	if (MOFN == openIndex)
	{
		// 文件未打开
		return -1;
	}

	// 清理被关闭的文件打开表项
	clear_open_item(g_openTable + openIndex);

	// 递减文件打开表项数目
	g_openTableSize--;

	return 0;
}

// 检查文件是否存在
size_t is_file_exist(const char name[])
{

	size_t i = 2;
	for (; i < MSD + 2; i++)
	{
		if (0 == strcmp(g_pCurDir->dirItem[i].name, name))
		{
			break;
		}
	}

	return i;
}

// 检查目录是否存在
size_t is_dir_exist(const char name[])
{
	size_t dir = 0;
	for (; dir < MFD_SIZE; dir++)
	{
		if (0 == strcmp(g_MFD[dir].name, name))
		{
			break;
		}
	}

	return dir;
}
*/
