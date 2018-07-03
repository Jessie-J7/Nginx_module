///
/// @file    queue.c
/// @author  faith(kh_faith@qq.com)
/// @date    2018-07-03 09:23:32
///

#include "queue.h"

int queue_size = 0;
link_queue queue_init()
{
	link_queue q = (link_queue)malloc(sizeof(*q));
	q -> front = q -> rear = NULL;
	return q;
}

int queue_en(link_queue q, datatype k,datatype v)
{
	/* 新建数据结点 */
	link_node new_node = (link_node)malloc(sizeof(q_node));
	/* 内存分配失败 */
	if(!new_node)
		return false;
	new_node -> key = k;
	new_node -> value = v;
	if(q->front == NULL)
	{
		q->front = new_node;
		q->rear = new_node;
	}
	else
	{
		new_node -> next = q -> front;
		q -> front = new_node;
	}
	queue_size ++;
	return true;
}

//void queue_traverse(link_queue q, void(*visit)(link_queue q))
//{
//	visit(q);
//}
//
//void visit(link_queue q)
//{
//	/* 头结点 */
//	link_node p = q -> front;
//	if(!p)
//	{
//		printf("队列为空");
//	}
//	while(p)
//	{
//		printf("%s ", p -> value);
//		p = p -> next;
//	}
//	printf("\n");
//}

int queue_lru(link_queue q,datatype k,datatype v)
{
	link_node p = q->front;
	if(!p)
	{
		printf("队列为空");
		return -1;
	}
	link_node temp;
	if(ngx_strcmp(p->key,k) == 0)
	{
		ngx_memcpy(v, p->value,ngx_strlen(p->value));
		return 0;
	}
	else
	{
		temp = p;
		p = p->next;
	}
	size_t i;
	while(p)
	{
		for(i = 0;i < ngx_strlen(k);i++)
		{
			if(p->key[i] == k[i])
			{	
			}
			else
			{
				temp = p;
				p = p->next;
				break;
			}
		}
		if(p->key[i] == k[i] && i == ngx_strlen(k))
			break;
	}
	ngx_memcpy(v, p->value,ngx_strlen(p->value));
	//删除节点
	temp->next = p->next;
	free(p);
	temp = NULL;
	queue_size--;
	//插入到队头
	queue_en(q,k,v);
	return true;
}
int queue_putLru(link_queue q,datatype k,datatype v)
{
	link_node p = q->front;
	if(queue_size < QUEUE_SIZE)
		queue_en(q,k,v);
	else
	{
		while(p)
		{
			if(p->next == q->rear)
			{
				p->next = q->rear->next;
				queue_size--;
				break;
			}
			p = p->next;
		}
		queue_en(q,k,v);
	}
	return 0;
}

//int main()
//{
//	link_queue q = queue_init();
//	const char *a = "12";
//	queue_putLru(q, a,"111");
//	queue_putLru(q, "34","222");
//	queue_putLru(q, "56","333");
//	queue_putLru(q, "78","444");
//	queue_putLru(q, "90","555");
//	queue_putLru(q, "qw","666");
//	queue_traverse(q,visit);
//	printf("queue_size : %d\n",queue_size);
////	datatype *k = (datatype *)malloc(sizeof(*k));
////	datatype *v = (datatype *)malloc(sizeof(*v));
//
//	datatype test = "34";
//	char *v = (char *)malloc(sizeof(*v));
//	queue_lru(q,test,v);
//	queue_traverse(q,visit);
//	printf("queue_size : %d\n",queue_size);
//	return 0;
//}


