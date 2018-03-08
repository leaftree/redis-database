
/**
 * dbdriver.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-07 16:16:37
 * Last Modified : 2018-03-07 16:16:37
 */

#ifndef __DBDRIVER_H__
#define __DBDRIVER_H__

#define SQL_MSG_LEN		   512
#define SQL_MAX_ITEM_NUM 100
#define SQL_MAX_SQL_LEN	 3000

#define SQL_FIELD_TYPE_STRING  1
#define SQL_FIELD_TYPE_INTEGER 2
#define SQL_FIELD_TYPE_DOBULE  3
#define SQL_FIELD_TYPE_DATE    4

#define SQL_TYPE_QUERY 0
#define SQL_TYPE_NONE_QUERY 1

#if defined(DBO_FAIL) || defined(DBO_SUCC)
# undef DBO_FAIL
# undef DBO_SUCC
#endif
#define DBO_SUCC (+0)
#define DBO_FAIL (-1)

typedef void* DBHDBC;
typedef void* DBHSTMT;

typedef struct __DBDriver
{
	char user[64];
	char password[64];
	char hostname[64];
	char database[64];

	int ecode; // Error number code
	int sqltype; // Zero:NonQuery !Zero:Query
	char *json_string;
	char errstr[SQL_MSG_LEN];
	char sqlstmt[SQL_MAX_SQL_LEN];
	DBHDBC connection; // reserve
	DBHSTMT statment;
} DBDriver;

__BEGIN_DECLS

DBDriver *DBHEnvNew(char *db, char *user, char *password);
int DBConnection(DBDriver *driver);
int DBExecute(DBDriver *driver, char *statment);
int DBStmtFree(DBDriver *driver);
int DBCloseConnection(DBDriver *driver);
int DBReleaseHEnv(DBDriver **driver);

__END_DECLS

#endif /* __DBDRIVER_H__ */
