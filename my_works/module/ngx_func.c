 ///
 /// @file    func.c
 /// @author  faith(kh_faith@qq.com)
 /// @date    2018-07-04 09:43:01
 ///
 
#include "ngx_func.h"
ngx_int_t query_string(ngx_http_request_t *r,u_char *match_name,ngx_str_t match_str)//解析uri
{
	ngx_log_stderr(0,"r->args: %s",r->args.data);
	ngx_uint_t i = 0, j = 0;
	for (; i <= r->args.len - match_str.len; i++)
	{
		if(ngx_strncmp(r->args.data + i, match_str.data,match_str.len) == 0)
		{
			ngx_log_stderr(0,"i :%i",i);
			if(*(r->args.data + i + match_str.len) == '=')
			{
				i = i + match_str.len + 1;
				ngx_log_stderr(0,"i :%i",i);
				for(j = i;j <= r->args.len;j++)
				{
					if(*(r->args.data + j) == '&' || j == r->args.len)
					{
						ngx_log_stderr(0,"j :%i",j);
						ngx_memcpy(match_name,(r->args.data)+i,j-i);
						ngx_log_stderr(0,"match_name : %s",match_name);
						return 0;;
					}
				}
			}
		}
	}
	ngx_log_stderr(0,"no %s",match_str.data);
	return -1;
}
//解密，并对比requesttime，可添加设备号的对比
ngx_int_t deaes_data(ngx_http_request_t *r,u_char *data,u_char *requesttime,u_char *key)
{
	ngx_str_t pencode;   //编码数据
	ngx_str_t pdecode;   //解码数据
	u_char *c = ngx_alloc(64,r->pool->log);
	ngx_memzero(c,64);

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
	ngx_int_t ret = deAes(c,clen,key);         //aes算法解密
	if(ret == -1)
	{
		return -1;
	}
	ngx_log_stderr(0,"c: %s",c);

	ngx_int_t i=0,j=0,k=0;
	u_char d[3][16] = {0};  
	u_char *temp = c;
	while(i < clen)             //对解密后的数据分段
	{
		if(c[i] == ' ')
		{
			ngx_memcpy(d[j],temp,i-k);
			temp = temp + (i-k+1);
			j++;
			k = i + 1;
		}
		if(j == 3)
			break;
		i++;
	}
	if(j < 3)
	{
		ngx_log_stderr(0,"args error");//数据段个数出错
		return -1;
	}
	ngx_log_stderr(0,"d[0]: %s",d+0);//请求时间
	ngx_log_stderr(0,"d[1]: %s",d+1);//设备号
	ngx_log_stderr(0,"d[2]: %s",d+2);//ip地址

	if(ngx_strcmp(requesttime,d[0]) == 0)  //对比请求时间
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
	file->fd = ngx_open_file(realfilename,NGX_FILE_RDWR,NGX_FILE_CREATE_OR_OPEN,0664);
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
	size_t len = 33; //32位随机字符串和空格
	u_char buf[33]={0};

	ngx_int_t server_len = 128;//token服务器向文件存放数据时，定义的每行长度
	off_t filelen = 0;//记录文件读取位置
	ssize_t n;
	while(filelen < size)
	{
		n = ngx_read_fd(file->fd,buf,len-1);//读32个字节，session
		ngx_log_stderr(0,"n: %d",n);
		ngx_log_stderr(0,"session : %s",buf);
		if(ngx_strcmp(buf,session) == 0)
		{	
			filelen += len;
			lseek(file->fd,filelen,SEEK_SET);//取key
			n = ngx_read_fd(file->fd,buf,len-1);
			ngx_log_stderr(0,"n: %d",n);
			ngx_memcpy(key,buf,len);
			ngx_log_stderr(0,"key : %s",key);
			return 0;
		}
		filelen += (off_t)server_len;//偏移一行
		lseek(file->fd,filelen,SEEK_SET);
	}
	return -1;//文件中不存在session
}
