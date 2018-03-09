
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
#include <signal.h>

#include <hiredis.h>
#include "cJSON.h"
#include "db.h"

const static int rport = 6379;

static void exit_sig(int sig)
{
	printf("Exit system\n");
	exit(0);
}

static void access_signal()
{
	signal(SIGQUIT, exit_sig);
}

int main()
{
	DBUG_ENTER(__func__);

	DBUG_PUSH ("d:t:O");
	DBUG_PROCESS ("测试程序");
	prctl(PR_SET_NAME, "测试程序");

	access_signal();

	//daemon(1, 1);

	DBDriver *dbo = DBHEnvNew(NULL, "fzlc50db@afc", "fzlc50db");
	if(dbo == NULL)
	{
		DBUG_PRINT("DBHEnvNew", ("Create handle env fail"));
		DBUG_RETURN(-1);
	}
	if(DBO_SUCC != DBConnection(dbo))
	{
		DBUG_PRINT("DBConnection", ("%s", dbo->errstr));
		DBUG_RETURN(-1);
	}

	defaultRedisHandle = RedisConnection(NULL, rport, 0);
	if(!defaultRedisHandle)
	{
		DBUG_PRINT("Redis", ("Create redis connect fail"));
		DBUG_RETURN(-1);
	}
	if(RedisAuth(defaultRedisHandle, "123kbc,./"))
	{
		DBUG_PRINT("Redis", ("Auth fail"));
		DBUG_RETURN(-1);
	}

	DBHandle *handle = DBServerInit(DBAPI_SYNC|DBAPI_TIMEO, 100);
	if(NULL==handle)
	{
		DBUG_PRINT("DBAPPHandle", ("New fail"));
		DBUG_RETURN(-1);
	}

	int loop=1000;
	while(loop--)
	{
		sleep(1);
		if(DBServerRecvRequest(handle))
		{
			DBUG_PRINT("DBServerRecvRequest", ("%s", handle->errstr));
			continue;
		}
		DBUG_PRINT("SQL-REQUEST", ("%s", cJSON_PrintUnformatted(handle->root)));

		cJSON *field = cJSON_GetObjectItem(handle->root, "type");
		if(field == NULL || field->type != cJSON_String || strncasecmp(field->valuestring, "REQUEST", strlen("REQUEST")))
		{
			DBUG_PRINT("parse object", ("It's not a request message:%s", field->string));
			continue;
		}

		DBUG_PRINT("SQL", ("%s", cJSON_GetObjectItem(handle->root, "sql")->valuestring));
		field = cJSON_GetObjectItem(handle->root, "sql");
		if(field == NULL)
		{
			DBUG_PRINT("parse object", ("Can't found sql"));
			continue;
		}
		char *stmt = cJSON_GetStringValue(field);

		field = cJSON_GetObjectItem(handle->root, "rchannel");
		if(field == NULL)
		{
			DBUG_PRINT("parse object", ("Can't found recive channle to response"));
			continue;
		}
		handle->send_channel = strdup(field->valuestring);

		field = cJSON_GetObjectItem(handle->root, "sync");
		if(field == NULL)
		{
			DBUG_PRINT("parse object", ("Can found sync"));
		}
		else
		{
			if(field->type == cJSON_True)
				handle->sync = DBAPI_SYNC;
			else
				handle->sync = DBAPI_ASYNC;
		}

		DBUG_PRINT("SqlStmt", ("%s", stmt));

		if(DBO_SUCC != DBExecute(dbo, stmt))
		{
			dbapp_gen_error_response_json_string(handle, dbo);

			//DBUG_PRINT("DBExecute", ("%s", dbo->errstr));
			//DBUG_RETURN(-1);
		}
		else
		{
			handle->json_string = dbo->json_string;
		}

		//handle->send_channel = strdup(defaultQueue);
		if(DBServerSendReponse(handle))
		{
			DBUG_PRINT("DBServerSendReponse", ("%s", handle->errstr));
		}

		if(DBO_SUCC != DBStmtFree(dbo))
		{
			DBUG_PRINT("DBStmtFree", ("%s", dbo->errstr));
			DBUG_RETURN(-1);
		}
	}

	DBCloseConnection(dbo);
	DBReleaseHEnv(&dbo);
	redisFree(defaultRedisHandle);

	DBUG_RETURN(0);
}
