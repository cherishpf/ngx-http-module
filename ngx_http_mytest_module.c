#include <ngx_config.h>
#include <nginx.h>
#include <ngx_http.h>

//���û��ʲô�����Ǳ�����HTTP��ܳ�ʼ��ʱ��ɵģ��ǾͲ���ʵ��ngx_http_module_t��8���ص�����
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

//�����û�����
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

//�����û�����
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
//�ڳ���mytest������ʱ��ngx_http_mytest�����ᱻ����
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
	ngx_str_t name; ����������
	ngx_uint_t type; ���������ͣ�type��ָ����������Գ��ֵ�λ�á����磬������server{}��location{}�У���
					 ��������Я���Ĳ�������
	char* (*set)(ngx_conf_t* cf, ngx_command_t* cmd, void* conf); ������name��ָ����������󣬵��ø÷���
																  ����������Ĳ���
	ngx_uint_t conf; �����ļ��е�ƫ����
	ngx_uint_t offset; ͨ������ʹ��Ԥ��Ľ�����������������
	void *post; �������ȡ��Ĵ�������������ngx_conf_post_t�ṹָ��
};
#define ngx_null_command {ngx_null_string, 0, NULL, 0, 0, NULL}
*/
//����mytest������Ĵ���
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

//����mytestģ��
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
