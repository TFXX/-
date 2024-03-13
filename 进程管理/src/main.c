#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct stProcess
{
	// ����ID
	int id;

	// ���̵����ʱ���
	unsigned int arriveTime;

	// ����ִ���������ʱ����
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

	// doneProcNum���ڼ�¼�Ѿ�ִ����Ľ�������
	size_t doneProcNum = 0;

	// ��ǰʱ���
	unsigned int time = 0;
	while (true)
	{
		if (queue_empty(&q))
		{
			// �����н���ȫ��ִ����ɺ��˳�����ģ��
			if (procNum == doneProcNum)
			{
				break;
			}

			// ��ʱ����ƽ�����һ�����̵ĵ���ʱ��
			// ����һ����������Ľ�����ӵ�����
			time += proc[doneProcNum].arriveTime;
			queue_push(&q, &proc[doneProcNum++]);
		}

		const Process *p = queue_pop(&q);

		// �������ִ����ɵ�ʱ��
		unsigned int doneTime = time + p->runningDu;

		// ������̵���תʱ�䣨�����
		unsigned int turnRoundDu = doneTime - p->arriveTime;

		// ������̿�ʼ���е�ʱ��
		unsigned int startTime = time;

		// ģ�⵱ǰ����ʱ��Ƭִ�й���
		for (; time < doneTime; time++)
		{
			if (doneProcNum == procNum)
			{
				continue;
			}
		
			// ���ִ�е���һ�����̵ĵ���ʱ��
			// �ͽ���һ��������ӵ����̶�����
			if (proc[doneProcNum].arriveTime == time)
			{
				queue_push(&q, &proc[doneProcNum++]);
			}
		}

		// ���Ѿ�ִ����Ľ�����Ϣ��ӡ
		printf(
			"FCFS]����id��%04d������ʱ�䣺%4d������ʱ�䣺%4d����ʼʱ�䣺%4d������ʱ�䣺%4d����תʱ�䣺%4d\n",
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
	// ��ȡ��������
	size_t procNum;
	printf("]��������̵ĸ���\n>");
	scanf("%zu", &procNum);

	// ������������
	Process *proc = (Process *)calloc(procNum, sizeof(Process));

	// ������̵���Ϣ
	printf(
		"]�밴��ʽ����%zu��������Ϣ\n"
		"]<id> <����ʱ��> <����ʱ��>\n",
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