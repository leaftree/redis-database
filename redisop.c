
/**
 * redisop.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-08 14:23:14
 * Last Modified : 2018-03-08 14:23:14
 */

#include "db.h"
#include "hiredis.h"

#ifdef mFree
# undef mFree
#endif
# define mFree(p) do { if(p) { free(p); p = NULL; } }while(0)

redisContext *defaultRedisHandle = NULL;

int dbapp_gen_request_json_string(DBHandle *handle, char *stmt)
{
	DBUG_ENTER(__func__);

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
	cJSON_AddItemToObject(root, "sync", handle->sync==DBAPI_SYNC?cJSON_CreateTrue():cJSON_CreateFalse());
	cJSON_AddItemToObject(root, "rchannel", cJSON_CreateString(handle->recv_channel));
	cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(handle->timestamp));
	cJSON_AddItemToObject(root, "sql", cJSON_CreateString(stmt));

	handle->json_string = cJSON_PrintBuffered(root, 4096, 0);

	cJSON_free(root);
	DBUG_RETURN(0);
}

int dbapp_gen_error_response_json_string(DBHandle *handle, DBDriver *driver)
{
	DBUG_ENTER(__func__);

	if(handle == NULL || driver == NULL)
		DBUG_RETURN(-1);

	prctl(PR_GET_NAME, handle->app);
	handle->timestamp = time(NULL);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "id", cJSON_CreateNumber(handle->crc16));
	cJSON_AddItemToObject(root, "app", cJSON_CreateString(handle->app));
	cJSON_AddItemToObject(root, "type", cJSON_CreateString("response"));
	cJSON_AddItemToObject(root, "eflag", driver->ecode?cJSON_CreateFalse():cJSON_CreateTrue());
	cJSON_AddItemToObject(root, "message", driver->ecode?cJSON_CreateString(driver->errstr):cJSON_CreateString("OK"));
	cJSON_AddItemToObject(root, "pid", cJSON_CreateNumber(getpid()));
	cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(handle->timestamp));
	cJSON_AddItemToObject(root, "sqltype", cJSON_CreateNumber(driver->sqltype));

	handle->json_string = cJSON_Print(root);
	printf("===%s\n", handle->json_string);
	cJSON_free(root);

	DBUG_RETURN(0);
}

int dbapp_write_redis(DBHandle *handle)
{
	DBUG_ENTER(__func__);

	redisReply *reply = NULL;
	static char L_key[] = "LPUSH";
	static char R_key[] = "RPUSH";

	return_val_if_fail(handle!=NULL && handle->redis!=NULL, -1, "");

	if(handle->send_channel == NULL || handle->json_string == NULL)
	{
		sprintf(handle->errstr, "%s", strerror(EINVAL));
		DBUG_RETURN(-1);
	}

	reply = redisCommand(handle->redis, "%s %s %s",
			handle->priority==PRI_R?R_key:L_key, handle->send_channel, handle->json_string);
	if(reply == NULL || reply->type != REDIS_REPLY_INTEGER)
	{
		freeReplyObject(reply);
		DBUG_RETURN(-1);
	}

	freeReplyObject(reply);
	DBUG_RETURN(0);
}

int dbapp_recv_redis(DBHandle *handle)
{
	DBUG_ENTER(__func__);

	redisReply *reply = NULL;
	const char defaultKey[] = "BLPOP";

	return_val_if_fail(handle!=NULL && handle->redis!=NULL, -1, "");

	reply = redisCommand(handle->redis, "%s %s %d",
			defaultKey, handle->recv_channel, handle->oper_timeout);
	if(reply == NULL || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2)
	{
		DBUG_PRINT("Reply", ("ret=%d %s %s %d", reply->type, defaultKey, handle->recv_channel, handle->oper_timeout));
		if(reply && reply->str)
			sprintf(handle->errstr, "[redis client] %s", reply->str);
		else if(reply && reply->type == REDIS_REPLY_NIL)
			sprintf(handle->errstr, "[redis client] Read response timeout");
		freeReplyObject(reply);
		DBUG_RETURN(-1);
	}

	handle->root = cJSON_Parse(reply->element[1]->str);

	freeReplyObject(reply);
	DBUG_RETURN(0);
}

static DBHandle *DBAPPInit(int flag, va_list arg)
{
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
	//handle->recv_channel = NULL;
	//handle->send_channel = strdup(defaultQueue);
	handle->priority = defaultSendChannelPriority;
	handle->redis = defaultRedisHandle;

	if(flag & DBAPI_SYNC) {
		handle->sync = DBAPI_SYNC;
	}
	if(flag & DBAPI_TIMEO) {
		handle->oper_timeout = va_arg(arg, int);
	}
	if(flag & DBAPI_CLIENT) {
		handle->recv_channel = NULL;
		handle->send_channel = strdup(defaultQueue);
	} else {
		handle->recv_channel = strdup(defaultQueue);
		handle->send_channel = NULL;
	}

	return handle;
}

DBHandle *DBClientInit(int flag, ...)
{
	va_list arg;
	DBHandle *handle = NULL;

	if(flag - (flag&DBAPI_SYNC) - (flag&DBAPI_TIMEO) -
			(flag&DBAPI_CLIENT) - (flag&DBAPI_SERVER))
	{
		errno = EINVAL;
		return NULL;
	}

	va_start(arg, flag);
	handle = DBAPPInit(flag|DBAPI_CLIENT, arg);
	va_end(arg);

	return handle;
}

DBHandle *DBServerInit(int flag, ...)
{
	va_list arg;
	DBHandle *handle = NULL;

	if(flag - (flag&DBAPI_SYNC) - (flag&DBAPI_TIMEO) -
			(flag&DBAPI_CLIENT) - (flag&DBAPI_SERVER))
	{
		errno = EINVAL;
		return NULL;
	}

	va_start(arg, flag);
	handle = DBAPPInit(flag|DBAPI_SERVER, arg);
	va_end(arg);

	return handle;
}

redisContext *RedisConnection(char *host, unsigned short int port, unsigned int timeo)
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
		redisFree(c);
		return NULL;
	}

	return c;
}

int RedisAuth(redisContext *redis, char *password)
{
	int retval = -1;
	redisReply *reply = NULL;
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
				retval = 0;
			}
			else if(reply->type == REDIS_REPLY_ERROR)
			{
				if(strstr(reply->str, invalid))
					retval = -1;
				else if(strstr(reply->str, notset))
					retval = 0;
			}
			freeReplyObject(reply);
		}
	}
	return retval;
}

//
// Client Api
//
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

	if(dbapp_write_redis(handle))
	{
		sprintf(handle->errstr+strlen(handle->errstr),
				"[dbapp_write_redis] Write request to redis failed");
		return -1;
	}

	if(handle->sync == DBAPI_SYNC)
	{
		if(dbapp_recv_redis(handle))
		{
			sprintf(handle->errstr + strlen(handle->errstr),
					"\n[dbapp_recv_redis] Receive response from redis failed");
			return -1;
		}
	}

	return 0;
}

int DBServerRecvRequest(DBHandle *handle)
{
	DBUG_ENTER(__func__);

	if(handle == NULL)
		DBUG_RETURN(-1);

	//handle->recv_channel = defaultQueue;
	if(dbapp_recv_redis(handle))
	{
		DBUG_PRINT("dbapp_recv_redis", ("%s", handle->errstr));
		DBUG_RETURN(-1);
	}

	DBUG_RETURN(0);
}

int DBServerSendReponse(DBHandle *handle)
{
	DBUG_ENTER(__func__);

	if(handle == NULL)
		DBUG_RETURN(-1);

	if(dbapp_write_redis(handle))
	{
		sprintf(handle->errstr,
				"[dbapp_write_redis] Write response to redis failed");
		DBUG_RETURN(-1);
	}

	DBUG_RETURN(0);
}

int DBClientSendRequest(DBHandle *handle, char *stmt)
{
	DBUG_ENTER(__func__);

	if(handle == NULL || stmt == NULL)
		DBUG_RETURN(-1);

	if(dbapp_gen_request_json_string(handle, stmt))
	{
		sprintf(handle->errstr,
				"[dbapp_gen_request_json_string] Generate json string failed");
		DBUG_RETURN(-1);
	}

	if(dbapp_write_redis(handle))
	{
		sprintf(handle->errstr,
				"[dbapp_write_redis] Write request to redis failed");
		DBUG_RETURN(-1);
	}
	mFree(handle->json_string);

	if(handle->sync == DBAPI_SYNC)
	{
		DBUG_PRINT("Receive channle", ("%s", handle->recv_channel));
		if(dbapp_recv_redis(handle))
		{
			sprintf(handle->errstr + strlen(handle->errstr),
					"\n[dbapp_recv_redis] Receive response from redis failed");
			DBUG_RETURN(-1);
		}
	}

	DBUG_RETURN(0);
}
