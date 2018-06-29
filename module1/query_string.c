#include "func.h"
void query_string(ngx_http_request_t *r,u_char *match_name,ngx_str_t match_str)
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
						break;
					}
				}
				break;
			}
		}
	}
}
