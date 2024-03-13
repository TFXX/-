#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// �ļ������
#define NAME_MAX 32

// �û��ļ�Ŀ¼��С
#ifdef _DEBUG
#define MSD 3
#else
#define MSD 16
#endif

// ���ļ�Ŀ¼��С
#define MFD_SIZE MSD

#define MFD_MEM_SIZE (sizeof(FCB) * MFD_SIZE)

// �û��ļ�Ŀ¼��С
#define UFD_SIZE MSD

#define UFD_MEM_SIZE (sizeof(FCB) * UFD_SIZE)

// ���̿��С(byte)
#define DISK_SIZE 1024

// ���̿���Ŀ
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

// Fat������FAT��
typedef struct stFATItem
{
	// ָ����һ�����̵�ָ��
	int nextDisk;

	// ���̿��Ƿ���б�־λ
	_Bool emFlag;
} FATItem, * FAT;

// FAT��
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

// ���������ʼ��ַ
void* g_pDiskMem = NULL;

void static init_virtual_disk_mem()
{
	g_pDiskMem = calloc(DISK_NUM, DISK_SIZE);
	if (!g_pDiskMem)
	{
		exit(-2);
	}
#ifdef _DEBUG
	printf("������̴����ɹ�\n");
#endif 
}

void static uninit_virtual_disk_mem()
{
	if (g_pDiskMem)
	{
		free(g_pDiskMem);
	}
}

// ��������̿�
int alloc_disk(int startDisk)
{
	// �ҵ������̿��
	for (; startDisk < DISK_NUM; startDisk++)
	{
		if (1 == g_FAT[startDisk].emFlag)
		{
			break;
		}
	}

	// û�п����̿�
	if (DISK_NUM == startDisk)
	{
		return -1;
	}

	// ��FAT��Ӧ������޸�
	g_FAT[startDisk].emFlag = 0;
	g_FAT[startDisk].nextDisk = -1;

	return startDisk;
}

// �����ͷ��̿�
void free_disk(const int firstDisk)
{
	for (int disk = firstDisk; disk != -1; disk = g_FAT[disk].nextDisk)
	{
		// ��FAT��Ӧ������޸�
		g_FAT[firstDisk].emFlag = 1;
	}
}

// �ļ����ƿ�
typedef struct stFCB
{
	// �ļ�/Ŀ¼��
	char name[NAME_MAX];

	// ���ԣ�1ΪĿ¼ 0Ϊ��ͨ�ļ�
	//_Bool dirFlag;

	// �ļ���С
	size_t size;

	// �ļ�/Ŀ¼��ʼ�̿��
	int firstDisk;
} FCB;

// ���̿���д������
size_t write_disk(const int disk, const size_t start, const void *buf, size_t len)
{
	if (len > DISK_SIZE - start)
	{
		len = DISK_SIZE - start;
	}

	memcpy((char *)g_pDiskMem + disk * DISK_SIZE + start, buf, len);

	// ���سɹ�д��ĳ���
	return len;
}

// ���̿��ж�ȡ����
size_t read_disk(const int disk, const size_t start, void *buf, size_t len)
{
	if (len > DISK_SIZE - start)
	{
		len = DISK_SIZE - start;
	}

	memcpy(buf, (char *)g_pDiskMem + disk * DISK_SIZE + start, len);

	// ���سɹ���ȡ�ĳ���
	return len;
}

// ����һ������FCB
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

// ����FCB
void free_FCB(FCB *pFCB)
{
	pFCB->firstDisk = -1;
}

// ��дFCB
void write_FCB(FCB *pFCB, const char name[], const char prop, const int size, const int firstDisk)
{
	strcpy(pFCB->name, name);
	//fcb->prop = prop;
	pFCB->size = size;
	pFCB->firstDisk = firstDisk;
}

// ��ǰ�û�
char* g_szUser = NULL;

// �ļ�Ŀ¼
typedef FCB* FileDir;

// ����ļ�/UFD�Ƿ�����
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

// ���ļ�Ŀ¼
FileDir g_MFD = NULL;

// ��ǰ�û��ļ�Ŀ¼
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
	//MFD�����޷���һ���̿�洢
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
		printf("install_MFD����%d\n", res);
	}
#endif
}

// ����UFD
int create_UFD(const char name[])
{
	// ����ļ�������
	if (strlen(name) > NAME_MAX)
	{
		// �ļ�������
		return -1;
	}

	// ���MFD�Ƿ��п���FCB
	size_t UFDFCBIndex = alloc_FCB(g_MFD, MFD_SIZE);
	if (MFD_SIZE == UFDFCBIndex)
	{
		// û�п���FCB
		return -2;
	}

	// ���Ŀ¼�Ƿ�����
	if (MFD_SIZE != is_name_exist(name, g_MFD, MFD_SIZE))
	{
		// Ŀ¼����
		return -3;
	}

	// �����̿����ڴ洢UFD
	g_MFD[UFDFCBIndex].firstDisk = alloc_disk(0);
	if (-1 == g_MFD[UFDFCBIndex].firstDisk)
	{
		// ���̿ռ䲻��
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

	// ����MFD
	store_MFD();

	return 0;
}

int load_UFD()
{
	size_t i = is_name_exist(g_szUser, g_MFD, MFD_SIZE);

	if (MFD_SIZE == i)
	{
		// UFD������
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
		// UFD������
		return (-15);
	}
	return UFD_MEM_SIZE == write_disk(g_MFD[i].firstDisk, 0, g_UFD, UFD_MEM_SIZE) ? (0) : (-17);
}

// �ļ�������
typedef size_t FILE_DESC;

// �ļ��򿪱���
typedef FCB* OpenItem, ** OpenTable;

// �ļ��򿪱�
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

// ����ļ��Ƿ��
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

// �����ļ�
int create_file(const char name[])
{
	if (strlen(name) > NAME_MAX)
	{
		return -1;
	}

	// ���Ҫ�������ļ��Ƿ���Ѿ����ڵ��ļ�����
	if(UFD_SIZE != is_name_exist(name, g_UFD, UFD_SIZE))
	{
		return -4;
	}

	size_t emFCBIndex = alloc_FCB(g_UFD, UFD_SIZE);
	if (UFD_SIZE == emFCBIndex)
	{
		return -2;
	}

	// ��������̿�
	int emDiskIndex = alloc_disk(0);
	if (DISK_NUM == emDiskIndex)
	{
		return -5;
	}

	// ��дĿ¼��
	write_FCB(g_UFD + emFCBIndex, name, '0', 0, emDiskIndex);

	// ����UFD
	store_UFD();

	return 0;
}

// ɾ���ļ�
int del_file(const char name[])
{
	// ��鱻ɾ�����ļ��Ƿ���Ŀ¼��
	size_t FCBIndex = is_name_exist(name, g_UFD, UFD_SIZE);
	if (UFD_SIZE == FCBIndex)
	{
		// �ļ����ڵ�ǰĿ¼��
		return -1;
	}

	// ��鱻ɾ�����ļ��Ƿ��
	if (INVALID_FILE_DESC != is_file_open(g_UFD + FCBIndex))
	{
		// ����ļ�������ɾ��
		return -2;
	}

	// �ͷŴ����ϵĶ�Ӧ�ռ�
	free_disk(g_UFD[FCBIndex].firstDisk);

	// �ͷ�Ŀ¼��
	free_FCB(g_UFD + FCBIndex);

	// ����UFD
	store_UFD();

	return 0;
}

// ���ļ�
FILE_DESC open_file(const char name[])
{
	// ���Ҫ�򿪵��ļ��Ƿ����
	size_t FCBIndex = is_name_exist(name, g_UFD, UFD_SIZE);
	if (UFD_SIZE == FCBIndex)
	{
		return 0;
	}

	// ����ļ��Ƿ��
	if (INVALID_FILE_DESC != is_file_open(g_UFD + FCBIndex))
	{
		// �ļ��Ѿ���
		return INVALID_FILE_DESC;
	}

	// ����һ�����д򿪱���
	FILE_DESC emOpen;
	for (emOpen = OPEN_TABLE_SIZE - 1; emOpen > 0; emOpen--)
	{
		if (NULL == g_openTable[emOpen])
		{
			break;
		}
	}

	// �����ļ��������Ƿ�ﵽ����
	if (INVALID_FILE_DESC == emOpen)
	{
		// �򿪵��ļ������ﵽ����
		return -3;
	}

	// ��д�򿪱���������Ϣ
	g_openTable[emOpen] = g_UFD + FCBIndex; 

	return emOpen;
}

// �ر��ļ�
int close_file(const FILE_DESC fd)
{
	// ����ļ��������Ƿ���Ч
	if (INVALID_FILE_DESC == fd)
	{
		// ��Ч���ļ�������
		return -11;
	}

	// ����ļ��Ƿ��
	if (NULL == g_openTable[fd])
	{
		// �ļ�δ��
		return -1;
	}

	// �����رյ��ļ��򿪱���
	g_openTable[fd] = NULL;

	return 0;
}

// д�ļ�
int write_file(const FILE_DESC fd, const char buf[], const size_t len)
{
	// ����ļ��������Ƿ���Ч
	if (INVALID_FILE_DESC == fd)
	{
		// ��Ч���ļ�������
		return -11;
	}

	// ����ļ��Ƿ��
	if (NULL == g_openTable[fd])
	{
		// �ļ�δ��
		return -1;
	}

	// ��ȡ�û��򿪱��Ӧ������׸����̿��
	int writeDisk = g_openTable[fd]->firstDisk;

	// �������ļ������һ�����
	for (; g_FAT[writeDisk].nextDisk != -1; writeDisk = g_FAT[writeDisk].nextDisk);

	const char *pCurBuf = buf;
	size_t curLen = len;

	int emDisk = 0;

	// ѭ�������̿鲢������д������ֱ��������ȫ����д��
	while (1)
	{
		// ����д�����д����
		size_t writeLen = write_disk(writeDisk, g_openTable[fd]->size % DISK_SIZE, buf, curLen);

		// ����ʣ�໺������ָ��ͳ���
		curLen -= writeLen;
		pCurBuf += writeLen;

		// ��黺�����Ƿ�ȫ��д��
		if (0 == curLen)
		{
			break;
		}

		// ��������̿�
		emDisk = alloc_disk(emDisk);
		if (INVALID_DISK == emDisk)
		{
			break;
		}

		// �޸�FAT��ʹ�����̿����ӵ����汻д�����̿��β��
		g_FAT[writeDisk].nextDisk = emDisk;
		
		writeDisk = emDisk;
	}

	g_FAT[writeDisk].nextDisk = INVALID_DISK;
	

	// �޸ĳ���
	g_openTable[fd]->size = len - curLen;

	return (INVALID_DISK == emDisk)?(-1):(0);
}

//���ļ�
int read_file(const FILE_DESC fd, char **ppBuf, size_t *pBufSize)
{
	// ����ļ��������Ƿ���Ч
	if (INVALID_FILE_DESC == fd)
	{
		// ��Ч���ļ�������
		return -11;
	}

	// ����ļ��Ƿ��
	if (NULL == g_openTable[fd])
	{
		// �ļ�δ��
		return -1;
	}
	

	// ��ȡ�û��򿪱��Ӧ������׸����̿��
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
	// ��ʼ������ģ����̵��ڴ�
	init_virtual_disk_mem();

	// ��ʼ��FAT��
	init_FAT();

	// ��ʼ���򿪱�
	init_open_table();

	// ��ʼ����ǰMFD�͵�ǰUFD
	init_MFD();
	init_UFD();

	// ��װMFD����װ�ļ�ϵͳ��
	install_MFD();
}

void uninit_file_sys()
{
	// ����ʼ����ǰMFD��UFD
	uninit_UFD();
	uninit_MFD();

	// ����ʼ���򿪱�
	uninit_open_table();

	// ����ʼ��FAT��
	uninit_FAT();

	// ����ʼ������ģ����̵��ڴ�
	uninit_virtual_disk_mem();
}

#define CMD_SIZE_MAX 256

#define PARAM_NUM_MAX 3

typedef struct stCmdProcPair
{
	const char szCmd[CMD_SIZE_MAX];
	void (*fnProc)(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt);
} CmdProcPair;

// �û���¼�߼�
void proc_login(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 2)
	{
		printf("]�����ʽ����\n");
		return;
	}

	if (g_szUser)
	{
		printf("%s]���Ѿ���½��\n", g_szUser);
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
		printf("]��¼ʧ��\n");
#ifdef _DEBUG
		printf("res:%d\n", res);
#endif
		return;
	}

	printf("%s]��¼�ɹ�\n", g_szUser);
}

// �û�ע���߼�
void proc_logout(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	free(g_szUser);
	g_szUser = NULL;

	printf("]ע���ɹ�\n");
}

// �½��ļ��߼�
void proc_create(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 2)
	{
		printf("]�����ʽ����");
		return;
	}

	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	int res = create_file(szParam[1]);
	if(0 != res)
	{
		printf("%s]�½��ļ�ʧ��\n", g_szUser);
#ifdef _DEBUG
		printf("res:%d\n", res);
#endif
		return;
	}


	printf("%s]�½��ļ��ɹ�\n", g_szUser);
}

// ɾ���ļ��߼�
void proc_delete(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{

	if (paramCnt < 2)
	{
		printf("]�����ʽ����");
		return;
	}

	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	int res = del_file(szParam[1]);
	if (0 != res)
	{
		printf("%s]ɾ���ļ�ʧ��\n", g_szUser);
#ifdef _DEBUG
		printf("res:%d\n", res);
#endif
		return;
	}

	printf("%s]ɾ���ļ��ɹ�\n", g_szUser);
}

// ���ļ��߼�
void proc_open(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{

	if (paramCnt < 2)
	{
		printf("�����ʽ����");
		return;
	}

	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	FILE_DESC fd = open_file(szParam[1]);
	if (INVALID_FILE_DESC == fd)
	{
		printf("%s]���ļ�ʧ��\n", g_szUser);
		return;
	}

	printf("%s]���ļ��ɹ����ļ�������Ϊ%zu\n", g_szUser, fd);
}


// �ر��ļ��߼�
void proc_close(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{

	if (paramCnt < 2)
	{
		printf("]�����ʽ����");
		return;
	}

	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	FILE_DESC fd = atoi(szParam[1]);
	if (0 != close_file(fd))
	{
		printf("%s]�ر��ļ�ʧ��\n", g_szUser);
		return;
	}

	printf("%s]�ر��ļ��ɹ�\n", g_szUser);
}

// ���ļ��߼�
void proc_read(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 2)
	{
		printf("]�����ʽ����");
		return;
	}


	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	char *pBuf = NULL;
	size_t bufSize = 0;

	FILE_DESC fd = atoi(szParam[1]);
	if (0 != read_file(fd, &pBuf, &bufSize))
	{
		printf("%s]��ȡ�ļ�ʧ��\n", g_szUser);
		return;
	}

	printf("%s]��ȡ�ļ��ɹ�\n", g_szUser);

	for (size_t i = 0; i < bufSize; i++)
	{
		fputc(pBuf[i], stdout);
	}

	fputc('\n', stdout);
}

// д�ļ��߼�
void proc_write(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (paramCnt < 3)
	{
		printf("]�����ʽ����");
		return;
	}


	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	FILE_DESC fd = atoi(szParam[1]);
	if (0 != write_file(fd, szParam[2], strlen(szParam[2])))
	{
		printf("%s]д�ļ�ʧ��\n", g_szUser);
		return;
	}

	printf("%s]д�ļ��ɹ�\n", g_szUser);
}

// ��Ŀ¼�߼�
void proc_dir(const char *szParam[PARAM_NUM_MAX], const size_t paramCnt)
{
	if (!g_szUser)
	{
		printf("]�㻹û�е�¼\n");
		return;
	}

	printf("%s\n+ |%-10s|%10s|%10s|\n", g_szUser, "�ļ���", "����λ��", "����");
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
			printf("]������������\n");
			continue;
		}

		cmdProcTable[pairIndex].fnProc(szParam, paramCnt);
	}

	uninit_file_sys();
}

int main(int argc, char **argv)
{
	// ��ʼ���ļ�ϵͳ
	init_file_sys();

	// �߼�����
	proc_main();

	// ����ʼ���ļ�ϵͳ
	uninit_file_sys();
	return 0;
}


/*

// ģ�����������ڴ��С
#define DISK_MEM_SIZE (DISK_NUM * DISK_SIZE)

// FAT�Ĵ�С
#define FAT_MEM_SIZE (DISK_NUM * sizeof(FATItem))

// ��Ŀ¼��ʼ�̿��
#define ROOT_DISK ((FAT_MEM_SIZE / DISK_SIZE) + 1)
// �ļ��򿪱�Ĵ��ļ���Ŀ
size_t g_openTableSize = 0;

// Ŀ¼��
typedef struct stDirectory
{
	// Ŀ¼���ڲ�ʵ��
	FCB dirItem[MSD + 2];
} Directory;

// ��ǰĿ¼
Directory *g_pCurDir = NULL;

// �ļ��򿪱���
typedef struct stOpenItem
{
	// �ļ���
	char name[NAME_MAX];

	// �ļ���С
	int size;

	// �ļ���ʼ�̿��
	int firstDisk
} OpenItem, *OpenTable;

// ����ļ��򿪱���
void clear_open_item(OpenItem* open)
{
	open->firstDisk = -1;
}

// ��д�ļ��򿪱���
void write_open_item_by_FCB(OpenItem *open, const FCB *fcb)
{
	strcpy(open->name, fcb->name);
	open->size = fcb->size;
	open->firstDisk = fcb->firstDisk;
}

// ����ļ��򿪱���
void clear_open_item(OpenItem *open)
{
	*open = NULL;
}

// ��д�ļ��򿪱���
void write_open_item(OpenItem* open, FCB *fcb)
{
	*open = fcb;
}

// ���ļ�
size_t open_file(const char name[])
{
	// ���Ҫ�򿪵��ļ��Ƿ����
	size_t FCBIndex = is_file_exist(name);
	if (MSD + 2 == FCBIndex)
	{
		return 0;
	}

	// ����ļ��Ƿ�ΪĿ¼
	if ('1' == g_pCurDir->dirItem[FCBIndex].prop)
	{
		return 0;
	}

	// ����ļ��Ƿ��
	if (MOFN != is_file_open(name))
	{
		// �ļ��Ѿ���
		return 0;
	}

	// �����ļ��������Ƿ�ﵽ����
	if (MOFN == g_openTableSize)
	{
		return 0;
	}

	// ����һ�����д򿪱���
	size_t emOpenIndex;
	for (emOpenIndex = 1; emOpenIndex < MOFN; emOpenIndex++)
	{
		if (NULL == g_openTable[emOpenIndex].firstDisk)
		{
			break;
		}
	}

	// ��д�򿪱���������Ϣ
	write_open_item(g_openTable + emOpenIndex, g_pCurDir->dirItem + FCBIndex);

	// �������ļ�������Ŀ
	g_openTableSize++;

	return emOpenIndex;
}

// �ر��ļ�
int close_file(const char name[])
{
	// ����ļ��Ƿ��
	size_t openIndex = is_file_open(name);

	if (MOFN == openIndex)
	{
		// �ļ�δ��
		return -1;
	}

	// �����رյ��ļ��򿪱���
	clear_open_item(g_openTable + openIndex);

	// �ݼ��ļ��򿪱�����Ŀ
	g_openTableSize--;

	return 0;
}

// ����ļ��Ƿ����
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

// ���Ŀ¼�Ƿ����
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
