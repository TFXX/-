#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct stProcess
{
	// 进程ID
	int id;

	// 进程到达的时间点
	unsigned int arriveTime;

	// 进程执行完所需的时间间隔
	unsigned int runningDu;
} Process;

typedef struct stQueue
{
	const Process **p;
	size_t head, rear;
	size_t maxSize;
} Queue;

void queue_init(Queue *p, size_t size)
{
	p->maxSize = size + 1;
	p->head = 0;
	p->rear = 0;
	p->p = calloc(p->maxSize, sizeof(Process *));
}

void queue_uninit(Queue *p)
{
	free(p->p);
}

bool queue_empty(const Queue *p)
{
	return p->head == p->rear;
}

void queue_push(Queue *pQ, const Process *pP)
{
	if (pQ->rear + 1 == pQ->head)
	{
		return;
	}

	pQ->p[pQ->rear] = pP;
	pQ->rear = (pQ->rear + 1) % pQ->maxSize;
}

const Process *queue_pop(Queue *p)
{
	if (queue_empty(p))
	{
		return NULL;
	}

	const Process *pP = p->p[p->head];

	p->head = (p->head + 1) % p->maxSize;

	return pP;
}

void FCFS(const Process proc[], const size_t procNum)
{
	Queue q;
	queue_init(&q, procNum);

	// doneProcNum用于记录已经执行完的进程数量
	size_t doneProcNum = 0;

	// 当前时间点
	unsigned int time = 0;
	while (true)
	{
		if (queue_empty(&q))
		{
			// 当所有进程全部执行完成后退出调度模拟
			if (procNum == doneProcNum)
			{
				break;
			}

			// 将时间点推进到下一个进程的到达时间
			// 将下一个即将到达的进程添加到队列
			time += proc[doneProcNum].arriveTime;
			queue_push(&q, &proc[doneProcNum++]);
		}

		const Process *p = queue_pop(&q);

		// 计算进程执行完成的时间
		unsigned int doneTime = time + p->runningDu;

		// 计算进程的周转时间（间隔）
		unsigned int turnRoundDu = doneTime - p->arriveTime;

		// 计算进程开始运行的时间
		unsigned int startTime = time;

		// 模拟当前进程时间片执行过程
		for (; time < doneTime; time++)
		{
			if (doneProcNum == procNum)
			{
				continue;
			}
		
			// 如果执行到下一个进程的到达时间
			// 就将下一个进程添加到进程队列中
			if (proc[doneProcNum].arriveTime == time)
			{
				queue_push(&q, &proc[doneProcNum++]);
			}
		}

		// 将已经执行完的进程信息打印
		printf(
			"FCFS]进程id：%04d，到达时间：%4d，服务时间：%4d，开始时间：%4d，结束时间：%4d，周转时间：%4d\n",
			p->id,
			p->arriveTime,
			p->runningDu,
			startTime,
			doneTime,
			turnRoundDu
		);
	}

	queue_uninit(&q);
}

int main(int argc, char **argv)
{
	// 获取进程数量
	size_t procNum;
	printf("]请输入进程的个数\n>");
	scanf("%zu", &procNum);

	// 创建进程数组
	Process *proc = (Process *)calloc(procNum, sizeof(Process));

	// 输入进程的信息
	printf(
		"]请按格式输入%zu条进程信息\n"
		"]<id> <到达时间> <服务时间>\n",
		procNum
	);

	for (size_t i = 0; i < procNum; i++)
	{
		printf(">");
		scanf("%d %d %d", &proc[i].id, &proc[i].arriveTime, &proc[i].runningDu);
	}

	FCFS(proc, procNum);
	return 0;
}