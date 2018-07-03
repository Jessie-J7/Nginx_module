 ///
 /// @file    queue.h
 /// @author  faith(kh_faith@qq.com)
 /// @date    2018-07-03 14:48:26
 ///
 
#ifndef __QUEUE_H__
#define __QUEUE_H__

/* 接口的实现文件 */
#include<stdio.h>
#include<stdlib.h>
#include <ngx_core.h>
#include <ngx_config.h>

#define true 1
#define false 0
#define	QUEUE_SIZE 5

/* 队列的数据类型 */
typedef u_char * datatype;

/* 静态链的数据结构 */
typedef struct q_node{
	datatype key;
	datatype value;
	struct q_node *next;
}q_node,*link_node;

typedef struct l_queue{
	/* 队头指针 */
	q_node *front;
	/* 队尾指针 */
	q_node *rear;
}*link_queue;


link_queue queue_init();
int queue_en(link_queue q, datatype k,datatype v);
int queue_lru(link_queue q,datatype k,datatype v);
int queue_putLru(link_queue q,datatype k,datatype v);

#endif
