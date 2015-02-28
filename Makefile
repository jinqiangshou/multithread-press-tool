#头文件
INCLUDE = 	-I./		\
		-I./include
#库文件
LIB 	=	-L ./		\
		-lz  -lpthread  -lm

CFLAG	=	-Wall -g -D_REENTRANT -D_FILTER
CPPFLAG	=	-Wall -g -D_REENTRANT -D_FILTER

OUTPUT	=	--error-printer --have-std --have-eh	

CC 	=	gcc
CPP	=	g++
PERL	=	perl
PY	=	python
AR	=	ar

all	:	press_tool
press_tool	:	press.cpp config.c
	$(CPP) $(CPPFLAG) -o $@ $^  $(INCLUDE) $(LIB)

clean	:	
	rm press_tool
