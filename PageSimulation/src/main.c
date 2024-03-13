#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INS_NUM 320

// 一页最多存储10条指令
#define INS_PER_PAGE 10

// 用户内存最小为4K
#define MIN_USER_MEM_SIZE 4

// 用户内存最大为32K
#define MAX_USER_MEM_SIZE 32

// 模拟指令结构
typedef struct stInstruction {
    // 指令的逻辑地址
    size_t address;
    
    // 指令在虚拟内存中的页码位置
    int page;
} Instruction;


// 模拟存储器中的页面结构
typedef struct stPage
{
    // 页码
    int page;

    // 用于比较两个页面之间最后一次被使用的先后顺序
    int  useOrder;
} Page;

// 构造指令序列
void generate_instruction(Instruction ins[])
{
    // 随机选取一个起点

    // 让m在[0, 319]中随机生成
    size_t m = rand() % INS_NUM + 0;

    // 通过随机数产生一个指令序列
    for (size_t i = 0; i < INS_NUM; i++)
    {
        // 构造下一条指令序列的位置
        switch (i % 4)
        {
        case 0:
        case 2:
            // 50%的概率让指令顺序执行
            // 同时确保m不会越界，让指令序列的地址空间首尾相连
            m = (m + 1) % INS_NUM;
            break;
        case 1:
            // 让m在前地址[0, m]中随机生成
            m = rand() % m + 0;
            break;
        case 3:
            // 让m在后地址[m+1, 319]中随机生成
            m = rand() % (319 - (m + 1)) + (m + 1);
            break;
        }

        ins[i].address = m;

        // 计算指令所在的页码
        ins[i].page = (int)(ins[i].address / INS_PER_PAGE);
    }
}

// FIFO页面置换算法
void FIFO_page_replace(
    const size_t userMemSize,
    const Instruction ins[],
    unsigned int *pCnt)
{
    // 模拟用户内存空间
    // 多余的一个页是虚构的，充当哨兵
    Page *userMem = (Page *)calloc(userMemSize + 1, sizeof(Page));

    // 哨兵，用于优化掉后面判断指令流是否在用户内存中的比较逻辑
    const size_t sentinel = userMemSize;

    // 用于累加页面失效次数
    *pCnt = 0;

    // 当页面时效应当调入的用户内存页码
    size_t nextPage = 0;

    // 开始时没有页被调入用户内存
    for (size_t i = 0; i < userMemSize; i++)
    {
        userMem[i].page = -1;
    }

    // 模拟页地址流运行
    for (size_t i = 0; i < INS_NUM; i++)
    {
        // 设置哨兵
        userMem[sentinel].page = ins[i].page;

        size_t j = 0;

        // 检查页面是否在内存中
        for (; j <= sentinel; j++)
        {
            if (ins[i].page == userMem[j].page)
            {
                break;
            }
        }

        // 没有找到页面
        if (sentinel == j)
        {
            // 累加页面失效次数
            (*pCnt)++;

            // 请求调页
            userMem[nextPage].page = ins[i].page;
            nextPage = (nextPage + 1) % userMemSize;
        }
    }

    free(userMem);
}

// LRU页面置换算法
void LRU_page_replace(const size_t userMemSize, const Instruction ins[], unsigned int *pCnt)
{
    // 模拟用户内存空间
    // 多余的一个页是虚构的，充当哨兵
    Page *userMem = (Page *)calloc(userMemSize + 1, sizeof(Page));

    // 哨兵，用于优化掉后面判断指令流是否在用户内存中的比较逻辑
    const size_t sentinel = userMemSize;

    // 用于累加页面失效次数
    *pCnt = 0;

    // 开始时没有页被调入用户内存
    for (size_t i = 0; i < userMemSize; i++)
    {
        userMem[i].page = -1;
        userMem[i].useOrder = -1;
    }

    // 模拟页地址流运行
    for (size_t i = 0; i < INS_NUM; i++)
    {
        // 设置哨兵
        userMem[sentinel].page = ins[i].page;

        size_t j = 0;

        // 检查页面是否在内存中
        for (; j <= sentinel; j++)
        {
            if (ins[i].page == userMem[j].page)
            {
                break;
            }
        }

        // 没有找到页面
        if (sentinel == j)
        {
            // 累加页面失效次数
            (*pCnt)++;

            j = 0;

            // 查找最久没有被使用的页面
            for (size_t k = j + 1; k < userMemSize; k++)
            {
                if (userMem[j].useOrder > userMem[k].useOrder)
                {
                    j = k;
                }
            }

            // 请求调页
            userMem[j].page = ins[i].page;
        }

        // 此时j为指令对应页码也用户内存中的物理序号

        // 更新页上一次被寻址的时间
        userMem[j].useOrder = (int)i;
    }

    free(userMem);
}

// 计算命中率
inline float calculate_hit_rate(const unsigned int faultCnt, const size_t addrStmLength)
{
    return (float)(addrStmLength - faultCnt) / addrStmLength;
}

int main(int argc, char **argv)
{
    srand((unsigned int)time(NULL));

    // 构造指令序列
    Instruction *ins = (Instruction *)calloc(INS_NUM, sizeof(Instruction));
    generate_instruction(ins);

    for (size_t userMemSize = MIN_USER_MEM_SIZE; userMemSize <= MAX_USER_MEM_SIZE; userMemSize++)
    {
        // 内存失效次数
        unsigned int FIFOFault, LRUFault;

        // 模拟FIFO和LRU调页
        FIFO_page_replace(userMemSize, ins, &FIFOFault);
        LRU_page_replace(userMemSize, ins, &LRUFault);

        float FIFO_hit_rate = calculate_hit_rate(FIFOFault, INS_NUM);
        float LRU_hit_rate = calculate_hit_rate(LRUFault, INS_NUM);

        printf(
            "]内存容量:%2zuK\tFIFO命中率:%0.2f\tLRU命中率:%0.2f\n",
            userMemSize,
            FIFO_hit_rate,
            LRU_hit_rate
        );
    }

    free(ins);
    return 0;
}

/*
// 指令结构体
typedef struct {
    int address;
    int page;
} Instruction;

// 页面结构体
typedef struct {
    int page_num;
    int last_used;
} Page;

// 随机生成指令序列
void generateInstructions(Instruction* instructions) {
    int i, m, m_prime;
    int m_start = rand() % INSTRUCTION_COUNT;
    int m_prime_start;

    for (i = 0; i < INSTRUCTION_COUNT; i++) {
        Instruction instr;
        instr.address = m_start + i;

        // 50%顺序执行
        if (i % 2 == 0) {
            instr.page = instr.address / PAGE_SIZE;
        }
        else {
            // 25%在前地址部分均匀分布
            if (i % 4 == 1) {
                m_prime_start = rand() % (m_start + 1);
            }
            // 25%在后地址部分均匀分布
            else {
                m_prime_start = (rand() % (INSTRUCTION_COUNT - m_start - 1)) + m_start + 1;
            }
            instr.page = m_prime_start / PAGE_SIZE;
        }

        instructions[i] = instr;
    }
}
*/