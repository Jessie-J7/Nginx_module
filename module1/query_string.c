
void query_string(ngx_http_request_t *r,ngx_str_t *match_name,ngx_str_t match_str)
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
						ngx_memcpy(match_name->data,r->args.data+i,match_name->len);
						ngx_log_stderr(0,"%s : %s",match_str.data,match_name->data);
						break;
					}
				}
				break;
			}
		}
	}
}
