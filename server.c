
/**
 * dbservice.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-02 11:03:12
 * Last Modified : 2018-03-02 11:03:12
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <limits.h>

#include <hiredis.h>
#include "cJSON.h"
#include "db.h"

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

redisContext *defaultRedisHandle = NULL;

redisContext *Redis(char *host, unsigned short int port, unsigned int timeo)
{
	char hostname[64] = "";
	struct timeval timeout = { timeo, 0 };
	redisContext *c = NULL;

	snprintf(hostname, sizeof(hostname), host?host:"127.0.0.1");

	if(timeo)
		c = redisConnectWithTimeout(hostname, port, timeout);
	else
		c = redisConnect(hostname, port);

	if (c == NULL )
	{
		return NULL;
	}
	else if(c->err)
	{
		printf("Connection error: %s\n", c->errstr);
		redisFree(c);
	}

	defaultRedisHandle = c;

	return c;
}

int RedisAuth(redisContext *redis, char *password)
{
	redisReply *reply = NULL;
	int ret = -1;
	static const char ok[] = "OK";
	static const char invalid[] = "invalid password";
	static const char notset[] = "no password is set";

	if(redis && password)
	{
		reply = redisCommand(redis, "AUTH %s", password);
		if(reply)
		{
			if(reply->type == REDIS_REPLY_STATUS && !strcasecmp(reply->str, ok))
			{
				ret = 0;
			}
			else if(reply->type == REDIS_REPLY_ERROR)
			{
				if(strstr(reply->str, invalid))
					ret = -1;
				else if(strstr(reply->str, notset))
					ret = 0;
			}
			freeReplyObject(reply);
		}
	}
	return ret;
}

int main()
{
	DBUG_PUSH ("d:t:O");
	DBUG_PROCESS ("测试程序");

	DBUG_ENTER(__func__);
	int i;
	int port = 6379;
	prctl(PR_SET_NAME, "测试程序");

	//daemon(1, 1);

	DBDriver *driver = DBHEnvNew(NULL, "fzlc50db@afc", "fzlc50db");
	if(driver == NULL)
	{
		DBUG_PRINT("DBHEnvNew", ("Create handle env fail"));
		DBUG_RETURN(-1);
	}
	if(DBO_SUCC != DBConnection(driver))
	{
		DBUG_PRINT("DBConnection", ("%s", driver->errstr));
		DBUG_RETURN(-1);
	}

	redisContext *c = Redis(NULL, port, 0);
	if(!c)
	{
		DBUG_PRINT("Redis", ("Create redis connect fail"));
		DBUG_RETURN(-1);
	}
	if(RedisAuth(c, "123kbc,./"))
	{
		DBUG_PRINT("Redis", ("Auth fail"));
		DBUG_RETURN(-1);
	}

	while(1)
	{
		DBHandle *handle = DBAPPInit(DBAPI_SYNC|DBAPI_TIMEO, 1);
		if(NULL==handle)
		{
			DBUG_PRINT("DBAPPHandle", ("New fail"));
			DBUG_RETURN(-1);
		}

		i = DBAPPExecute(handle, "SELECT * FROM BASI_STATION_INFO");
		if(i)
		{
			fprintf(stdout, "%s\n", handle->errstr);
			//return -1;
		}

		time_t t1 = time(NULL);
		if(DBO_SUCC != DBExecute(driver, "select * from dev_modbus_status"))
		{
			DBUG_PRINT("DBExecute", ("%s", driver->errstr));
			DBUG_RETURN(-1);
		}
		if(DBO_SUCC != DBStmtFree(driver))
		{
			DBUG_PRINT("DBStmtFree", ("%s", driver->errstr));
			DBUG_RETURN(-1);
		}
		time_t t2 = time(NULL);
		DBUG_PRINT("timestamp", ("%d", t2-t1));

	}
	DBCloseConnection(driver);
	DBReleaseHEnv(&driver);
	redisFree(c);

	DBUG_RETURN(0);
}
