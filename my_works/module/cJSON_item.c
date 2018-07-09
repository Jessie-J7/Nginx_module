 ///
 /// @file    cJSON_item.c
 /// @author  faith(kh_faith@qq.com)
 /// @date    2018-07-04 09:45:52
 ///

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "cJSON.h"
//解析token服务器的json
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
							return 0;
						}
						else
							return -1;
					}
				}
				else
					return -1;
			}
		}
		else
			return -1;
	}
	cJSON_Delete(json);
	return -1;
}
//解析真实服务器的json
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
			return 0;
		}
		else
			return -1;
	}
	cJSON_Delete(root);
	return -1;
}
