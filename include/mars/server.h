#ifndef HAVE_MARS_SERVER_H
#define HAVE_MARS_SERVER_H

#ifndef IPX_NCP_PTYPE
#define IPX_NCP_PTYPE (0x11U)
#endif

#ifndef IPX_NCP_PORT
#define IPX_NCP_PORT (0x451U)
#endif

#ifndef MARS_SERVER_TYPE_FILE
#define MARS_SERVER_TYPE_FILE (4U)
#endif

#ifndef MARS_SERVER_TYPE_DIR
#define MARS_SERVER_TYPE_DIR (0x278U)
#endif

#ifndef MARS_SERVER_MAX_NCP_CONN
#define MARS_SERVER_MAX_NCP_CONN 50
#endif

#ifndef MARS_SERVER_NCP_SELECT
#define MARS_SERVER_NCP_SELECT 3
#endif

#include <stdint.h>
#include <pthread.h>

#include <netipx/ipx.h>

typedef struct mars_server_volume_t {
    char *name;
    char *path;

    struct mars_server_volume_t *next;
} mars_server_volume_t;

typedef struct mars_server_connection_t {
    int idx;
    
    uint8_t address[6];
    uint32_t network;

    uint8_t seq_no;
    uint8_t task_number;
} mars_server_connection_t;

typedef struct mars_server_bindery_t {
    char *source_name;

    void *ctx;
} mars_server_bindery_t;

typedef struct mars_server_t {
    unsigned short type;

    mars_server_volume_t *volumes;
    mars_server_connection_t *connections[MARS_SERVER_MAX_NCP_CONN];
    pthread_mutex_t session_mtx;

    pthread_t thread;
    int should_run;
    int running;

    int fd;
    pthread_mutex_t send_mtx;

    void *bindery;

    void (*destroy)(struct mars_server_t *);

    struct mars_server_t *next;
} mars_server_t;

int mars_server_init(void);
int mars_server_start(void);
void mars_server_stop(void);
int mars_server_am_a(uint16_t type);

int mars_server_start_ncp(mars_server_t *srv);

#endif