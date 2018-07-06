 ///
 /// @file    ngx_util.h
 /// @author  faith(kh_faith@qq.com)
 /// @date    2018-07-05 18:06:23
 ///
 
#ifndef __NGX_UTIL_H__
#define __NGX_UTIL_H__

typedef struct{
	u_char *session;
	u_char *data;
	u_char *requesttime;
	u_char *ser_data;
	u_char key[33];
	ngx_msec_t start;
	ngx_msec_t end;
}ngx_http_mytest_ctx_t;

typedef struct {
    u_char rbtree_node_data; 
    ngx_queue_t queue;        
    size_t len;             
	size_t key_len;
    u_char data[33];         
    u_char key[33];         
} ngx_http_mytest_node_t;

typedef struct {
    ngx_rbtree_t rbtree;
    ngx_rbtree_node_t sentinel;
    ngx_queue_t queue;
} ngx_http_mytest_shm_t;

typedef struct {
    ssize_t shmsize;
    ngx_slab_pool_t* shpool;
    ngx_http_mytest_shm_t* sh;
} ngx_http_mytest_conf_t;

ngx_int_t ngx_http_mytest_init(ngx_conf_t* cf);
char * ngx_http_mytest_createmem(ngx_conf_t* cf, ngx_command_t* cmd, void* conf); 
void* ngx_http_mytest_create_main_conf(ngx_conf_t* cf);

ngx_int_t ngx_http_mytest_lookup(ngx_http_request_t* r,ngx_http_mytest_conf_t* conf,ngx_uint_t hash, u_char* data,u_char *key,size_t len); 
ngx_int_t ngx_http_mytest_insert(ngx_http_request_t* r,ngx_http_mytest_conf_t* conf,ngx_uint_t hash, u_char* data,u_char *key, size_t len,size_t key_len); 
void ngx_http_mytest_expire(ngx_http_request_t* r, ngx_http_mytest_conf_t* conf);
void ngx_http_mytest_rbtree_insert_value(ngx_rbtree_node_t* temp,ngx_rbtree_node_t* node,ngx_rbtree_node_t* sentinel) ;

ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);
char *ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);
ngx_int_t mytest_post_handler(ngx_http_request_t * r);
ngx_int_t mytest_subrequest_post_real_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);
ngx_int_t mytest_post_real_handler(ngx_http_request_t * r);

#endif
