#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

extern struct conf_info clist[1000];

//去掉字符串左右两侧的空格、tab、换行等符号。
void trim(char *s)
{
	char *c = s + strlen(s) - 1;
	while (isspace(*c) && c > s) {
		*c = '\0';
		--c;
	}
	if(c == s && isspace(*c))
		*c = '\0';
	return;
}

//在clist数组中查找name属性是否有等于char *c的，有的话将该位置的结构体返回
struct conf_info *lookup_keyword(char *c)
{
	struct conf_info *p;
	
	for (p = clist; p < clist + (sizeof (clist) / sizeof (struct conf_info)); p++)
	{
		if (strcasecmp(c, p->name) == 0)
			return p;
	}
	return NULL;
}

//将args的内容复制一份到p所指的结构体的object属性
void apply_command(Cconf_info * p, char *args)
{
	if (p->object) {
		if (*(char **) p->object != NULL)
			free(*(char **) p->object);
		*(char **) p->object = strdup(args);
	}
}

//读取配置文件的内容
void parse(FILE * fp)
{
	Cconf_info *p;
	char buf[1024], *c;
	int line = 0; //统计行数
	while (fgets(buf, 1024, fp) != NULL)
	{
		line++;
		if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n')
		{ //井号开头为注释行
			memset(buf, 0, 1024);
			continue;
		}
		trim(buf); //去掉两边空格
		if (buf[0] == '\0')
			continue;
		c = buf;
		while (*c != ':') { //找到配置文件中的冒号
			c++;
		}
		if (*c == '\0') {
			c = NULL;
		} else { //将冒号替换成‘\0’结束符
			*c = '\0';
			c++;
		}
		trim(buf);
		p = lookup_keyword(buf); //查询配置的是哪一项
		if(p != NULL) //将查到的配置加入到name属性中
		{
			trim(c);
			apply_command(p, c);
		}
		memset(buf, 0, 1024);
	}
}
