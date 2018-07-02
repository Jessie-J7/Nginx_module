/*
 *  * Copyright (C) yejianfeng
 *   */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "func.h"
#include "cJSON.h"
typedef struct{
	u_char *session;
	u_char *data;
	u_char *requesttime;
	u_char *ser_data;
	u_char key[33];
	struct timeval start;
	struct timeval end;
}ngx_http_mytest_ctx_t;

static ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);
static ngx_int_t
mytest_post_handler(ngx_http_request_t * r);
static ngx_int_t mytest_subrequest_post_key_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);
static void
mytest_post_key_handler(ngx_http_request_t * r);
ngx_int_t deaes_data(ngx_http_request_t *r,u_char *data,u_char *requesttime,u_char *key);
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

ngx_int_t cJSON_to_array(char *text,u_char *key,u_char *session,ngx_http_request_t *r)
{
	cJSON *json,*arrayItem,*item,*object;
	ngx_int_t i;

	json=cJSON_Parse(text);
	if (!json)
	{
		ngx_log_stderr(0,"Error before: [%s]\n",cJSON_GetErrorPtr());
	}
	else
	{
		arrayItem=cJSON_GetObjectItem(json,"token");
		if(arrayItem!=NULL)
		{
			int size=cJSON_GetArraySize(arrayItem);
			ngx_log_stderr(0,"cJSON_GetArraySize: size=%d\n",size);

			for(i=0;i<size;i++)
			{
				object = cJSON_GetArrayItem(arrayItem,i);

				item = cJSON_GetObjectItem(object,"session");
				if(item != NULL)
				{
					ngx_log_stderr(0,"item: %s",item->valuestring);
					if(0 == ngx_strcmp(item->valuestring,(char *)session))
					{
						item = cJSON_GetObjectItem(object,"key");
						if(item != NULL)
						{
							ngx_memzero(key,ngx_strlen(item->valuestring)+1);
							ngx_memcpy(key,item->valuestring,ngx_strlen(item->valuestring));
							ngx_log_stderr(0,"key: %s",key);
						}
						break;
					}
				}
			}
		}
	}
	cJSON_Delete(json);
	return 0;
}

static ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r,void *data, ngx_int_t rc)
{
	ngx_http_request_t          *pr = r->parent;
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(pr, ngx_http_mytest_module);

	pr->headers_out.status = r->headers_out.status;
	if (r->headers_out.status == NGX_HTTP_OK)//解析json，取key
	{
		ngx_log_stderr(0,"data: %s",myctx->data);
	}
	ngx_buf_t* pRecvBuf = &r->upstream->buffer;
	
	ngx_str_t cjson;
	cjson.len = pRecvBuf->last - pRecvBuf->pos;
	cjson.data = pRecvBuf->pos;

	cJSON_to_array((char *)cjson.data,myctx->key,myctx->session,r);
	ngx_log_stderr(0,"myctx->key: %s",myctx->key);

	u_char filename[] = {"/home/faith/ngx_test"};
	ngx_file_t *file;
	file = ngx_palloc(r->pool,sizeof(ngx_file_t));
	file->fd = ngx_open_file(filename,NGX_FILE_WRONLY,NGX_FILE_CREATE_OR_OPEN,0644);
	if(file->fd < 0){
		ngx_log_stderr(0,"%s failed",filename);
		return NGX_ERROR;
	}
	ngx_log_stderr(0,"file->fd : %d",file->fd);
	u_char * buf = ngx_alloc(66,r->pool->log);
	ngx_snprintf(buf,66,"%s %s\n",myctx->session,myctx->key);
	ngx_log_stderr(0,"buf : %s",buf);
	ngx_write_fd(file->fd,buf,66);

	pr->write_event_handler = (void*)mytest_post_handler;

	return NGX_OK;
}

static ngx_int_t mytest_post_handler(ngx_http_request_t * r)
{
	if (r->headers_out.status != NGX_HTTP_OK)
	{
		ngx_http_finalize_request(r, r->headers_out.status);
		return NGX_ERROR;
	}

	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);

	if(ngx_strlen(myctx->key) == 0)
	{
		//没有key返回用户错误码
		return NGX_ERROR;
	}
	else
	{
		ngx_int_t ret = deaes_data(r,myctx->data,myctx->requesttime,myctx->key);
		if(ret == 0)
		{
			//ret等于0说明 requesttime相等。
			ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
			if (psr == NULL)
			{
				return NGX_ERROR;
			}
			psr->handler = mytest_subrequest_post_key_handler;

			psr->data = myctx;
			ngx_str_t sub_prefix = ngx_string("/rest/index");
			ngx_str_t sub_location;
			sub_location.len = sub_prefix.len;
			sub_location.data = ngx_palloc(r->pool, sub_location.len);
			ngx_snprintf(sub_location.data, sub_location.len,
					"%V", &sub_prefix);

			ngx_http_request_t *sr;
			ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);
			if (rc != NGX_OK)
			{
				return NGX_ERROR;
			}
			return NGX_DONE;
		}
		else
		{
			return NGX_ERROR;
		}
	}
}
ngx_int_t cJSON_to_str(char *json_string,u_char *serdata,ngx_http_request_t *r)
{
	cJSON *root=cJSON_Parse(json_string);
	if (!root)
	{
		ngx_log_stderr(0,"Error before: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
		cJSON *item=cJSON_GetObjectItem(root,"body");
		if(item!=NULL)
		{
			ngx_memcpy(serdata,item->valuestring,ngx_strlen(item->valuestring));
			ngx_log_stderr(0,"serdata : %s",serdata);
		}
		cJSON_Delete(root);
	}
	return 0;
}
static ngx_int_t mytest_subrequest_post_key_handler(ngx_http_request_t *r,void *data, ngx_int_t rc)
{
	//当前请求r是子请求，它的parent成员就指向父请求
	ngx_http_request_t          *pr = r->parent;
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(pr, ngx_http_mytest_module);

	pr->headers_out.status = r->headers_out.status;
	if (r->headers_out.status == NGX_HTTP_OK)//取数据server_data，放入ctx
	{
		ngx_log_stderr(0,"key: %s",myctx->key);
	}
	ngx_buf_t* pRecvBuf = &r->upstream->buffer;

	ngx_str_t cjson;
	cjson.len = pRecvBuf->last - pRecvBuf->pos;
	cjson.data = pRecvBuf->pos;

	myctx->ser_data = ngx_alloc(1024,r->pool->log);
	ngx_memzero(myctx->ser_data,1024);
	cJSON_to_str((char *)cjson.data,myctx->ser_data,r);
	ngx_log_stderr(0,"myctx->ser_data : %s",myctx->ser_data);
	pr->write_event_handler = mytest_post_key_handler;

	return NGX_OK;
}


static void mytest_post_key_handler(ngx_http_request_t * r)
{
	if (r->headers_out.status != NGX_HTTP_OK)
	{
		ngx_http_finalize_request(r, r->headers_out.status);
		return;
	}	
	//当前请求是父请求，直接取其上下文
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
	u_char *p = ngx_alloc(ngx_strlen(myctx->ser_data)+1,r->pool->log);
	ngx_memzero(p,ngx_strlen(myctx->ser_data)+1);
	ngx_memcpy(p,myctx->ser_data,ngx_strlen(myctx->ser_data));
	ngx_log_stderr(0,"p: %s",p);
	ngx_int_t plen = ngx_strlen(p);
	if(plen == 0){
		ngx_log_stderr(0,"明文字符长度不为空！\n");
		return;
	}
	ngx_int_t i;
	ngx_int_t pstrlen = plen;
	//	ngx_int_t psub = 16 - pstrlen % 16;
	if(pstrlen % 16 != 0) {
		plen = (plen / 16 + 1) * 16;
		for(i=pstrlen;i<plen;i++)
			p[i] = ' ';
	}
	aes(p,plen,myctx->key);
	ngx_str_t pcur; 
	pcur.len = plen;
	pcur.data = ngx_alloc(pcur.len,r->pool->log);
	ngx_memcpy(pcur.data,p,pcur.len);

	ngx_str_t pencode;
	pencode.len = ngx_base64_encoded_length(pcur.len);
	pencode.data = ngx_alloc(pencode.len + 1,r->pool->log);
	ngx_encode_base64(&pencode,&pcur);
	ngx_str_t output_format = ngx_string("%V");

	int bodylen = output_format.len + pencode.len - 2;
	r->headers_out.content_length_n = bodylen;

	ngx_log_stderr(0,"bodylen: %d",bodylen);
	//在内存池上分配内存保存将要发送的包体
	ngx_buf_t* b = ngx_create_temp_buf(r->pool, bodylen);
	ngx_snprintf(b->pos, bodylen, (char*)output_format.data,
			&pencode);
	b->last = b->pos + bodylen;
	b->last_buf = 1;

	ngx_log_stderr(0,"b->pos: %s",b->pos);

	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	static ngx_str_t type = ngx_string("text/plain; charset=GBK");
	r->headers_out.content_type = type;
	r->headers_out.status = NGX_HTTP_OK;

	r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;
	ngx_int_t ret=ngx_http_send_header(r);
	ret=ngx_http_output_filter(r, &out);

	ngx_gettimeofday(&myctx->end);
	ngx_int_t time = (myctx->end.tv_sec - myctx->start.tv_sec)*1000 
		+ (myctx->end.tv_usec - myctx->start.tv_usec)/1000;
	ngx_log_stderr(0,"time: %d",time);
	ngx_http_finalize_request(r, ret);
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
	ngx_gettimeofday(&myctx->start);

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
	ngx_int_t flag = read_file(r,filename,myctx->session,myctx->key);  //根据session，在文件中查找key

	if(flag == -1) //进入token
	{
		ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
		if (psr == NULL)
		{
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}

		//设置子请求回调方法为mytest_subrequest_post_handler
		psr->handler = mytest_subrequest_post_handler;

		//data设为myctx上下文，这样回调mytest_subrequest_post_handler
		//时传入的data参数就是myctx
		psr->data = myctx;

		ngx_str_t sub_prefix = ngx_string("/rest/index2");
		ngx_str_t sub_location;
		sub_location.len = sub_prefix.len;
		sub_location.data = ngx_palloc(r->pool, sub_location.len);
		ngx_snprintf(sub_location.data, sub_location.len,
				"%V", &sub_prefix);

		//sr就是子请求
		ngx_http_request_t *sr;
		ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);
		if (rc != NGX_OK)
		{
			return NGX_ERROR;
		}

		return NGX_DONE;
	}
	else
	{
		ngx_int_t ret = deaes_data(r,myctx->data,myctx->requesttime,myctx->key);
		if(ret == 0)
		{
			ngx_log_stderr(0,"真实服务器");
			ngx_http_post_subrequest_t *psr_key = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
			if (psr_key == NULL)
			{
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			//ret等于0说明 requesttime相等。
			psr_key->handler = mytest_subrequest_post_key_handler;

			psr_key->data = myctx;
			ngx_str_t sub_prefix = ngx_string("/rest/index");
			ngx_str_t sub_location;
			sub_location.len = sub_prefix.len;
			sub_location.data = ngx_palloc(r->pool, sub_location.len);
			ngx_snprintf(sub_location.data, sub_location.len,
					"%V", &sub_prefix);

			ngx_http_request_t *sr_key;
			ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr_key, psr_key, NGX_HTTP_SUBREQUEST_IN_MEMORY);
			if (rc != NGX_OK)
			{
				return NGX_ERROR;
			}
			return NGX_DONE;
		}
		else
		{
			return NGX_ERROR;
		}
	}
}
ngx_int_t deaes_data(ngx_http_request_t *r,u_char *data,u_char *requesttime,u_char *key)
{
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
	ngx_str_t pencode;   //编码数据
	ngx_str_t pdecode;   //解码数据
	u_char *c = ngx_alloc(4096,r->pool->log);
	ngx_memzero(c,4096);

	pencode.len = ngx_strlen(data);
	pencode.data = ngx_alloc(pencode.len + 1,r->pool->log);
	ngx_memzero(pencode.data,pencode.len+1);
	ngx_memcpy(pencode.data,data,pencode.len);
	ngx_log_stderr(0,"pencode: %s",pencode.data);

	pdecode.len = ngx_base64_decoded_length(pencode.len);
	pdecode.data = ngx_alloc(pdecode.len + 1,r->pool->log);
	ngx_memzero(pdecode.data,pdecode.len+1);
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
			ngx_memcpy(key,buf,len);
			ngx_log_stderr(0,"key : %s",key);
			return 0;
		}
		filelen += 2 * (off_t)len;
		lseek(file->fd,filelen,SEEK_SET);
	}
	return -1;
}


