#ifndef HAVE_MARS_SERVER_H
#define HAVE_MARS_SERVER_H

#ifndef IPX_NCP_PTYPE
#define IPX_NCP_PTYPE (0x11U)
#endif

#ifndef IPX_NCP_PORT
#define IPX_NCP_PORT (0x451U)
#endif

#ifndef MARS_SERVER_TYPE_FILE
#define MARS_SERVER_TYPE_FILE 4U
#endif

#ifndef MARS_SERVER_TYPE_DIR
#define MARS_SERVER_TYPE_DIR 0x278U
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

typedef struct mars_server_volume_ncp_dirent_t {
    uint32_t dirent_no;
    uint32_t dosdir_no;
    uint32_t vol_no;
} mars_server_volume_ncp_dirent_t;

typedef struct mars_server_volume_dirent_t {
    int volume;
    uint32_t handle;

    char *name;
    char *local_path;
    char *netware_path;

    mars_server_volume_ncp_dirent_t ncp_dirent;

    struct mars_server_volume_dirent_t *root;
    struct mars_server_volume_dirent_t *parent;
    struct mars_server_volume_dirent_t *next;
} mars_server_volume_dirent_t;

typedef struct mars_server_volume_t {
    int idx;

    char *name;
    char *path;

    int is_system;

    mars_server_volume_dirent_t *dirents;
    pthread_mutex_t dirents_mtx;
} mars_server_volume_t;

typedef struct mars_ncp_service_search_request_sequence_t {
    uint8_t vol_no;
    uint32_t dirent __attribute__ ((packed));
    uint32_t sequence __attribute__ ((packed));
} mars_ncp_service_search_request_sequence_t;

typedef struct mars_server_connection_t {
    int idx;
    time_t last_activity;

    uint8_t address[6];
    uint32_t network;

    uint8_t seq_no;
    uint8_t task_number;

    uint16_t packet_sz;
    uint16_t buff_sz;

    mars_server_volume_dirent_t *dir_handles[16];
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
} mars_server_t;

int mars_server_init(void);
int mars_server_start(void);
void mars_server_stop(void);

mars_server_volume_t *mars_server_find_volume(char *name);
mars_server_volume_t *mars_server_get_volume(int idx);

mars_server_volume_dirent_t *mars_server_dirent_walk(mars_server_volume_t *vol, char *path);
mars_server_volume_dirent_t *mars_server_dirent_get(mars_server_volume_t *vol, int directory);

mars_server_volume_dirent_t *mars_server_dirent_get_or_create(mars_server_volume_t *vol, char *name, mars_server_volume_dirent_t *parent);
mars_server_volume_dirent_t *mars_server_dirent_from_path(mars_server_volume_t *vol, char *path);

#endif