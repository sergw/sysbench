#ifdef STDC_HEADERS
# include <stdio.h>
#endif
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "sb_options.h"
#include "db_driver.h"
#include "../../db_driver.h"

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
#include <string.h>

#define xfree(ptr) ({ if (ptr) free((void *)ptr); ptr = NULL; })
#define URI_MAX_LENGTH 100
#define MAX_NUMBER_EMPTY_RESULT 5

/* Tarantool driver arguments */
static sb_arg_t tarantool_drv_args[] =
{
  SB_OPT("tarantool-host", "Tarantool server host", "localhost", STRING),
  SB_OPT("tarantool-port", "Tarantool server port", "3301", INT),
  SB_OPT("tarantool-user", "Tarantool user", "sbtest", STRING),
  SB_OPT("tarantool-password", "Tarantool password", "", STRING),
  SB_OPT("tarantool-db", "Tarantool database name", "sbtest", STRING),

  SB_OPT_END
};

typedef struct
{
    char               *host;
    char               *port;
    char               *user;
    char               *password;
    char               *db;
} tarantool_drv_args_t;

/* Tarantool driver capabilities */
static drv_caps_t tarantool_drv_caps =
{
  .multi_rows_insert = 1,
  .prepared_statements = 0,
  .auto_increment = 0,
  .needs_commit = 0,
  .serial = 0,
  .unsigned_int = 0,
};

static tarantool_drv_args_t args;          /* driver args */

/* Tarantool driver operations */
static int tarantool_drv_init(void);
static int tarantool_drv_describe(drv_caps_t *);
static int tarantool_drv_connect(db_conn_t *);
static int tarantool_drv_disconnect(db_conn_t *);
static db_error_t tarantool_drv_query(db_conn_t *, const char *, size_t, db_result_t *);
static int tarantool_drv_done(void);
static int tarantool_drv_prepare(db_stmt_t *, const char *, size_t);
static int tarantool_drv_bind_param(db_stmt_t *, db_bind_t *, size_t);
static db_error_t tarantool_drv_execute(db_stmt_t *, db_result_t *);
static int tarantool_drv_close(db_stmt_t *);
static int tarantool_drv_free_results(db_result_t *);

/* Tarantool driver definition */
static db_driver_t tarantool_driver =
{
  .sname = "tarantool",
  .lname = "Tarantool driver",
  .args = tarantool_drv_args,
  .ops = {
    .init = tarantool_drv_init,
    .connect = tarantool_drv_connect,
    .query = tarantool_drv_query,
    .disconnect = tarantool_drv_disconnect,
    .done = tarantool_drv_done,
    .describe = tarantool_drv_describe,
    .prepare = tarantool_drv_prepare,
    .bind_param = tarantool_drv_bind_param,
    .execute = tarantool_drv_execute,
    .close = tarantool_drv_close,
    .free_results = tarantool_drv_free_results,
  }
};


/* Register Tarantool driver */
int register_driver_tarantool(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&tarantool_driver.listitem, drivers);

  return 0;
}


/* Tarantool driver initialization */
int tarantool_drv_init(void)
{
  log_text(LOG_DEBUG, "tarantool_drv_init\n");

  args.host = sb_get_value_string("tarantool-host");
  args.port = sb_get_value_string("tarantool-port");
  args.user = sb_get_value_string("tarantool-user");
  args.password = sb_get_value_string("tarantool-password");
  args.db = sb_get_value_string("tarantool-db");

  return 0;
}

/* Connect to database */
int tarantool_drv_connect(db_conn_t *sb_conn)
{
  log_text(LOG_DEBUG, "tarantool_drv_connect\n");

  const char uri[URI_MAX_LENGTH];
  strcpy(uri, args.host);
  strcat(uri, ":");
  strcat(uri, args.port);

  struct tnt_stream * con = tnt_net(NULL); // Allocating stream
  tnt_set(con, TNT_OPT_URI, uri); // Setting URI
  tnt_set(con, TNT_OPT_SEND_BUF, 0); // Disable buffering for send
  tnt_set(con, TNT_OPT_RECV_BUF, 0); // Disable buffering for recv

  // Initialize stream and connect to Tarantool
  if (tnt_connect(con) < 0) {
    log_text(LOG_FATAL, "Connection to database failed");
    return 1;
  }

  sb_conn->ptr = con;

  return 0;
}

/* Disconnect from database */
int tarantool_drv_disconnect(db_conn_t *sb_conn)
{
  log_text(LOG_DEBUG, "tarantool_drv_disconnect\n");

  struct tnt_stream * con = sb_conn->ptr;

  if (con != NULL) {
    // Close connection and free stream object
    tnt_close(con); 
    tnt_stream_free(con); 
  }

  return 0;
}

/* Execute SQL query */
db_error_t tarantool_drv_query(db_conn_t *sb_conn, const char *query, size_t len,
                           db_result_t *rs)
{
  log_text(LOG_DEBUG, "tarantool_drv_query(%p, \"%s\")", sb_conn->ptr, query);

  struct tnt_stream * con = sb_conn->ptr;

  /*MAKE REQUEST*/
  struct tnt_stream *tuple = tnt_object(NULL);
  tnt_object_format(tuple, "[%s]", query);

  /*SEND REQUEST*/
  const char proc[] = "box.sql.execute";
  tnt_call(con, proc, sizeof(proc) - 1, tuple);
  tnt_flush(con);

  tnt_stream_free(tuple);

  struct tnt_reply *reply = tnt_reply_init(NULL);
  con->read_reply(con, reply);

  if (reply->code == 0) {
    if (reply->data_end - reply->data > MAX_NUMBER_EMPTY_RESULT)
      rs->counter = SB_CNT_READ;
    else
      rs->counter = SB_CNT_WRITE;

    tnt_reply_free(reply);
  } else {
    log_text(LOG_FATAL, "Failed %lu (%s).\n", reply->code, reply->error);

    rs->counter = SB_CNT_ERROR;

    tnt_reply_free(reply);
    return DB_ERROR_FATAL;
  }

  return DB_ERROR_NONE;
}

/* Uninitialize driver */
int tarantool_drv_done(void)
{
  log_text(LOG_DEBUG, "tarantool_drv_done\n");
  return 0;
}

/* Describe database capabilities */
int tarantool_drv_describe(drv_caps_t *caps)
{
  log_text(LOG_DEBUG, "tarantool_drv_describe\n");

  *caps = tarantool_drv_caps;

  const char uri[URI_MAX_LENGTH];
  strcpy(uri, args.host);
  strcat(uri, ":");
  strcat(uri, args.port);

  struct tnt_stream * con = tnt_net(NULL); // Allocating stream
  tnt_set(con, TNT_OPT_URI, uri); // Setting URI
  tnt_set(con, TNT_OPT_SEND_BUF, 0); // Disable buffering for send
  tnt_set(con, TNT_OPT_RECV_BUF, 0); // Disable buffering for recv

  // Initialize stream and connect to Tarantool
  if (tnt_connect(con) < 0) {
    log_text(LOG_FATAL, "Connection to database failed");
    return 1;
  }

  if (con != NULL) {
    // Close connection and free stream object
    tnt_close(con);
    tnt_stream_free(con);
  }

  return 0;
}

/* Prepare statement */
int tarantool_drv_prepare(db_stmt_t *stmt, const char *query, size_t len)
{
  log_text(LOG_DEBUG, "tarantool_drv_prepare: %s\n", query);

  struct tnt_stream * con = stmt->connection->ptr;

  (void) len; /* unused */

  if (con == NULL)
    return 1;

  stmt->query = strdup(query);

  return 0;
}

/* Bind parameters for prepared statement */
int tarantool_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  log_text(LOG_DEBUG, "tarantool_drv_bind_param\n");

  if (stmt->bound_param != NULL)
    free(stmt->bound_param);
  stmt->bound_param = (db_bind_t *)malloc(len * sizeof(db_bind_t));
  if (stmt->bound_param == NULL)
    return 1;
  memcpy(stmt->bound_param, params, len * sizeof(db_bind_t));
  stmt->bound_param_len = len;

  return 0;
}

/* Execute prepared statement */
db_error_t tarantool_drv_execute(db_stmt_t *stmt, db_result_t *rs)
{
  log_text(LOG_DEBUG, "tarantool_drv_execute\n");

  db_conn_t       *con = stmt->connection;
  char            *buf = NULL;
  unsigned int    buflen = 0;
  unsigned int    i, j, vcnt;
  char            need_realloc;
  int             n;
  db_error_t      rc;
  unsigned long   len;

  /* Build the actual query string from parameters list */
  need_realloc = 1;
  vcnt = 0;
  for (i = 0, j = 0; stmt->query[i] != '\0'; i++)
  {
      again:
    if (j+1 >= buflen || need_realloc)
    {
      buflen = (buflen > 0) ? buflen * 2 : 256;
      buf = realloc(buf, buflen);
      if (buf == NULL)
        return DB_ERROR_FATAL;
      need_realloc = 0;
    }

    if (stmt->query[i] != '?')
    {
      buf[j++] = stmt->query[i];
      continue;
    }

    n = db_print_value(stmt->bound_param + vcnt, buf + j, buflen - j);
    if (n < 0)
    {
      need_realloc = 1;
      goto again;
    }
    j += n;
    vcnt++;
  }
  buf[j] = '\0';

  rc = tarantool_drv_query(con, buf, j, rs);

  free(buf);

  return rc;
}

/* Close prepared statement */
int tarantool_drv_close(db_stmt_t *stmt)
{
  log_text(LOG_DEBUG, "tarantool_drv_close\n");

  xfree(stmt->ptr);
  xfree(stmt->query);

  return 0;
}

/* Free result set */
int tarantool_drv_free_results(db_result_t *rs)
{
  log_text(LOG_DEBUG, "tarantool_drv_free_results");

  return 0;
}