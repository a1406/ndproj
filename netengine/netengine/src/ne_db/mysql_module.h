#ifndef _MYSQL_MODULE_H
#define _MYSQL_MODULE_H

/* todo
 * create a msg queue, support a API to add cmd to the msg queue, and support a
 * callback to notify the SQL result.
 */ 

#include "mysql/mysql.h"

NEINT32 init_db(NEINT8 *host, NEUINT32 port, NEINT8 *dbname, NEINT8 *user, NEINT8 *pwd);
NEINT32 close_db();

MYSQL_RES *query(NEINT8 *sql, NEINT32 noret, NEUINT32 *effect);
void free_query(MYSQL_RES *res);

MYSQL_ROW fetch_row(MYSQL_RES *res);
NEUINT32 escape_string(NEINT8 *to, const NEINT8 *from, NEUINT32 length);

const NEINT8 *fetch_field(MYSQL_ROW row, MYSQL_RES *res, NEINT8 *name, NEINT32 *len);
#endif
