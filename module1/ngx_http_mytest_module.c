/*
 *  * Copyright (C) yejianfeng
 *   */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "func.h"
typedef struct{
	u_char *session;
	u_char *data;
	u_char *requesttime;
	u_char key[33];
}ngx_http_mytest_ctx_t;

ngx_int_t deaes_data(ngx_http_request_t *r,u_char *c,u_char *data,u_char *requesttime,u_char *key);
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);
static char *ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_http_mytest_commands[] = {
	{
		ngx_string("mytest"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
		ngx_http_mytest,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	ngx_null_command
};

static ngx_http_module_t ngx_http_mytest_module_ctx = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

ngx_module_t ngx_http_mytest_module = {
	NGX_MODULE_V1,
	&ngx_http_mytest_module_ctx,
	ngx_http_mytest_commands,
	NGX_HTTP_MODULE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NGX_MODULE_V1_PADDING
};

	static char * 
ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t *clcf;

	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

	clcf->handler = ngx_http_mytest_handler;

	return NGX_CONF_OK;
}
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
	//创建http上下文
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
	if (myctx == NULL)
	{
		myctx = ngx_palloc(r->pool, sizeof(ngx_http_mytest_ctx_t));
		if (myctx == NULL)
		{
			return NGX_ERROR;
		}

		//将上下文设置到原始请求r中
		ngx_http_set_ctx(r, myctx, ngx_http_mytest_module);
	}
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response;
	response.len = 0;

	myctx->session = ngx_alloc(33,r->pool->log);
	ngx_memzero(myctx->session,33);
	ngx_str_t str1 = ngx_string("session");
	query_string(r,myctx->session,str1);         //获取参数中的session
	myctx->data = ngx_alloc(1024,r->pool->log);
	ngx_memzero(myctx->data,1024);
	ngx_str_t str2 = ngx_string("data");
	query_string(r,myctx->data,str2);           //获取data
	myctx->requesttime = ngx_alloc(16,r->pool->log);
	ngx_memzero(myctx->requesttime,16);
	ngx_str_t str3 = ngx_string("requesttime");
	query_string(r,myctx->requesttime,str3);	 //获取requesttime

	u_char filename[] = {"/home/faith/ngx_test"};
	read_file(r,filename,myctx->session,myctx->key);  //根据session，在文件中查找key

	u_char *c = ngx_alloc(4096,r->pool->log);
	ngx_memzero(c,4096);
	if(ngx_strlen(myctx->key) == 0) //进入token
	{
		
	}
	else
	{
		ngx_int_t ret = deaes_data(r,c,myctx->data,myctx->requesttime,myctx->key);
		if(ret == 0)
		{
			response.len = ngx_strlen(c);
			response.data = ngx_alloc(response.len,r->pool->log);
			ngx_memcpy(response.data,c,response.len);   //返回解密后的数据
		}
	}
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;

	ngx_int_t rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		return rc;
	}

	ngx_buf_t *b;
	b = ngx_create_temp_buf(r->pool, response.len);
	if (b == NULL) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	ngx_memcpy(b->pos, response.data, response.len);
	b->last = b->pos + response.len;
	b->last_buf = 1;

	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;

	return ngx_http_output_filter(r, &out);
}
ngx_int_t deaes_data(ngx_http_request_t *r,u_char *c,u_char *data,u_char *requesttime,u_char *key)
{
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
	ngx_str_t pencode;   //编码数据
	ngx_str_t pdecode;   //解码数据

	pencode.len = ngx_strlen(data);
	pencode.data = ngx_alloc(pencode.len + 1,r->pool->log);
	ngx_memcpy(pencode.data,data,pencode.len);
	ngx_log_stderr(0,"pencode: %s",pencode.data);

	pdecode.len = ngx_base64_decoded_length(pencode.len);
	pdecode.data = ngx_alloc(pdecode.len + 1,r->pool->log);
	ngx_decode_base64(&pdecode,&pencode);     //base64解码
	ngx_log_stderr(0,"pdecode: %s",pdecode.data);

	ngx_memcpy(c,pdecode.data,pdecode.len);
	ngx_int_t clen = pdecode.len;
	deAes(c,clen,myctx->key);         //aes算法解密
	ngx_log_stderr(0,"c: %s",c);

	ngx_int_t i=0,j=0,k=0,templen=0;
	u_char d[3][16] = {0};
	u_char *temp = c;
	while(i < clen)             //对解密后的数据分段
	{
		if(c[i] == ' ')
		{
			templen = i - k;
			ngx_log_stderr(0,"templen : %d",templen);
			ngx_memcpy(d[j],temp,templen);
			j++;
			templen++;
			k = k + templen;
			temp = temp + templen;
		}
		if(j == 3)
			break;
		i++;
	}
	ngx_log_stderr(0,"d[0]: %s",d+0);
	ngx_log_stderr(0,"d[1]: %s",d+1);
	ngx_log_stderr(0,"d[2]: %s",d+2);

	if(ngx_strcmp(requesttime,d[2]) == 0)  //对比请求时间
	{
		return 0;
	}else{
		ngx_log_stderr(0,"requesttime error");
		return -1;
	}
}
ngx_int_t read_file(ngx_http_request_t *r,u_char *realfilename,u_char *session,u_char *key){
	ngx_file_t *file;
	ngx_file_info_t fi;

	file = ngx_palloc(r->pool,sizeof(ngx_file_t));
	file->fd = ngx_open_file(realfilename,NGX_FILE_RDONLY,NGX_FILE_OPEN,0);
	if(file->fd < 0){
		ngx_log_stderr(0,"%s failed",realfilename);
		return NGX_ERROR;
	}
	ngx_log_stderr(0,"file->fd : %d",file->fd);
	if(ngx_fd_info(file->fd,&fi) == NGX_FILE_ERROR)
	{
		ngx_log_stderr(0,"%s failed",realfilename);
		return NGX_ERROR;
	}

	off_t size = ngx_file_size(&fi);
	ngx_log_stderr(0,"size : %d",size);
	size_t len = 33;
	u_char buf[33]={0};

	off_t filelen = 0;
	ssize_t n;
	while(filelen < size)
	{
		n = ngx_read_fd(file->fd,buf,len-1);
		ngx_log_stderr(0,"n: %d",n);
		ngx_log_stderr(0,"session : %s",buf);
		if(ngx_strcmp(buf,session) == 0)
		{	
			filelen += len;
			lseek(file->fd,filelen,SEEK_SET);
			n = ngx_read_fd(file->fd,buf,len-1);
			ngx_log_stderr(0,"n: %d",n);
			ngx_log_stderr(0,"key : %s",buf);
			ngx_memcpy(key,buf,len);
			break;
		}
		filelen += 2 * (off_t)len;
		lseek(file->fd,filelen,SEEK_SET);
	}
	return 0;
}


