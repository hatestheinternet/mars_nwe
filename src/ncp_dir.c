#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/statvfs.h>

#include <mars/ncp.h>

typedef struct mars_ncp_dir_request_t {
    mars_ncp_service_request_t req;
    uint16_t sub_func_struct_len __attribute__ ((packed));
    uint8_t sub_func;
} mars_ncp_dir_request_t;

#pragma region Function 0x16:0x15 - Get Volume Info with Handle

typedef struct mars_ncp_service_volume_from_dir_request_t {
    mars_ncp_dir_request_t req;
    uint8_t handle;
} mars_ncp_service_volume_from_dir_request_t;

typedef struct mars_ncp_service_volume_from_dir_response_t {
    mars_ncp_response_t resp;
    uint16_t sect_per_cluster __attribute__ ((packed));
    uint16_t clusters_ttl __attribute__ ((packed));
    uint16_t clusters_avail __attribute__ ((packed));
    uint16_t dir_slot_ttl __attribute__ ((packed));
    uint16_t dir_slot_avail __attribute__ ((packed));
    uint8_t vol_name[16];
    uint16_t removable __attribute__ ((packed));
} mars_ncp_service_volume_from_dir_response_t;

int _mars_ncp_dir_volume_info_with_handle(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_service_volume_from_dir_request_t *req = (mars_ncp_service_volume_from_dir_request_t *)buff;
    mars_server_volume_dirent_t *dirent = conn->dir_handles[req->handle];
    mars_server_volume_t *vol = mars_server_get_volume(dirent->volume);
    struct statvfs vfsbuf;
    
    mars_ncp_service_volume_from_dir_response_t resp;
    mars_ncp_response_prepare(conn, &resp, sizeof(resp));

    unsigned long bytes_total, bytes_avail;

    // TODO Proper response?
    if( !dirent ) {
        fprintf(stderr, "mars_ncp_dir_volume_info_with_handle: Unknown handle %02X\n", req->handle);
        return 0;
    }

    if( statvfs(dirent->local_path, &vfsbuf) == -1 ) {
        fprintf(stderr, "mars_ncp_dir_volume_info_with_handle: statvfs(%s) failed\n", dirent->local_path);
        return 0;
    }

    bytes_total = ((vfsbuf.f_blocks * vfsbuf.f_frsize) / 512) / 64;
    bytes_avail = ((vfsbuf.f_bavail * vfsbuf.f_frsize) / 512) / 64;

    resp.sect_per_cluster = htons(64U);
    resp.clusters_ttl = bytes_total & 0xFFFF;
    resp.clusters_avail = bytes_avail & 0xFFFF;
    memcpy(&resp.vol_name, vol->name, strlen(vol->name)>15?15:strlen(vol->name));

    return mars_ncp_send(srv, &resp, sizeof(resp), sipx);
}

#pragma endregion

int mars_ncp_service_directory(mars_server_t *srv, mars_server_connection_t *conn, struct sockaddr_ipx *sipx, uint8_t *buff, int sz) {
    mars_ncp_dir_request_t *req = (mars_ncp_dir_request_t *)buff;

    switch( req->sub_func ) {
        case 0x15:
            return _mars_ncp_dir_volume_info_with_handle(srv, conn, sipx, buff, sz);
            break;
    }

    return 0;
}