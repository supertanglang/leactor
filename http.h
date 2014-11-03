#include "event_lea.h"
#include <features.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/eventfd.h>

#define DEFAULT_HEADER_BUFFER_SIZE (16384)
#define DEFAULT_UPSTREAM_BUFFER_SIZE (16384)
#define HTTP_PARSE_HEADER_DONE 1
typedef struct string {
    int length;
    char *data;
    unsigned hash;
} lt_string_t;

typedef struct hash {
    struct string *key;
    struct string *value;
    unsigned hash_value;
} lt_hash_t;

typedef struct hash_table {
    struct hash **buckets;//buckets array of struct hash pointer
    int size;
} lt_hash_table_t;

static inline void lt_string_assign_new(lt_string_t *p, int length, char *data) 
{
    p->data = data;
    p->length = length;
    p->data[p->length] = '\0';
}

typedef struct conf {
    int efd_distributor;
//    int pfd[2];
    int listen_fd;
    base_t *base;
} conf_t;

struct upstream {
} upstream_t;

typedef struct request {
    lt_buffer_t *header_in;//parse pos
    int state;

//request_line part
    struct string method_name;
    char *method_end;
    int method;
    struct string http_protocol;
    char *http_protocol_end;
    struct string uri;
    struct string unparsed_uri;
    int valid_unparsed_uri;
    char *uri_start;
    char *uri_end;
    char *uri_ext;
    char *port_end;
    char *host_start;
    char *host_end;
    char *args_start;
    char *schema_start;
    char *schema_end;

    int http_version;
    int complex_uri;
    int quoted_uri;
    int plus_in_uri;
    int space_in_uri;
//request_line
    struct string request_line;
    int request_length;
    char *request_start;//request line start
    char *request_end;//request line end
//request_header
    char *header_start; //one header field start
    char *header_end; //one header field end
//request_header_field
    char *header_name_start;// one header field name start
    char *header_name_end;//one header field name end
    struct http_header_element *element_head;
    struct http_header_element *element_tail;
//request_header_part
    int header_hash;
    int lowcase_index;
    int invalid_header; 
    int http_major;
    int http_minor;

    char lowcase_header[32];

    struct upstream *upstream;

    struct lt_memory_pool *header_pool;
    struct lt_memory_pool  header_pool_manager;

    struct lt_memory_pool *chain_pool;
    struct lt_memory_pool  chain_pool_manager;
} request_t;


typedef struct connection {
    int fd;
    struct sockaddr peer_addr;
    struct sockaddr_in peer_addr_in;
    char *peer_addr_c;
    char *peer_port;

    int (*handler)(struct connection *, void *);
    void *handler_arg;

    struct event *ev;

    struct lt_memory_pool *request_pool;
    struct lt_memory_pool request_pool_manager;
    struct request *request_list;

    lt_buffer_t *buf;
    struct lt_memory_pool *buf_pool_manager;

    int status;

    struct connection *next;
    int timeout;
    int close;
} connection_t;

typedef int (*conn_cb_t)(struct connection *, void *);

typedef struct listening {
    int fd;
//    struct addrinfo local_addr;
    char *bind_addr;
    char *bind_port;
    struct sockaddr saddr;

    struct lt_memory_pool *connection_pool;
    struct lt_memory_pool connection_pool_manager;
    struct connection listen_conn;
    struct connection *client_list;
    struct connection *downstream_list;;
    struct event *ev;

    struct lt_memory_pool *buf_pool;
    struct lt_memory_pool buf_pool_manager;
} listening_t;

typedef struct http {
    struct base *base;
//    struct connection *connection_list;
    struct listening listen;
    int core_amount;
    int efd;

} http_t;

void ignore_sigpipe(void);

http_t *http_master_new(base_t *, conf_t *);
http_t *http_worker_new(base_t *, conf_t *);

typedef struct http_header_element {
    int hash;
    struct string key;
    struct string value;
    struct string lowcase_key;
    struct http_header_element *next;
} lt_http_header_element_t;

void lowcase_key_copy_from_origin(struct string *, struct string *);
typedef struct proxy {
    base_t *base;
    connection_t conn[4];//TEMP TODO
    struct lt_memory_pool *buf_pool;
    struct lt_memory_pool buf_pool_manager;
} proxy_t;

proxy_t *proxy_worker_new(base_t *, conf_t *);
int proxy_connect_backend(proxy_t *, conf_t *);
int proxy_connect(connection_t *, conf_t *);
int proxy_send_to_upstream(connection_t *conn, request_t *req);
