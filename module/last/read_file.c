void read_file(u_char *realfilename,ngx_str_t session,ngx_str_t *key){
	ngx_file_t *file;
	ngx_file_info_t fi;

	file = ngx_palloc(r->pool,sizeof(ngx_file_t));
	if(file == NULL){
		return NGX_ERROR;
	}

	file->log = r->pool->log;
	file->fd = ngx_open_file(realfilename,NGX_FILE_RDONLY,NGX_FILE_OPEN,0);
	if(file->fd < 0){
		ngx_log_error(NGX_LOG_CRIT,r->pool->log,ngx_errno,ngx_open_file_n " \"%s\" failed",realfilename);
		return NGX_HTTP_NOT_FOUND;
	}
	ngx_log_stderr(0,"file->fd : %d",file->fd);
	if(ngx_fd_info(fd,&fi) == NGX_FILE_ERROR)
	{
		ngx_log_error(NGX_LOG_ALERT,r->pool->log,ngx_errno,ngx_fd_info_n " \"%s\" failed",realfilename);
		return NGX_HTTP_NOT_FOUND;
	}

	off_t size = ngx_file_size(&fi);
	size_t len = 33;
	u_char *buf = ngx_alloc(len,r->pool->log);
	ngx_memzero(buf,len);

	off_t filelen = 0;
	while(filelen < size)
	{
		ngx_read_file(file,buf,len-1,filelen);
		ngx_log_stderr(0,"buf : %s",buf);
		if(ngx_strcmp(buf,session.data) == 0)
		{	
			ngx_read_file(file,buf,len-1,SEEK_CUR);
			ngx_log_stderr(0,"buf : %s",buf);
			ngx_memcpy(key,buf,len);
			break;
		}
		lseek(file->fd,(off_t)len,SEEK_CUR);
		filelen += (off_t)len;
	}
}
