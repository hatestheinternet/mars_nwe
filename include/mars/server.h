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

#ifndef MARS_SERVER_MAX_CONN
#define MARS_SERVER_MAX_CONN 100
#endif

#ifndef MARS_SERVER_MAX_VOLS
#define MARS_SERVER_MAX_VOLS 255U
#endif

#ifndef MARS_SERVER_NCP_SELECT
#define MARS_SERVER_NCP_SELECT 3
#endif

#ifndef MARS_SERVER_SYS_PATH
#define MARS_SERVER_SYS_PATH "volumes/SYS"
#endif

#include <stdint.h>
#include <pthread.h>

#include <netipx/ipx.h>

typedef struct mars_server_volume_t {
    int idx;

    char *name;
    char *path;

    int is_system;

    struct mars_server_volume_t *next;
} mars_server_volume_t;

typedef struct mars_server_connection_t {
    int idx;
    time_t last_activity;

    uint8_t address[6];
    uint32_t network;

    uint8_t seq_no;
    uint8_t task_number;

    uint16_t packet_sz;
    uint16_t buff_sz;
} mars_server_connection_t;

typedef struct mars_server_bindery_t {
    char *source_name;

    void *ctx;
} mars_server_bindery_t;

typedef struct mars_server_t {
    unsigned short type;

    mars_server_volume_t *volumes[MARS_SERVER_MAX_VOLS];
    pthread_mutex_t vol_mtx;
    
    mars_server_connection_t *connections[MARS_SERVER_MAX_CONN];
    pthread_mutex_t conn_mtx;

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

mars_server_volume_t *mars_server_find_volume(char *name);
mars_server_volume_t *mars_server_get_volume(int idx);

#endif