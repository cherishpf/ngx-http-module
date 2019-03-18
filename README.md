# ngx-http-module
nginx http dynamic module, c and c++ demo

使用Openresty编译自定义的nginx模块
1、c语言开发的模块
/data/mymodule/ngx_http_mytest_module.c

cd /data/software/openresty-1.13.6.1/
./configure --prefix=/data/software/openresty-1.13.6.1 --add-dynamic-module=/data/mymodule/
gmake 
gmake install

配置nginx.conf
location /test {
    mytest helloworld;
}

location /test2 {
    mytest sendfile;
}

2、C++开发的模块
/data/mymodule/ngx_http_shellface_module.cpp

cd /data/software/openresty-1.13.6.1/
./configure --prefix=/data/software/openresty-1.13.6.1 --add-dynamic-module=/data/mymodule/
编译方式修改(最好不要修改configure文件，所以修改Makefile文件):
vim /data/software/openresty-1.13.6.1/build/nginx-1.13.6/objs/Makefile
#新增g++编译器，前提是已经在系统安装g++
CXX = g++
#修改连接器为g++
LINK = $(CXX)
#修改自己的模块(注意仅修改自己的模块相关的)的编译方式为g++（把原来的$(CC)改为$(CXX)）
objs/addon/mymodule/ngx_http_shellface_module.o:   $(ADDON_DEPS) \
    src/mymodule/ngx_http_shellface_module.cpp
    $(CXX) -c $(CFLAGS)  $(ALL_INCS) \
        -o objs/addon/mymodule/ngx_http_shellface_module.o \
        src/mymodule/ngx_http_shellface_module.cpp

修改完成后再执行：
gmake 
gmake install        

配置nginx.conf
location /test {
    shellface helloworld;
}
