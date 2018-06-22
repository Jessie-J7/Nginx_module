#include <ngx_file.h>

void ngx_pool_cleanup_file(void *data)
{
	ngx_pool_cleanup_file_t *c = data;
}

ngx_pool_cleanup_t *cln;
ngx_pool_cleanup_file_t *clfn;
ngx_file_t *file;

file = ngx_palloc(r->pool,sizeof(ngx_file_t));
if(file == NULL){
	return NGX_ERROR;
}

char *realfilename = "/home/faith/module/session/input.info";

file->fd = ngx_open_file(realfilename,NGX_FILE_RDONLY,NGX_FILE_OPEN,0);
file->name.len = ngx_strlen(realfilename);
file->name.data = realfilename;
file->log = r->pool->log;

cln = ngx_pool_cleanup_add(r->pool,sizeof(ngx_pool_cleanup_file_t));
if(cln == NULL){
	return NGX_ERROR;
}

clfn = cln->data;
clfn->fd = file->fd;
clfn->name = realfilename;
clfn->log = r->pool->log;
cln->handler = ngx_pool_cleanup_file;
