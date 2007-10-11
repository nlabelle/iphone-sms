#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "shttpd.h"
#include <sqlite3.h>


unsigned char  *
urlencode(unsigned char *s, unsigned char *t)
{
	unsigned char  *p, *tp;

	if (t == NULL) {
		fprintf(stderr, "Serious memory error...\n");
		return t;
	}
//	fprintf(stderr,"urlencode:\n");
	tp = t;
	for (p = s; *p; p++) {
//		fprintf(stderr,"%02X ",*p);
		if ((*p > 0x00 && *p < ',') ||
		    (*p > '9' && *p < 'A') ||
		    (*p > 'Z' && *p < '_') ||
		    (*p > '_' && *p < 'a') ||
		    (*p > 'z' && *p < 0xA1)) {
			sprintf(tp, "%%%02X", *p);
			tp += 3;
		} else {
			*tp = *p;
			tp++;
		}
	}

	*tp = '\0';

	return t;
}

int
sqlite_html(struct shttpd_arg_t * arg)
{
	int             n = 0;
	int            *p = arg->user_data;	/* integer passed to us */
	const char     *db_name, *exec_str;

	sqlite3        *db;
	sqlite3_stmt   *stmt;
	int             rc;

	if ((db_name = shttpd_get_var(arg, "db")) != NULL) {
		if ((exec_str = shttpd_get_var(arg, "sql_exec")) != NULL) {
			//n += snprintf(arg->buf + n, arg->buflen - n, "db=%s, sql=%s\n", db_name, exec_str);
			//fprintf(stderr, "db=%s, sql=%s\n", db_name, exec_str);
			rc = sqlite3_open(db_name, &db);
			if (rc) {
				n += snprintf(arg->buf + n, arg->buflen - n, "Can't open database: %s,%s\n", db_name, sqlite3_errmsg(db));
				sqlite3_close(db);
				arg->last = 1;
				return n;
			}
			rc = sqlite3_prepare(db, exec_str, 0, &stmt, 0);
			if (rc != SQLITE_OK) {
				n += snprintf(arg->buf + n, arg->buflen - n, "sql error #%d: %s\n", rc, sqlite3_errmsg(db));
				arg->last = 1;
				return n;
			} else {
				int             first_line = 1;
				n += snprintf(arg->buf + n, arg->buflen - n, "%s",
					      "HTTP/1.1 200 OK\r\n"
					  "Content-Type: text/plain\r\n\r\n"
					      "<meta http-equiv=\"Content-Type\" content=\"text/plain; charset=utf8\">"
					      "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />");
				while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
					switch (rc) {
					case SQLITE_BUSY:
						fprintf(stderr, "busy, wait 1 seconds\n");
						sleep(1);
						break;
					case SQLITE_ERROR:
						fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
						break;
					case SQLITE_ROW:
						{
							int             c = sqlite3_column_count(stmt);
							int             i;

							if (first_line) {
								for (i = 0; i < c; i++)
									n += snprintf(arg->buf + n, arg->buflen - n, "%s|", sqlite3_column_name(stmt, i));
								n += snprintf(arg->buf + n, arg->buflen - n, "\n");
								first_line = 0;
							}
							for (i = 0; i < c; i++) {
								switch (sqlite3_column_type(stmt, i)) {
								case SQLITE_TEXT:
									n += snprintf(arg->buf + n, arg->buflen - n, "%s", sqlite3_column_text(stmt, i));
									//fprintf(stderr, "%s", sqlite3_column_text(stmt, i));
									break;
								case SQLITE_INTEGER:
									n += snprintf(arg->buf + n, arg->buflen - n, "%d", sqlite3_column_int(stmt, i));
									break;
								case SQLITE_FLOAT:
									n += snprintf(arg->buf + n, arg->buflen - n, "%f", sqlite3_column_double(stmt, i));
									break;
								case SQLITE_BLOB:
									printf("(blob)");
									break;
								case SQLITE_NULL:
									printf("(null)");
									break;
								default:
									printf("(unknown: %d)", sqlite3_column_type(stmt, i));
									break;
								}
								n += snprintf(arg->buf + n, arg->buflen - n, "|");
							}
							n += snprintf(arg->buf + n, arg->buflen - n, "\n");

						}
						break;

					}

				}
				sqlite3_finalize(stmt);
				sqlite3_close(db);
			}

		} else {
			char            url[512];
			n += snprintf(arg->buf + n, arg->buflen - n, "<p>usage: sqlite.html?db=/path/db_file&sql_exec=sql...</p>");
			n += snprintf(arg->buf + n, arg->buflen - n, "<a href=\"/sqlite.html?db=/var/root/Library/SMS/sms.db&sql_exec=%s\">SMS.db</a>", urlencode("select * from message", url));
		}
	} else {
		char            url[512];
		n += snprintf(arg->buf + n, arg->buflen - n, "%s",
			      "HTTP/1.1 200 OK\r\n"
			      "Content-Type: text/html\r\n\r\n"
			      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">"
			      "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
			      "<html><body>");
		n += snprintf(arg->buf + n, arg->buflen - n, "<p>usage: sqlite.html?db=/path/db_file&sql_exec=sql...</p>");
		n += snprintf(arg->buf + n, arg->buflen - n, "<a href=\"/sqlite.html?db=/var/root/Library/SMS/sms.db&sql_exec=%s\">SMS.db</a>", urlencode("select * from message", url));
		n += snprintf(arg->buf + n, arg->buflen - n, "</body></html>");
	}

	assert(n < (int) arg->buflen);
	arg->last = 1;

	return (n);
}


#define RESULT_LEN 64*1024
//colum1|colum2|...\ncolum1|...\n
char *sqlite_exec(const char *db_name, const char *exec_str)
{
	int             n = 0;
	sqlite3		*db;
	sqlite3_stmt	*stmt;
	int             rc;
	char		*sql_result;
	
	rc = sqlite3_open(db_name, &db);
	if (rc) {
		fprintf(stderr, "can't open db %s: %s\n", db_name, sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}
	rc = sqlite3_prepare(db, exec_str, 0, &stmt, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "sql error #%d: %s\n", rc, sqlite3_errmsg(db));
		return NULL;
	} else {
		sql_result = (char *)malloc(RESULT_LEN);
		sql_result[0] = 0;
		while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
			switch (rc) {
			case SQLITE_BUSY:
				fprintf(stderr, "busy, wait 1 seconds\n");
				sleep(1);
				break;
			case SQLITE_ERROR:
				fprintf(stderr, "step error: %s\n", sqlite3_errmsg(db));
				break;
			case SQLITE_ROW:
				{
					int             c = sqlite3_column_count(stmt);
					int             i;
					for (i = 0; i < c; i++) {
						switch (sqlite3_column_type(stmt, i)) {
						case SQLITE_TEXT:
							n += snprintf(sql_result+n, RESULT_LEN - n, "%s", sqlite3_column_text(stmt, i));
							break;
						case SQLITE_INTEGER:
							n += snprintf(sql_result+n, RESULT_LEN - n, "%d", sqlite3_column_int(stmt, i));
							break;
						case SQLITE_FLOAT:
							n += snprintf(sql_result+n, RESULT_LEN - n, "%f", sqlite3_column_double(stmt, i));
							break;
						case SQLITE_BLOB:
							printf("(blob)");
							break;
						case SQLITE_NULL:
						//	printf("(null)");
							break;
						default:
							printf("(unknown: %d)", sqlite3_column_type(stmt, i));
							break;
						}
						n += snprintf(sql_result + n, RESULT_LEN - n, "|");
					}
					n += snprintf(sql_result + n, RESULT_LEN - n, "\n");

				}
				break;
			}

		}
		sqlite3_finalize(stmt);
		sqlite3_close(db);
	}

	return sql_result;
}
/*
select address,text,date from message order by date desc;
select multivalue_id,value from ABPhoneLastFour;
sqlite> select record_id from ABMultiValue where value="1 (352) 126-1768";
2
sqlite> select LastSort from ABPerson where ROWID=2;
*/
#if 0
int main()
{
    char *line;
    char *sr,*sr1;
    sr = sqlite_exec("sms.db","select address,text,date from message order by date desc");
    for(line = sr; line != 2; line = strstr(line,"|\n")+2)
    {
	    char addr[64];
	    char text[1024];
	    time_t t;
	    char sql[128];
	    int id;
	    sscanf(line,"%[^|]|%[^|]|%d",addr,text,&t);
	    sprintf(sql,"select multivalue_id from ABPhoneLastFour where value=%s",&addr[strlen(addr)-4]);
	    sr1 = sqlite_exec("AddressBook.sqlitedb",sql);
	    sscanf(sr1,"%d",&id);
	    free(sr1);
	    sprintf(sql,"select LastSort from ABPerson where ROWID=%d",id);
	    sr1 = sqlite_exec("AddressBook.sqlitedb",sql);
	    if(sr1[0] != 0)
	    {
		sr1[strlen(sr1)-2] = 0;
		printf("%s ",sr1);
	    }else
		printf("%s ",addr);
	    printf("%s%s\n",ctime(&t),text);
	    free(sr1);
    }
    free(sr);
}
#endif

