
/**
 * db.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-05 13:28:44
 * Last Modified : 2018-03-05 13:28:44
 */

#include <hiredis.h>
#include "db.h"
#include "cJSON.h"
#include "db.h"

int main()
{
	DBUG_PUSH ("d:t:O");
	DBUG_PROCESS ("Client");

	DBUG_ENTER(__func__);
	int i;
	int port = 6379;
	prctl(PR_SET_NAME, "Client");

	//daemon(1, 1);

	defaultRedisHandle = RedisConnection(NULL, port, 0);
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

	DBHandle *handle = DBClientInit(DBAPI_SYNC|DBAPI_TIMEO, 10);
	if(NULL==handle)
	{
		DBUG_PRINT("DBAPPHandle", ("New fail"));
		DBUG_RETURN(-1);
	}
	//handle->recv_channel = strdup(defaultQueue);

	int loop=1;
	while(loop--)
	{
		if(DBClientSendRequest(handle, "select STATION_ID, vvv, STATION_CN_NAME, STATION_IP, DEVICE_ID from basi_station_info"))
		{
			DBUG_PRINT("DBClientSendRequest", ("%s", handle->errstr));
			continue;
		}

		DBUG_PRINT("json", ("%s", cJSON_Print(handle->root)));
	}

	redisFree(defaultRedisHandle);

	DBUG_RETURN(0);
}
