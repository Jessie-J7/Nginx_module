/*
 *  * Copyright (C) yejianfeng
 *   */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_func.h"
#include "cJSON.h"
#include "ngx_util.h"

static ngx_command_t ngx_http_mytest_commands[] = {
    { 
	  ngx_string("filename"),
      NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
	  NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_mytest_loc_conf_t,filename),
      NULL
	},
	{
		ngx_string("mytest"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
		ngx_http_mytest,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
    { 
	  ngx_string("test_slab"),
      NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
      ngx_http_mytest_createmem,
      0,
      0,
      NULL
	},
	ngx_null_command
};

static ngx_http_module_t ngx_http_mytest_module_ctx = {
	NULL,
    ngx_http_mytest_init,
    ngx_http_mytest_create_main_conf,
	NULL,
	NULL,
	NULL,
	ngx_http_mytest_create_loc_conf,
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

ngx_int_t ngx_http_mytest_shm_init(ngx_shm_zone_t* shm_zone, void* data)
{
	ngx_http_mytest_conf_t* conf =(ngx_http_mytest_conf_t*)shm_zone->data;
    ngx_http_mytest_conf_t* oconf = (ngx_http_mytest_conf_t*)data;
    //判断是否为reload后导致的初始化共享内存
	if (oconf) {
        conf->sh = oconf->sh;
        conf->shpool = oconf->shpool;
        return NGX_OK;
    }

    conf->shpool = (ngx_slab_pool_t*)shm_zone->shm.addr;
    conf->sh = (ngx_http_mytest_shm_t*) ngx_slab_alloc(conf->shpool, sizeof(ngx_http_mytest_shm_t));
    if (conf->sh == NULL) {
        return NGX_ERROR;
    }
    conf->shpool->data = conf->sh;

	ngx_rbtree_init(&conf->sh->rbtree,&conf->sh->sentinel,ngx_http_mytest_rbtree_insert_value);
    ngx_queue_init(&conf->sh->queue);

    size_t len = sizeof(" in mytest \"\"") + shm_zone->shm.name.len;
    conf->shpool->log_ctx = (u_char*)ngx_slab_alloc(conf->shpool, len);
    if (conf->shpool->log_ctx == NULL) {
        return NGX_ERROR;
    }
    ngx_sprintf(conf->shpool->log_ctx, " in mytest \"%V\"%Z", &shm_zone->shm.name);
    return NGX_OK;
}

char * ngx_http_mytest_createmem(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) 
{
    ngx_http_mytest_conf_t* mconf = (ngx_http_mytest_conf_t*)conf;
    ngx_str_t name = ngx_string("test_slab_shm");
    ngx_str_t* value = (ngx_str_t*)cf->args->elts;

    mconf->shmsize = ngx_parse_size(&value[1]);
    if (mconf->shmsize == (ssize_t)NGX_ERROR || mconf->shmsize == 0) 
	{
        return (char*)NGX_ERROR;
    }

    ngx_shm_zone_t* shm_zone = ngx_shared_memory_add(cf, &name, mconf->shmsize,&ngx_http_mytest_module);
    if (shm_zone == NULL)
	{
        return (char*)NGX_CONF_ERROR;
    }

    shm_zone->init = ngx_http_mytest_shm_init;
    shm_zone->data = mconf;
    return NGX_CONF_OK;
}

ngx_int_t ngx_http_mytest_init(ngx_conf_t* cf)
{
    ngx_http_core_main_conf_t* cmcf = (ngx_http_core_main_conf_t*)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    
	ngx_http_handler_pt* h = (ngx_http_handler_pt*)ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL)
	{
        return NGX_ERROR;
    }

    *h = ngx_http_mytest_handler;
    return NGX_OK;
}

void* ngx_http_mytest_create_main_conf(ngx_conf_t* cf) {
    ngx_http_mytest_conf_t* mycf;

    mycf = (ngx_http_mytest_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_mytest_conf_t));
    if (mycf == NULL) {
        return NULL;
    }

    mycf->shmsize = -1;
    return mycf;
}
void* ngx_http_mytest_create_loc_conf(ngx_conf_t* cf) {
    ngx_http_mytest_loc_conf_t* mycf;

    mycf = (ngx_http_mytest_loc_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_mytest_loc_conf_t));
    if (mycf == NULL) {
        return NULL;
    }

	mycf->filename.len = 0;
	mycf->filename.data = NULL;
    return mycf;
}

char * ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t *clcf;

	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

	clcf->handler = ngx_http_mytest_handler;

	return NGX_CONF_OK;
}

//不能在handler中创建两个子请求，因为子请求是异步执行，不管token有没有验证成功，都会反向代理到真实服务器
ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)//原始请求
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
	ngx_time_t *tp = ngx_timeofday();//起始时间
	myctx->start = (ngx_msec_t)(tp->sec *1000 + tp->msec);

	myctx->session = ngx_palloc(r->pool,33);
	ngx_memzero(myctx->session,33);
	ngx_str_t str1 = ngx_string("session");
	ngx_int_t ret = query_string(r,myctx->session,str1);         //获取参数中的session
	if(ret == -1)
		return NGX_ERROR;
	myctx->data = ngx_palloc(r->pool,64);
	ngx_memzero(myctx->data,64);
	ngx_str_t str2 = ngx_string("data");
	ret = query_string(r,myctx->data,str2);           //获取data
	if(ret == -1)
		return NGX_ERROR;
	myctx->requesttime = ngx_palloc(r->pool,16);
	ngx_memzero(myctx->requesttime,16);
	ngx_str_t str3 = ngx_string("requesttime");
	ret = query_string(r,myctx->requesttime,str3);	 //获取requesttime
	if(ret == -1)
		return NGX_ERROR;

    ngx_http_mytest_conf_t* conf = (ngx_http_mytest_conf_t*)ngx_http_get_module_main_conf(r, ngx_http_mytest_module);
    ngx_http_mytest_loc_conf_t* cmlf = (ngx_http_mytest_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_mytest_module);

	size_t len = ngx_strlen(myctx->session); 
    uint32_t hash = ngx_crc32_short(myctx->session,len);
	ngx_log_stderr(0,"data_args: %s",myctx->session);

    ngx_shmtx_lock(&conf->shpool->mutex);
    ngx_int_t rc = ngx_http_mytest_lookup(r, conf, hash, myctx->session,myctx->key,len);
    ngx_shmtx_unlock(&conf->shpool->mutex);
	if(rc != NGX_DECLINED)
	{
		return NGX_ERROR;
	}
	if(ngx_strlen(myctx->key) == 0)//缓存没有
	{
		//u_char filename[] = {"/home/faith/ngx_test"};
		u_char *filename = ngx_palloc(r->pool,cmlf->filename.len);
		ngx_memzero(filename,cmlf->filename.len);
		ngx_memcpy(filename,cmlf->filename.data,cmlf->filename.len);
		ngx_log_stderr(0,"filename: %s",filename);
		myctx->filename = ngx_palloc(r->pool,ngx_strlen(filename));
		ngx_memzero(myctx->filename,ngx_strlen(filename));
		ngx_memcpy(myctx->filename,filename,ngx_strlen(filename));
		ngx_log_stderr(0,"myctx->filename: %s",myctx->filename);
		ret = read_file(r,filename,myctx->session,myctx->key);  //根据session，在文件中查找key
		len = ngx_strlen(myctx->session); 
		size_t key_len = ngx_strlen(myctx->key); 
	    hash = ngx_crc32_short(myctx->session, len);
		ngx_log_stderr(0,"data_local: %s",myctx->session);
	
		if(ngx_strlen(myctx->key) != 0)//文件有
		{
		    ngx_shmtx_lock(&conf->shpool->mutex);
		    ngx_int_t rc = ngx_http_mytest_insert(r, conf, hash, myctx->session,myctx->key,len,key_len);
		    ngx_shmtx_unlock(&conf->shpool->mutex);
			if(rc != NGX_DECLINED)
			{
				return NGX_ERROR;
			}
			ngx_log_stderr(0,"key: %s",myctx->key);
		}
	}
	if(ret == -1) //文件没有，进入token
	{
		ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
		if (psr == NULL)
		{
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}

		//设置访问token服务器的子请求回调方法为mytest_subrequest_post_handler
		psr->handler = mytest_subrequest_post_handler;

		psr->data = myctx;
		ngx_str_t sub_prefix = ngx_string("/rest/index2");
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
	else //在文件中找到key
	{
		ngx_int_t ret = deaes_data(r,myctx->data,myctx->requesttime,myctx->key);//解密data，并对比请求时间
		if(ret == 0)
		{
			ngx_log_stderr(0,"真实服务器");
			ngx_http_post_subrequest_t *psr_real = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
			if (psr_real == NULL)
			{
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			psr_real->handler = mytest_subrequest_post_real_handler;

			psr_real->data = myctx;
			ngx_str_t sub_prefix = ngx_string("/rest/index");
			ngx_str_t sub_location;
			sub_location.len = sub_prefix.len;
			sub_location.data = ngx_palloc(r->pool, sub_location.len);
			ngx_snprintf(sub_location.data, sub_location.len,"%V", &sub_prefix);

			ngx_http_request_t *sr_real;
			ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr_real, psr_real, NGX_HTTP_SUBREQUEST_IN_MEMORY);
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

//token服务器子请求
ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r,void *data, ngx_int_t rc)
{
	//获取上下文和配置
	ngx_http_request_t          *pr = r->parent;
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(pr, ngx_http_mytest_module);

	pr->headers_out.status = r->headers_out.status;
	if (r->headers_out.status == NGX_HTTP_OK)//解析json，取key
	{
		ngx_log_stderr(0,"data: %s",myctx->data);
	}
	//取出上游服务器的全部缓冲内容
	ngx_buf_t* pRecvBuf = &r->upstream->buffer;
	
	ngx_str_t cjson;
	cjson.len = pRecvBuf->last - pRecvBuf->pos;
	cjson.data = pRecvBuf->pos;
	
	//解析json，应该包含session,key,设备号,时间戳，设备号等其他参数也保存在进程上下文中
	ngx_int_t ret = cJSON_to_array((char *)cjson.data,myctx->key,myctx->session,r);
	if(ret == -1)
	{
		ngx_log_stderr(0,"json error");
		return NGX_ERROR;
	}

	//父请求的回调函数
	pr->write_event_handler = (void*)mytest_post_handler;

	return NGX_OK;
}
//代理token服务器的父请求
ngx_int_t mytest_post_handler(ngx_http_request_t * r)
{
	if (r->headers_out.status != NGX_HTTP_OK)
	{
		ngx_http_finalize_request(r, r->headers_out.status);
		return NGX_ERROR;
	}

	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
    ngx_http_mytest_conf_t* conf = (ngx_http_mytest_conf_t*)ngx_http_get_module_main_conf(r, ngx_http_mytest_module);

	size_t len = ngx_strlen(myctx->session); 
	size_t key_len = ngx_strlen(myctx->key); 
    uint32_t hash = ngx_crc32_short(myctx->session, len);
	ngx_log_stderr(0,"data_token: %s",myctx->session);

    ngx_shmtx_lock(&conf->shpool->mutex);
    ngx_int_t rc = ngx_http_mytest_insert(r, conf, hash, myctx->session,myctx->key,len,key_len);
    ngx_shmtx_unlock(&conf->shpool->mutex);
	if(rc != NGX_DECLINED)
	{
		return NGX_ERROR;
	}

	//写入文件末尾
	ngx_log_stderr(0,"myctx->filename: %s",myctx->filename);
	ngx_file_t *file;
	file = ngx_palloc(r->pool,sizeof(ngx_file_t));
	file->fd = ngx_open_file(myctx->filename,NGX_FILE_RDWR,NGX_FILE_CREATE_OR_OPEN,0664);
	if(file->fd < 0){
		ngx_log_stderr(0,"%V failed",myctx->filename);
		return NGX_ERROR;
	}
	//固定长度写入文件，不足补空格，方便文件的读取，方便加入设备号等其他参数
	ngx_int_t server_len = 128;
	ngx_log_stderr(0,"file->fd : %d",file->fd);
	u_char * buf = ngx_palloc(r->pool,server_len);
	ngx_memset(buf,' ',server_len);
	ngx_snprintf(buf,server_len,"%s %s",myctx->session,myctx->key);
	buf[127] = '\n';
	ngx_log_stderr(0,"buf : %s",buf);
	lseek(file->fd,0,SEEK_END);
	ngx_write_fd(file->fd,buf,server_len);

	//解密并代理真实服务器
	ngx_int_t ret = deaes_data(r,myctx->data,myctx->requesttime,myctx->key);
	if(ret == 0)
	{
		ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
		if (psr == NULL)
		{
			return NGX_ERROR;
		}
		psr->handler = mytest_subrequest_post_real_handler;

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

//真实服务器的子请求
ngx_int_t mytest_subrequest_post_real_handler(ngx_http_request_t *r,void *data, ngx_int_t rc)
{
	ngx_http_request_t          *pr = r->parent;
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(pr, ngx_http_mytest_module);

	pr->headers_out.status = r->headers_out.status;
	if (r->headers_out.status == NGX_HTTP_OK)
	{
		ngx_log_stderr(0,"key: %s",myctx->key);
	}
	//取数据server_data，放入ctx
	ngx_buf_t* pRecvBuf = &r->upstream->buffer;

	ngx_str_t cjson;
	cjson.len = pRecvBuf->last - pRecvBuf->pos;
	cjson.data = pRecvBuf->pos;

	myctx->ser_data = ngx_palloc(r->pool,1024);
	ngx_memzero(myctx->ser_data,1024);

	ngx_int_t ret = cJSON_to_str((char *)cjson.data,myctx->ser_data,r);
	if(ret == -1)
	{
		ngx_log_stderr(0,"json error");
		return NGX_ERROR;
	}
	ngx_log_stderr(0,"myctx->ser_data : %s",myctx->ser_data);
	pr->write_event_handler = (void *)mytest_post_real_handler;

	return NGX_OK;
}

//代理真实服务器的父请求
ngx_int_t mytest_post_real_handler(ngx_http_request_t * r)
{
	if (r->headers_out.status != NGX_HTTP_OK)
	{
		ngx_log_stderr(0,"finalize..");
		ngx_http_finalize_request(r, r->headers_out.status);
		return NGX_ERROR;
	}	
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
	u_char *p = ngx_palloc(r->pool,ngx_strlen(myctx->ser_data)+1);
	ngx_memzero(p,ngx_strlen(myctx->ser_data)+1);
	ngx_memcpy(p,myctx->ser_data,ngx_strlen(myctx->ser_data));
	ngx_log_stderr(0,"p: %s",p);
	ngx_int_t plen = ngx_strlen(p);
	//不是16的倍数，需要空格填充
	if(plen == 0){
		ngx_log_stderr(0,"明文字符长度不为空！\n");
		return NGX_ERROR;
	}
	ngx_int_t i;
	ngx_int_t pstrlen = plen;
	if(pstrlen % 16 != 0) {
		plen = (plen / 16 + 1) * 16;
		for(i=pstrlen;i<plen;i++)
			p[i] = ' ';
	}
	aes(p,plen,myctx->key);//加密
	ngx_str_t pcur; 
	pcur.len = plen;
	pcur.data = ngx_palloc(r->pool,pcur.len);
	ngx_memcpy(pcur.data,p,pcur.len);

	ngx_str_t pencode;//base64编码
	pencode.len = ngx_base64_encoded_length(pcur.len);
	pencode.data = ngx_palloc(r->pool,pencode.len);
	ngx_encode_base64(&pencode,&pcur);
	
	//将要发送的包体
	ngx_str_t output_format = ngx_string("%V");
	ngx_int_t bodylen = output_format.len + pencode.len - 2;
	r->headers_out.content_length_n = bodylen;

	ngx_log_stderr(0,"bodylen: %d",bodylen);
	ngx_buf_t* b = ngx_create_temp_buf(r->pool, bodylen);
	ngx_snprintf(b->pos, bodylen, (char*)output_format.data,&pencode);
	b->last = b->pos + bodylen;
	b->last_buf = 1;

	ngx_log_stderr(0,"b->pos: %s",b->pos);

	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;

	ngx_time_t *tp = ngx_timeofday();//起始时间
	myctx->end = (ngx_msec_t)(tp->sec *1000 + tp->msec);
	ngx_msec_t time = myctx->end - myctx->start;
	ngx_log_stderr(0,"time: %d",time);
	
	static ngx_str_t type = ngx_string("text/plain; charset=GBK");
	r->headers_out.content_type = type;
	r->headers_out.status = NGX_HTTP_OK;

	r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;
	ngx_int_t ret = ngx_http_send_header(r);	
	ret = ngx_http_output_filter(r, &out);
	
	ngx_http_finalize_request(r, ret);

	return 0;
}

