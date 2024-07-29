CFLAGS=-Wall -Wextra
CWD=$(shell pwd)


cproj: main.c cJSON.c bin
	gcc main.c cJSON.c $(CFLAGS) -I$(CWD)/clog -o bin/cproj -DJSON_SKIP_WHITESPACE -DCLOG_NO_TIME

bin:
	mkdir -p $@
