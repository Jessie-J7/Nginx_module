#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

void query(ngx_http_request_t *r,ngx_str_t *match_name,ngx_str_t match_str)
{
	ngx_log_stderr(0,"r->args: %s",r->args.data);
	ngx_uint_t i = 0, j = 0;
	for (; i <= r->args.len - match_str.len; i++)
	{
		if(ngx_strncmp(r->args.data + i, match_str.data,match_str.len) == 0)
		{
			ngx_log_stderr(0,"i :%i",i);
			ngx_log_stderr(0,"len :%d",r->args.len);
			if(*(r->args.data + i + match_str.len) == '=')
			{
				ngx_log_stderr(0,"=");
				i = i + match_str.len + 1;
				ngx_log_stderr(0,"i :%i",i);
				for(j = i;j <= r->args.len;j++)
				{
					if(*(r->args.data + j) == '&' || j == r->args.len)
					{
						ngx_log_stderr(0,"j :%i",j);
						match_name->len = j-i;
						ngx_memcpy(match_name->data,(r->args.data) + i,match_name->len);
						ngx_log_stderr(0,"session %s",match_name->data);
						break;
					}
				}
				break;
			}
		}
	}
}

static ngx_int_t ngx_http_myargs_handler(ngx_http_request_t *r)
{
	if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {
		return NGX_HTTP_NOT_ALLOWED;
	}

	ngx_int_t rc = ngx_http_discard_request_body(r);
	if (rc != NGX_OK) {
		return rc;
	}

	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response;

	ngx_str_t session;
	ngx_str_t buf1 = ngx_string("12345");
	ngx_str_t str1 = ngx_string("session");
	query(r,&session,str1);
	ngx_str_t data;
	ngx_str_t buf2 = ngx_string("qwer");
	ngx_str_t str2 = ngx_string("data");
	query(r,&data,str2);
	ngx_str_t requesttime;
	ngx_str_t buf3 = ngx_string("1212");
	ngx_str_t str3 = ngx_string("requesttime");
	query(r,&requesttime,str3);	


	if((buf1.len == session.len)
		&&(0 == ngx_strcmp(buf1.data,session.data))
		&&(buf2.len == data.len)
		&&(0 == ngx_strcmp(buf2.data,data.data))
		&&(buf3.len == requesttime.len)
		&&(0 == ngx_strcmp(buf3.data,requesttime.data))
	  )
	{
		ngx_str_set(&response,"success");
	}
	else
	{
		ngx_str_set(&response,"failed");
	}

	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;

	rc = ngx_http_send_header(r);
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

	static char * 
ngx_http_myargs(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t  *clcf;

	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

	clcf->handler = ngx_http_myargs_handler;

	return NGX_CONF_OK;
}

static ngx_command_t ngx_http_myargs_commands[] = {
	{
		ngx_string("check"),
		NGX_HTTP_MAIN_CONF |NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF |NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
		ngx_http_myargs,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL,
	},

	ngx_null_command
};

// 定义上下文, 只会在location中出现，所以都为null
static ngx_http_module_t ngx_http_myargs_module_ctx = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// 定义模块
ngx_module_t ngx_http_myargs_module = {
	NGX_MODULE_V1,
	&ngx_http_myargs_module_ctx,
	ngx_http_myargs_commands,
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


