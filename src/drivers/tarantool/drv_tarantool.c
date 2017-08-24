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
#include <string.h>

#include "sb_options.h"
#include "db_driver.h"
#include "../../db_driver.h"

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>

#define xfree(ptr) ({ if (ptr) free((void *)ptr); ptr = NULL; })
#define URI_MAX_LENGTH 100
#define GUEST_USER "guest"


/* Tarantool driver arguments */
static sb_arg_t tarantool_drv_args[] =
{
  SB_OPT("tarantool-host", "Tarantool server host", "localhost", STRING),
  SB_OPT("tarantool-port", "Tarantool server port", "3301", INT),
  SB_OPT("tarantool-user", "Tarantool user", GUEST_USER, STRING),
  SB_OPT("tarantool-password", "Tarantool password", "", STRING),

  SB_OPT_END
};

typedef struct
{
  char  *host;
  char  *port;
  char  *user;
  char  *password;
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

static tarantool_drv_args_t args;

/* Tarantool driver operations */
static int tarantool_drv_init(void);
static int tarantool_drv_describe(drv_caps_t *);
static int tarantool_drv_connect(db_conn_t *);
static int tarantool_drv_disconnect(db_conn_t *);
static db_error_t tarantool_drv_query(db_conn_t *, const char *, size_t,
                                      db_result_t *);
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


/* Local functions */

db_error_t tarantool_execute(struct tnt_stream *, const char *,
                             struct tnt_stream *, db_result_t *);


/* Register Tarantool driver */
int register_driver_tarantool(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&tarantool_driver.listitem, drivers);

  return 0;
}


/* Tarantool driver initialization */
int tarantool_drv_init(void)
{
  args.host = sb_get_value_string("tarantool-host");
  args.port = sb_get_value_string("tarantool-port");
  args.user = sb_get_value_string("tarantool-user");
  args.password = sb_get_value_string("tarantool-password");

  return 0;
}

/* Connect to database */
int tarantool_drv_connect(db_conn_t *sb_conn)
{
  char uri[URI_MAX_LENGTH] = "";

  if (strcmp(args.user, GUEST_USER) && args.password) {
    strcat(uri, args.user);
    strcat(uri, ":");
    strcat(uri, args.password);
    strcat(uri, "@");
  }

  strcat(uri, args.host);
  strcat(uri, ":");
  strcat(uri, args.port);

  log_text(LOG_DEBUG, "TNT_OPT_URI = %s", uri);

  struct tnt_stream * con = tnt_net(NULL);
  tnt_set(con, TNT_OPT_URI, uri);
  tnt_set(con, TNT_OPT_SEND_BUF, 0);
  tnt_set(con, TNT_OPT_RECV_BUF, 0);

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
  struct tnt_stream * con = sb_conn->ptr;

  if (con != NULL) {
    tnt_close(con);
    tnt_stream_free(con); 
  }

  return 0;
}

/* Execute SQL query */
db_error_t tarantool_drv_query(db_conn_t *sb_conn, const char *query,
                               size_t len, db_result_t *rs)
{
  struct tnt_stream * con = sb_conn->ptr;

  struct tnt_stream *params = tnt_object(NULL);

  if (tnt_object_type(params, TNT_SBO_PACKED) == -1)
    return DB_ERROR_FATAL;

  if (tnt_object_add_array(params, 0) == -1)
    return DB_ERROR_FATAL;

  if (tnt_object_container_close(params) == -1)
    return DB_ERROR_FATAL;

  db_error_t err = tarantool_execute(con, query, params, rs);

  tnt_stream_free(params);

  return err;
}

/* Uninitialize driver */
int tarantool_drv_done(void)
{
  return 0;
}

/* Describe database capabilities */
int tarantool_drv_describe(drv_caps_t *caps)
{
  *caps = tarantool_drv_caps;

  char uri[URI_MAX_LENGTH] = "";

  if (strcmp(args.user, GUEST_USER) && args.password) {
    strcat(uri, args.user);
    strcat(uri, ":");
    strcat(uri, args.password);
    strcat(uri, "@");
  }

  strcat(uri, args.host);
  strcat(uri, ":");
  strcat(uri, args.port);

  struct tnt_stream * con = tnt_net(NULL);
  tnt_set(con, TNT_OPT_URI, uri);
  tnt_set(con, TNT_OPT_SEND_BUF, 0);
  tnt_set(con, TNT_OPT_RECV_BUF, 0);

  if (tnt_connect(con) < 0) {
    log_text(LOG_FATAL, "Connection to database failed");
    return 1;
  }

  if (con != NULL) {
    tnt_close(con);
    tnt_stream_free(con);
  }

  return 0;
}

/* Prepare statement */
int tarantool_drv_prepare(db_stmt_t *stmt, const char *query, size_t len)
{
  struct tnt_stream * con = stmt->connection->ptr;

  if (con == NULL)
    return 1;

  /* Use client-side PS */
  stmt->emulated = 1;
  stmt->query = strdup(query);

  return 0;
}

/* Bind parameters for prepared statement */
int tarantool_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
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
  struct tnt_stream *con = stmt->connection->ptr;
  unsigned int      i;
  db_error_t        rc;
  db_bind_t         *var;

  struct tnt_stream *params = tnt_object(NULL);

  if (tnt_object_type(params, TNT_SBO_PACKED) == -1)
    return DB_ERROR_FATAL;

  if (tnt_object_add_array(params, 0) == -1)
    return DB_ERROR_FATAL;

  for (i = 0; i < stmt->bound_param_len; i++)
  {
    var = stmt->bound_param + i;

    switch (var->type) {
    case DB_TYPE_TINYINT:
    case DB_TYPE_SMALLINT:
    case DB_TYPE_BIGINT:
    case DB_TYPE_INT:
      if (tnt_object_add_int(params, *(int64_t *) var->buffer) == -1)
        return DB_ERROR_FATAL;
      break;
    case DB_TYPE_CHAR:
    case DB_TYPE_VARCHAR:
      tnt_object_add_strz(params, (char *)var->buffer);
      break;
    case DB_TYPE_FLOAT:
      tnt_object_add_float(params, *(float *) var->buffer);
      break;
    case DB_TYPE_DOUBLE:
      tnt_object_add_double(params, *(double *) var->buffer);
      break;
    case DB_TYPE_DATETIME:
    case DB_TYPE_TIMESTAMP:
    case DB_TYPE_TIME:
    case DB_TYPE_DATE:
    default:
      log_text(LOG_ALERT, "This var->type isn't implemented yet");
      return DB_ERROR_FATAL;
    }
  }

  if (tnt_object_container_close(params) == -1)
    return DB_ERROR_FATAL;

  rc = tarantool_execute(con, stmt->query, params, rs);

  tnt_stream_free(params);

  return rc;
}

/* Close prepared statement */
int tarantool_drv_close(db_stmt_t *stmt)
{
  xfree(stmt->ptr);
  xfree(stmt->query);

  return 0;
}

/* Free result set */
int tarantool_drv_free_results(db_result_t *rs)
{
  return 0;
}

db_error_t tarantool_execute(struct tnt_stream *con, const char *query,
                             struct tnt_stream *params, db_result_t *rs)
{
  tnt_execute(con, query, strlen(query), params);
  tnt_flush(con);

  struct tnt_reply *reply = tnt_reply_init(NULL);
  con->read_reply(con, reply);

  if (reply->code) {
    log_text(LOG_FATAL, "Failed %lu (%s).\n", reply->code, reply->error);

    rs->counter = SB_CNT_ERROR;

    tnt_reply_free(reply);
    return DB_ERROR_FATAL;
  }

  if (reply->metadata)
    rs->counter = SB_CNT_READ;
  else if (reply->sqlinfo)
    rs->counter = SB_CNT_WRITE;
  else
    rs->counter = SB_CNT_OTHER;

  tnt_reply_free(reply);

  return DB_ERROR_NONE;
}