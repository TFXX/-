#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INS_NUM 320

// һҳ���洢10��ָ��
#define INS_PER_PAGE 10

// �û��ڴ���СΪ4K
#define MIN_USER_MEM_SIZE 4

// �û��ڴ����Ϊ32K
#define MAX_USER_MEM_SIZE 32

// ģ��ָ��ṹ
typedef struct stInstruction {
    // ָ����߼���ַ
    size_t address;
    
    // ָ���������ڴ��е�ҳ��λ��
    int page;
} Instruction;


// ģ��洢���е�ҳ��ṹ
typedef struct stPage
{
    // ҳ��
    int page;

    // ���ڱȽ�����ҳ��֮�����һ�α�ʹ�õ��Ⱥ�˳��
    int  useOrder;
} Page;

// ����ָ������
void generate_instruction(Instruction ins[])
{
    // ���ѡȡһ�����

    // ��m��[0, 319]���������
    size_t m = rand() % INS_NUM + 0;

    // ͨ�����������һ��ָ������
    for (size_t i = 0; i < INS_NUM; i++)
    {
        // ������һ��ָ�����е�λ��
        switch (i % 4)
        {
        case 0:
        case 2:
            // 50%�ĸ�����ָ��˳��ִ��
            // ͬʱȷ��m����Խ�磬��ָ�����еĵ�ַ�ռ���β����
            m = (m + 1) % INS_NUM;
            break;
        case 1:
            // ��m��ǰ��ַ[0, m]���������
            m = rand() % m + 0;
            break;
        case 3:
            // ��m�ں��ַ[m+1, 319]���������
            m = rand() % (319 - (m + 1)) + (m + 1);
            break;
        }

        ins[i].address = m;

        // ����ָ�����ڵ�ҳ��
        ins[i].page = (int)(ins[i].address / INS_PER_PAGE);
    }
}

// FIFOҳ���û��㷨
void FIFO_page_replace(
    const size_t userMemSize,
    const Instruction ins[],
    unsigned int *pCnt)
{
    // ģ���û��ڴ�ռ�
    // �����һ��ҳ���鹹�ģ��䵱�ڱ�
    Page *userMem = (Page *)calloc(userMemSize + 1, sizeof(Page));

    // �ڱ��������Ż��������ж�ָ�����Ƿ����û��ڴ��еıȽ��߼�
    const size_t sentinel = userMemSize;

    // �����ۼ�ҳ��ʧЧ����
    *pCnt = 0;

    // ��ҳ��ʱЧӦ��������û��ڴ�ҳ��
    size_t nextPage = 0;

    // ��ʼʱû��ҳ�������û��ڴ�
    for (size_t i = 0; i < userMemSize; i++)
    {
        userMem[i].page = -1;
    }

    // ģ��ҳ��ַ������
    for (size_t i = 0; i < INS_NUM; i++)
    {
        // �����ڱ�
        userMem[sentinel].page = ins[i].page;

        size_t j = 0;

        // ���ҳ���Ƿ����ڴ���
        for (; j <= sentinel; j++)
        {
            if (ins[i].page == userMem[j].page)
            {
                break;
            }
        }

        // û���ҵ�ҳ��
        if (sentinel == j)
        {
            // �ۼ�ҳ��ʧЧ����
            (*pCnt)++;

            // �����ҳ
            userMem[nextPage].page = ins[i].page;
            nextPage = (nextPage + 1) % userMemSize;
        }
    }

    free(userMem);
}

// LRUҳ���û��㷨
void LRU_page_replace(const size_t userMemSize, const Instruction ins[], unsigned int *pCnt)
{
    // ģ���û��ڴ�ռ�
    // �����һ��ҳ���鹹�ģ��䵱�ڱ�
    Page *userMem = (Page *)calloc(userMemSize + 1, sizeof(Page));

    // �ڱ��������Ż��������ж�ָ�����Ƿ����û��ڴ��еıȽ��߼�
    const size_t sentinel = userMemSize;

    // �����ۼ�ҳ��ʧЧ����
    *pCnt = 0;

    // ��ʼʱû��ҳ�������û��ڴ�
    for (size_t i = 0; i < userMemSize; i++)
    {
        userMem[i].page = -1;
        userMem[i].useOrder = -1;
    }

    // ģ��ҳ��ַ������
    for (size_t i = 0; i < INS_NUM; i++)
    {
        // �����ڱ�
        userMem[sentinel].page = ins[i].page;

        size_t j = 0;

        // ���ҳ���Ƿ����ڴ���
        for (; j <= sentinel; j++)
        {
            if (ins[i].page == userMem[j].page)
            {
                break;
            }
        }

        // û���ҵ�ҳ��
        if (sentinel == j)
        {
            // �ۼ�ҳ��ʧЧ����
            (*pCnt)++;

            j = 0;

            // �������û�б�ʹ�õ�ҳ��
            for (size_t k = j + 1; k < userMemSize; k++)
            {
                if (userMem[j].useOrder > userMem[k].useOrder)
                {
                    j = k;
                }
            }

            // �����ҳ
            userMem[j].page = ins[i].page;
        }

        // ��ʱjΪָ���Ӧҳ��Ҳ�û��ڴ��е��������

        // ����ҳ��һ�α�Ѱַ��ʱ��
        userMem[j].useOrder = (int)i;
    }

    free(userMem);
}

// ����������
inline float calculate_hit_rate(const unsigned int faultCnt, const size_t addrStmLength)
{
    return (float)(addrStmLength - faultCnt) / addrStmLength;
}

int main(int argc, char **argv)
{
    srand((unsigned int)time(NULL));

    // ����ָ������
    Instruction *ins = (Instruction *)calloc(INS_NUM, sizeof(Instruction));
    generate_instruction(ins);

    for (size_t userMemSize = MIN_USER_MEM_SIZE; userMemSize <= MAX_USER_MEM_SIZE; userMemSize++)
    {
        // �ڴ�ʧЧ����
        unsigned int FIFOFault, LRUFault;

        // ģ��FIFO��LRU��ҳ
        FIFO_page_replace(userMemSize, ins, &FIFOFault);
        LRU_page_replace(userMemSize, ins, &LRUFault);

        float FIFO_hit_rate = calculate_hit_rate(FIFOFault, INS_NUM);
        float LRU_hit_rate = calculate_hit_rate(LRUFault, INS_NUM);

        printf(
            "]�ڴ�����:%2zuK\tFIFO������:%0.2f\tLRU������:%0.2f\n",
            userMemSize,
            FIFO_hit_rate,
            LRU_hit_rate
        );
    }

    free(ins);
    return 0;
}

/*
// ָ��ṹ��
typedef struct {
    int address;
    int page;
} Instruction;

// ҳ��ṹ��
typedef struct {
    int page_num;
    int last_used;
} Page;

// �������ָ������
void generateInstructions(Instruction* instructions) {
    int i, m, m_prime;
    int m_start = rand() % INSTRUCTION_COUNT;
    int m_prime_start;

    for (i = 0; i < INSTRUCTION_COUNT; i++) {
        Instruction instr;
        instr.address = m_start + i;

        // 50%˳��ִ��
        if (i % 2 == 0) {
            instr.page = instr.address / PAGE_SIZE;
        }
        else {
            // 25%��ǰ��ַ���־��ȷֲ�
            if (i % 4 == 1) {
                m_prime_start = rand() % (m_start + 1);
            }
            // 25%�ں��ַ���־��ȷֲ�
            else {
                m_prime_start = (rand() % (INSTRUCTION_COUNT - m_start - 1)) + m_start + 1;
            }
            instr.page = m_prime_start / PAGE_SIZE;
        }

        instructions[i] = instr;
    }
}
*/