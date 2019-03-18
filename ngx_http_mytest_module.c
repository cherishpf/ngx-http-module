#include <ngx_config.h>
#include <nginx.h>
#include <ngx_http.h>

//如果没有什么工作是必须在HTTP框架初始化时完成的，那就不必实现ngx_http_module_t的8个回调方法
static ngx_http_module_t ngx_http_mytest_module_ctx = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

//处理用户请求
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t* r)
{
	if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))){
		return NGX_HTTP_NOT_ALLOWED;
	}
	ngx_int_t rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK){
		return rc;
	}
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response = ngx_string("Hello World!");
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;
	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
		return rc;
	}
	ngx_buf_t *b;
	b = ngx_create_temp_buf(r->pool, response.len);
	if(b == NULL){
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

//处理用户请求
static ngx_int_t ngx_http_mytest_handler2(ngx_http_request_t* r){
	if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))){
		return NGX_HTTP_NOT_ALLOWED;
	}
	ngx_int_t rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK){
		return rc;
	}
	ngx_buf_t *b;
	b = ngx_palloc(r->pool, sizeof(ngx_buf_t));
	u_char* filename = (u_char*)"/var/www/html/index.html";
	b->in_file = 1;
	b->file = ngx_palloc(r->pool, sizeof(ngx_file_t));
	b->file->fd = ngx_open_file(filename, NGX_FILE_RDONLY | NGX_FILE_NONBLOCK, NGX_FILE_OPEN, 0);
	b->file->log = r->connection->log;
	b->file->name.data = filename;
	b->file->name.len = sizeof(filename) - 1;
	if(b->file->fd <= 0){
		return NGX_HTTP_NOT_FOUND;
	}
	if(ngx_file_info(filename, &b->file->info) == NGX_FILE_ERROR){
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}	
	b->file_pos = 0;
	b->file_last = b->file->info.st_size;
	
	ngx_str_t type = ngx_string("text/html");
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = b->file->info.st_size;
	r->headers_out.content_type = type;
	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
		return rc;
	}
	
	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	return ngx_http_output_filter(r, &out);
}

/*
struct ngx_conf_s  {
    char                   *name;
    ngx_array_t          *args;
    ngx_cycle_t          *cycle;
    ngx_pool_t           *pool;
    ngx_pool_t           *temp_pool;
    ngx_conf_file_t     *conf_file;
    ngx_log_t            *log;
    void                   *ctx;
    ngx_uint_t           module_type;
    ngx_uint_t            cmd_type;
    ngx_conf_handler_pt      handler;
    char                   *handler_conf;
};
typedef struct ngx_array_s       ngx_array_t;
struct ngx_array_s {
    void        *elts;
    ngx_uint_t   nelts;
    size_t       size;
    ngx_uint_t   nalloc;
    ngx_pool_t  *pool;
};

#define ngx_http_conf_get_module_loc_conf(cf, module) \
    ((ngx_http_conf_ctx_t *) cf->ctx)->loc_conf[module.ctx_index]
*/
//在出现mytest配置项时，ngx_http_mytest方法会被调用
static char* ngx_http_mytest(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	if(cf->args->nelts < 2){
		return NGX_CONF_ERROR;
	}
	ngx_http_core_loc_conf_t* clcf;
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	ngx_str_t* str = cf->args->elts;
	if(ngx_strncmp(str[1].data, "helloworld", str[1].len) == 0)
		clcf->handler = ngx_http_mytest_handler;
	else if(ngx_strncmp(str[1].data, "sendfile", str[1].len) == 0)
		clcf->handler = ngx_http_mytest_handler2;
	else
		return NGX_CONF_ERROR;
	return NGX_CONF_OK;
} 
/*
typedef struct ngx_command_s ngx_command_t;
struct ngx_command_s{
	ngx_str_t name; 配置项名称
	ngx_uint_t type; 配置项类型，type将指定配置项可以出现的位置。例如，出现在server{}或location{}中，以
					 及它可以携带的参数个数
	char* (*set)(ngx_conf_t* cf, ngx_command_t* cmd, void* conf); 出现了name中指定的配置项后，调用该方法
																  处理配置项的参数
	ngx_uint_t conf; 配置文件中的偏移量
	ngx_uint_t offset; 通常用于使用预设的解析方法解析配置项
	void *post; 配置项读取后的处理方法，必须是ngx_conf_post_t结构指针
};
#define ngx_null_command {ngx_null_string, 0, NULL, 0, 0, NULL}
*/
//定义mytest配置项的处理
static ngx_command_t ngx_http_mytest_commands[] = {
	{
		ngx_string("mytest"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
		ngx_http_mytest,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL
	},
	ngx_null_command
};

//定义mytest模块
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
