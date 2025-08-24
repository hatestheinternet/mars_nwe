#ifndef HAVE_MARS_SERVER_H
#define HAVE_MARS_SERVER_H

#ifndef MARS_SERVER_PORT_NCP
#define MARS_SERVER_PORT_NCP (0x451U)
#endif

#ifndef MARS_SERVER_TYPE_FILE
#define MARS_SERVER_TYPE_FILE (4U)
#endif

#ifndef MARS_SERVER_TYPE_DIR
#define MARS_SERVER_TYPE_DIR (0x278U)
#endif

typedef struct mars_server_volume_t {
    char *name;
    char *path;

    struct mars_server_volume_t *next;
} mars_server_volume_t;

typedef struct mars_server_bindery_t {
    char *source_name;

    void *ctx;
} mars_server_bindery_t;

typedef struct mars_server_t {
    unsigned short type;

    mars_server_volume_t *volumes;
    void *bindery;

    void (*destroy)(struct mars_server_t *);

    struct mars_server_t *next;
} mars_server_t;

int mars_server_init(void);
void mars_server_stop(void);
int mars_server_am_a(unsigned short type);

#endif