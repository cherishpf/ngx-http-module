extern "C" {
	#include <ngx_config.h>
	#include <nginx.h>
	#include <ngx_http.h>
	//#include <ExtractionShellFace.h>
	static ngx_int_t ngx_http_shellface_handler(ngx_http_request_t* r);
	static char* ngx_http_shellface(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
}

//模块上下文, 如果没有什么工作是必须在HTTP框架初始化时完成的，那就不必实现ngx_http_module_t的8个回调方法
static ngx_http_module_t ngx_http_shellface_module_ctx = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

//实际完成处理的回调函数, 处理用户请求
//r 是nginx已经处理完了的http请求头
static ngx_int_t ngx_http_shellface_handler(ngx_http_request_t* r)
{
	if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))){
		//非法请求方式 状态码 405
		return NGX_HTTP_NOT_ALLOWED;
	}
	//丢弃客户端发送来的HTTP包体内容
	ngx_int_t rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK){
		return rc;
	}
	//设置返回的Content-Type ngx_string是一个宏可以初始化data字段和len字段
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response = ngx_string("Hello World!");
//	ngx_str_t response = ngx_string("Hello World!" + ExtractionShellFace("demo", "test.xdb"));
  //响应包体内容和状态码设置
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;
	//发送响应头
	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
		return rc;
	}
	//构造ngx_buf_t结构体准备发送报文
	ngx_buf_t *b;
	//r->pool内存池
	b = (ngx_buf_t *)ngx_create_temp_buf(r->pool, response.len);
	if(b == NULL){
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	//拷贝响应报文
	ngx_memcpy(b->pos, response.data, response.len);
	//必须设置好last指针，如果last和pos相等，是不会发送的
	b->last = b->pos + response.len;
	//声明这是最后一块缓冲区
	b->last_buf = 1;
	//构造发送时的ngx_chain_t结构体
	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	//发送响应，结束后HTTP框架会调用ngx_http_finalize_request方法结束请求
	return ngx_http_output_filter(r, &out);
}

//在出现shellface配置项时，ngx_http_shellface方法会被调用
static char* ngx_http_shellface(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	if(cf->args->nelts < 2){
		return (char*)NGX_CONF_ERROR;
	}
	ngx_http_core_loc_conf_t* clcf;
	//找到shellface所属配置块
	clcf = (ngx_http_core_loc_conf_t *)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	ngx_str_t* str = (ngx_str_t*)cf->args->elts;
	if(ngx_strncmp(str[1].data, "helloworld", str[1].len) == 0)
		//在NGX_HTTP_CONTENT_PHASE阶段会调用此回调函数
		clcf->handler = ngx_http_shellface_handler;
	else
		return (char*)NGX_CONF_ERROR;
	return (char*)NGX_CONF_OK;
} 

//定义shellface配置项的处理
static ngx_command_t ngx_http_shellface_commands[] = {
	{
		//配置项名称
		ngx_string("shellface"),
		//指令在main/server/location/limit_except配置部分出现是合法的，允许读入1个参数
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
		//set 函数, 当某个配置块中出现shellface时，就会回调此函数
		ngx_http_shellface,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	//空的ngx_command_t用于表示数组结束
	ngx_null_command
};

//定义shellface模块
ngx_module_t ngx_http_shellface_module = {
	NGX_MODULE_V1,
	//ctx,对于HTTP模块来说，ctx必须是ngx_http_module_t接口
	&ngx_http_shellface_module_ctx,
	//commands
	ngx_http_shellface_commands,
	//定义http模块时，必须设置成NGX_HTTP_MODULE
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
