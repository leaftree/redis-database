
/**
 * db.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-05 13:11:44
 * Last Modified : 2018-03-05 13:11:44
 */

#ifndef __DB_H__
#define __DB_H__

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "util.h"
#include "dbug.h"
#include "cJSON.h"
#include "hiredis.h"
#include "dbdriver.h"

#define return_if_fail(expr, title) do{                                 \
	if (expr) { } else {                                                  \
		fprintf(stdout, "[%s(%d)-%s] %s: %s\n", __FILE__, __LINE__, __func__, title, #expr); \
		return;                                                             \
	};}while(0)

#define return_val_if_fail(expr, value, title) do{                      \
	if (expr) { } else {                                                  \
		fprintf(stdout, "[%s(%d)-%s] %s: %s\n", __FILE__, __LINE__, __func__, title, #expr); \
		return(value);                                                      \
	};}while(0)

//
// 默认Redis连接
//
extern redisContext *defaultRedisHandle;

//
// 执行SQL时可以选择同步SYNC或异步ASYNC的方式
// 异步：将SQL发送到缓存后即刻返回
// 同步：将SQL发送到缓存后阻塞读SQL执行结果
// 建议在执行查询SQL语句时才使用同步方式
//
#define DBAPI_ASYNC 0
#define DBAPI_SYNC 1
#define DBAPI_TIMEO 4

//
// 默认发送+接收队列名称
//
#define defaultQueue "AfcDefaultQueue"

//
//
//
#define defaultOperTimeOut 0

//
// 优先级定义：R->右 L->左
// 默认发送：R
// 正常接收：L
// 若发送时使用L进行发送，则会优先处理
//
#define PRI_R 0
#define PRI_L 1
#define defaultSendChannelPriority PRI_R
#define defaultReadChannelPriority PRI_L

//
//
//
typedef struct __DBHandle {
	cJSON *root;
	cJSON *rset;
	redisContext *redis;
	int ora;
	int idx;
	int sync;
	int more;
	int crc16;
	int priority;
	int timestamp;
	int oper_timeout;
	char *app;
	char *send_channel;
	char *recv_channel;
	char *json_string;
	char errstr[512];
} DBHandle;

__BEGIN_DECLS

DBHandle *DBAPPInit(int flag, ...);
DBHandle *DBAPPFree(DBHandle *handle);
int DBAPPExecute(DBHandle *handle, char *stmt);

__END_DECLS

#endif /* __DB_H__ */
