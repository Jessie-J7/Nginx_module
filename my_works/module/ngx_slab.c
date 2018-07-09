 ///
 /// @file    slab.c
 /// @author  faith(kh_faith@qq.com)
 /// @date    2018-07-05 18:01:09
 ///

#include <ngx_core.h>
#include <ngx_config.h>
#include <ngx_http.h>
#include "ngx_util.h"
//红黑树插入方法，可以初始化红黑树conf->sh->rbtree
void ngx_http_mytest_rbtree_insert_value(ngx_rbtree_node_t* temp,ngx_rbtree_node_t* node,ngx_rbtree_node_t* sentinel) 
{
    ngx_rbtree_node_t** p;
    ngx_http_mytest_node_t* lrn;
    ngx_http_mytest_node_t* lrnt;
    for (;;) 
	{
        if (node->key < temp->key) 
		{
            p = &temp->left;
        }
		else if (node->key > temp->key)
		{
            p = &temp->right;
        }
		else 
		{
            lrn = (ngx_http_mytest_node_t*)&node->data;
            lrnt = (ngx_http_mytest_node_t*)&temp->data;
            p = ngx_memn2cmp(lrn->data, lrnt->data, lrn->len, lrnt->len) < 0? &temp->left : &temp->right;
        }
        if (*p == sentinel){
            break;
        }
        temp = *p;
    }
    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}
//释放共享内存空间，删除队列的最后一个结点
void ngx_http_mytest_expire(ngx_http_request_t* r, ngx_http_mytest_conf_t* conf)
{
    if (ngx_queue_empty(&conf->sh->queue))
	{
        return;
    }
	//找到conf->sh->queue,一个lru队列的最后一个结点
    ngx_queue_t* q = ngx_queue_last(&conf->sh->queue);
    ngx_http_mytest_node_t* lr =ngx_queue_data(q, ngx_http_mytest_node_t, queue);
    ngx_rbtree_node_t* node = (ngx_rbtree_node_t*)((u_char*)lr - offsetof(ngx_rbtree_node_t, data));

	size_t size = offsetof(ngx_rbtree_node_t, data) + offsetof(ngx_http_mytest_node_t,key) +lr->key_len;
	ngx_log_stderr(0,"size : %d",size);
	conf->shmsize += size;

	//删除结点
    ngx_queue_remove(q);
    ngx_rbtree_delete(&conf->sh->rbtree, node);

    ngx_slab_free_locked(conf->shpool, node);
}
//在共享内存中查找key
ngx_int_t ngx_http_mytest_lookup(ngx_http_request_t* r,ngx_http_mytest_conf_t* conf,ngx_uint_t hash, u_char* data,u_char *key,size_t len) 
{
    ngx_rbtree_node_t* node = conf->sh->rbtree.root;
    ngx_rbtree_node_t* sentinel = conf->sh->rbtree.sentinel;

    ngx_http_mytest_node_t* lr; 
	//hash查找
    while (node != sentinel) {
        if (hash < node->key) {
            node = node->left;
            continue;
        }
        if (hash > node->key) {
            node = node->right;
            continue;
        }
        lr = (ngx_http_mytest_node_t*)&node->data;
		ngx_log_stderr(0,"data_rtree: %s",lr->data);
        ngx_int_t rc = ngx_strncmp(data, lr->data, lr->len);
        if (rc == 0) {//如果找到
			ngx_log_stderr(0,"lr->key: %s",lr->key);
			ngx_memcpy(key,lr->key,lr->key_len);
			//从队列中移出，插入conf->sh->queue的队首，维护一个lru队列
            ngx_queue_remove(&lr->queue);
            ngx_queue_insert_head(&conf->sh->queue, &lr->queue);
            return NGX_DECLINED;
        }
        node = rc < 0? node->left : node->right;
    }
	ngx_log_stderr(0,"key: %s",key);
    return NGX_DECLINED;    
}
//插入session，key
ngx_int_t ngx_http_mytest_insert(ngx_http_request_t* r,ngx_http_mytest_conf_t* conf,ngx_uint_t hash, u_char* data,u_char *key,size_t len,size_t key_len) 
{
    ngx_rbtree_node_t* node = conf->sh->rbtree.root;
    ngx_http_mytest_node_t* lr;    
	//计算要插入的长度
	size_t size = offsetof(ngx_rbtree_node_t, data)+ offsetof(ngx_http_mytest_node_t,key) +key_len;
	ngx_log_stderr(0,"size : %d",size);
	conf->shmsize -= size;
	//如果共享内存空间不够，释放空间
    while(conf->shmsize < 0)
	{
		ngx_log_stderr(0,"ngx_http_mytest_expire");
		ngx_http_mytest_expire(r, conf);
	}
    node = (ngx_rbtree_node_t*)ngx_slab_alloc_locked(conf->shpool, size);
    if (node == NULL) {//共享内存不足
		ngx_http_mytest_expire(r, conf);
    }
    node->key = hash;

    lr = (ngx_http_mytest_node_t*)&node->data;
	lr->len = len;
	lr->key_len = key_len;
    ngx_memcpy(lr->data, data, len);
    ngx_memcpy(lr->key,key,key_len);
	ngx_log_stderr(0,"data_slab: %s %d",lr->data,lr->len);
	ngx_log_stderr(0,"lr->key: %s",lr->key);

	//插入红黑树，插入队首
    ngx_rbtree_insert(&conf->sh->rbtree, node);
    ngx_queue_insert_head(&conf->sh->queue, &lr->queue);
    return NGX_DECLINED;    
}


