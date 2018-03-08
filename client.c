
/**
 * db.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-05 13:28:44
 * Last Modified : 2018-03-05 13:28:44
 */

#include "db.h"
#include "hiredis.h"

static int dbapp_gen_request_json_string(DBHandle *handle, char *stmt)
{
	int seq;
	static int sequence=0;
	static char app[64] = "";

	return_val_if_fail(stmt!=NULL && handle!=NULL, -1, "");

	prctl(PR_GET_NAME, app);
	handle->app = strdup(app);
	handle->timestamp = time(NULL);
	handle->crc16 = GenCrc16(stmt, strlen(stmt));
	handle->recv_channel = strdup(app);

	seq = __sync_fetch_and_add(&sequence, 1);
	if(seq==INT_MAX-1) __sync_fetch_and_sub(&sequence, INT_MAX-1);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "id", cJSON_CreateNumber(handle->crc16));
	cJSON_AddItemToObject(root, "app", cJSON_CreateString(handle->app));
	cJSON_AddItemToObject(root, "pid", cJSON_CreateNumber(getpid()));
	cJSON_AddItemToObject(root, "type", cJSON_CreateString("request"));
	cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(handle->timestamp));
	cJSON_AddItemToObject(root, "sql", cJSON_CreateString(stmt));

	handle->json_string = cJSON_PrintBuffered(root, 4096, 0);

	cJSON_free(root);
	return 0;
}

 int dbapp_client_write_redis(DBHandle *handle)
{
	redisReply *reply = NULL;
	static char L_key[] = "LPUSH";
	static char R_key[] = "RPUSH";

	return_val_if_fail(handle!=NULL && handle->redis!=NULL, -1, "");

	reply = redisCommand(handle->redis, "%s %s %s",
			handle->priority==PRI_R?R_key:L_key, handle->send_channel, handle->json_string);
	if(reply == NULL || reply->type != REDIS_REPLY_INTEGER)
	{
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;
}

static int dbapp_client_recv_redis(DBHandle *handle)
{
	redisReply *reply = NULL;
	const char defaultKey[] = "BLPOP";

	return_val_if_fail(handle!=NULL && handle->redis!=NULL, -1, "");

	reply = redisCommand(handle->redis, "%s %s %d",
			defaultKey, handle->recv_channel, handle->oper_timeout);
	if(reply == NULL || reply->type != REDIS_REPLY_STRING)
	{
		if(reply && reply->str)
			sprintf(handle->errstr, "[redis client] %s", reply->str);
		else if(reply && reply->type == REDIS_REPLY_NIL)
			sprintf(handle->errstr, "[redis client] Read response timeout");
		freeReplyObject(reply);
		return -1;
	}

	//handle->rset;
	handle->root = cJSON_Parse(reply->str);

	freeReplyObject(reply);
	return 0;
}

DBHandle *DBAPPInit(int flag, ...)
{
	va_list arg;

	DBHandle *handle = malloc(sizeof(DBHandle));
	if(handle == NULL)
	{
		return NULL;
	}

	handle->root = handle->rset = NULL;
	handle->ora = 0;
	handle->idx = 0;
	handle->more = 0;
	handle->crc16 = 0;
	handle->timestamp = 0;
	handle->errstr[0] = 0;
	handle->sync = DBAPI_ASYNC;
	handle->oper_timeout = defaultOperTimeOut;
	handle->recv_channel = NULL;
	handle->send_channel = strdup(defaultQueue);
	handle->priority = defaultSendChannelPriority;
	handle->redis = defaultRedisHandle;

	if(flag & DBAPI_SYNC) {
		handle->sync = DBAPI_SYNC;
	}
	if(flag & DBAPI_TIMEO) {
		va_start(arg, flag);
		handle->oper_timeout = va_arg(arg, int);
		va_end(arg);
	}
	return handle;
}

int DBAPPExecute(DBHandle *handle, char *stmt)
{
	return_val_if_fail(handle!=NULL && stmt!=NULL, -1, "");

	if(dbapp_gen_request_json_string(handle, stmt))
	{
		sprintf(handle->errstr,
				"[dbapp_gen_request_json_string] Generate json string failed");
		return -1;
	}
	printf("json:%s\n", handle->json_string);

	if(dbapp_client_write_redis(handle))
	{
		sprintf(handle->errstr,
				"[dbapp_write_redis] Write request to redis failed");
		return -1;
	}

	if(handle->sync == DBAPI_SYNC)
	{
		if(dbapp_client_recv_redis(handle))
		{
			sprintf(handle->errstr + strlen(handle->errstr),
					"\n[dbapp_recv_redis] Receive response from redis failed");
			return -1;
		}
	}

	return 0;
}
