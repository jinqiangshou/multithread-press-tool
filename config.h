#ifndef __CONFIG_H__
#define __CONFIG_H__

struct conf_info
{
    const char *name;
    void *object;
};

typedef struct conf_info Cconf_info;

//读取配置文件的内容
void parse(FILE *);

#endif //__CONFIG_H__
