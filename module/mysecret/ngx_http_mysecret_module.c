
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_mysecret_handler(ngx_http_request_t *r)
{
	if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {
		return NGX_HTTP_NOT_ALLOWED;
	}

	ngx_int_t rc = ngx_http_discard_request_body(r);
	if (rc != NGX_OK) {
		return rc;
	}

	ngx_file_t *file;

	size_t len = 1024;
	ssize_t n;

	file = ngx_palloc(r->pool,sizeof(ngx_file_t));
	if(file == NULL){
		return NGX_ERROR;
	}

	u_char *realfilename = ngx_alloc(len,r->pool->log);;
	ngx_str_t str = ngx_string("/home/faith/ngx_test");
	ngx_memcpy(realfilename,str.data,str.len);
	ngx_log_stderr(0,"realfilename : %s",realfilename);

	file->log = r->pool->log;
	file->fd = ngx_open_file(realfilename,NGX_FILE_RDWR|S_IXUSR|NGX_FILE_NONBLOCK,NGX_FILE_CREATE_OR_OPEN,NGX_FILE_DEFAULT_ACCESS);
	if(file->fd < 0){
		ngx_log_error(NGX_LOG_CRIT,r->pool->log,ngx_errno,ngx_open_file_n " \"%s\" failed",realfilename);
		return NGX_HTTP_NOT_FOUND;
	}
	ngx_log_stderr(0,"file->fd : %d",file->fd);

	u_char *buf = ngx_alloc(len,r->pool->log);
	ngx_memzero(buf,len);
	ngx_str_t buffer = ngx_string("qwer1234");
	ngx_memcpy(buf,buffer.data,buffer.len);
	ngx_log_stderr(0,"buf : %s",buf);

	ngx_write_file(file,buf,ngx_strlen(buf),0);
	ngx_memzero(buf,len);

	n = ngx_read_file(file,buf,len,0);
	ngx_log_stderr(0,"n : %d",n);
	ngx_log_stderr(0,"buf : %s",buf);
	
	//返回数据
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response;
	response.data = ngx_alloc(len,r->pool->log);
	ngx_memzero(response.data,len);
	if(0 != n)
	{
		response.len = n;
		ngx_memcpy(response.data,buf,response.len);
	}
	else
	{
		ngx_str_set(&response,"buf is empty");
	}

	ngx_log_stderr(0,"response : %s",response.data);
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
ngx_http_mysecret(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t  *clcf;

	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

	clcf->handler = ngx_http_mysecret_handler;

	return NGX_CONF_OK;
}

static ngx_command_t ngx_http_mysecret_commands[] = {
	{
		ngx_string("check"),
		NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
		ngx_http_mysecret,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL,
	},

	ngx_null_command
};

// 定义上下文, 只会在location中出现，所以都为null
static ngx_http_module_t ngx_http_mysecret_module_ctx = {
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
ngx_module_t ngx_http_mysecret_module = {
	NGX_MODULE_V1,
	&ngx_http_mysecret_module_ctx,
	ngx_http_mysecret_commands,
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


