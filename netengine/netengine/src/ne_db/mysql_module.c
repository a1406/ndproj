#include "mysql_module.h"

static MYSQL *m_mysql = NULL;

NEINT32 init_db(NEINT8 *host, NEUINT32 port, NEINT8 *dbname, NEINT8 *user, NEINT8 *pwd)
{
	if (m_mysql != NULL)
		return (-1);

	if (!host || !dbname || !user || !pwd)
		return (-1);

	m_mysql = mysql_init(NULL);
	if (!m_mysql)
		return (-1);

	if (m_mysql != mysql_real_connect(m_mysql, host, user, pwd, dbname, port, NULL, 0)) {
		mysql_close(m_mysql);
		m_mysql = NULL;
		return (-1);
	}
	
	return (0);
}

NEINT32 close_db()
{
	if (m_mysql) {
		mysql_close(m_mysql);
		m_mysql = NULL;
	}
	return (0);		
}

MYSQL_RES *query(NEINT8 *sql, NEINT32 noret, NEUINT32 *effect)
{
	NEINT32 ret;

	if (!m_mysql)
		return (NULL);
//	NEINT32 mysql_real_query(MYSQL *mysql, const NEINT8 *query, NEUINT32 length)
	ret = mysql_real_query(m_mysql, sql, strlen(sql));
	if (ret != 0)
		return (NULL);
	if (affect)
		*affect = mysql_affected_rows(m_mysql);
	
	
	return mysql_use_result(m_mysql);
//	return mysql_store_result(m_mysql);
	
}

void free_query(MYSQL_RES *res)
{
	if (res)
		mysql_free_result(res);	
}

static NEINT32 find_field_pos(MYSQL_RES *res, NEINT8 *name)
{
	NEUINT32 num_fields;
	NEUINT32 i;
	MYSQL_FIELD *fields;

	if (!res || !name)
		return (-1);

	num_fields = mysql_num_fields(result);
	fields = mysql_fetch_fields(result);
	for(i = 0; i < num_fields; i++) {
		if (strcmp(name, fields[i].name) == 0)
			return (i);
	}
	return (-2);
}

MYSQL_ROW fetch_row(MYSQL_RES *res)
{
	return mysql_fetch_row(res); 
}

NEUINT32 escape_string(NEINT8 *to, const NEINT8 *from, NEUINT32 length)
{
	return mysql_real_escape_string(m_mysql, to, from, length);
}

const NEINT8 *fetch_field(MYSQL_ROW row, MYSQL_RES *res, NEINT8 *name, NEINT32 *len)
{
	NEUINT32 *lengths;
	NEINT32 pos;
	if (!res || !name)
		return (NULL);

	pos = find_field_pos(res, name);
	if (pos < 0)
		return (NULL);

	if (len) {
		lengths = mysql_fetch_lengths(res);
		if (!lengths)
			return (NULL);
		*len = lengths[pos];
	}
	return row[pos];
}

