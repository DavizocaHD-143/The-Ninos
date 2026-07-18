#ifndef VXL_H
#define VXL_H
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
 
#define VXL_X 1024
#define VXL_Y 1024
#define VXL_Z 256
#define VXL_MAP_SIZE (VXL_X * VXL_Y * VXL_Z)
 

typedef struct { double x, y, z; } vxl_dpoint3d;
 
typedef struct {
    vxl_dpoint3d ipos;
    vxl_dpoint3d istr;
    vxl_dpoint3d ihei;
    vxl_dpoint3d ifor;
    uint8_t* geom;   
    uint32_t* color; 
} vxl_map_t;
 
static vxl_map_t g_server_map = {0};
 
static int vxl_init_map() {
    g_server_map.geom = (uint8_t*)malloc(VXL_MAP_SIZE);
    g_server_map.color = (uint32_t*)calloc(VXL_MAP_SIZE, sizeof(uint32_t));
 
    if (!g_server_map.geom || !g_server_map.color) {
        return 0; 
    }
    return 1;
}
 

static void vxl_free_map() {
    if (g_server_map.geom) free(g_server_map.geom);
    if (g_server_map.color) free(g_server_map.color);
    g_server_map.geom = NULL;
    g_server_map.color = NULL;
}
 
static inline void vxl_set_geom(int x, int y, int z, uint8_t issolid) {
    g_server_map.geom[x + y * VXL_X + z * VXL_X * VXL_Y] = issolid;
}
 
static inline uint8_t vxl_get_geom(int x, int y, int z) {
    return g_server_map.geom[x + y * VXL_X + z * VXL_X * VXL_Y];
}
 
static inline void vxl_set_color(int x, int y, int z, uint32_t argb) {
    g_server_map.color[x + y * VXL_X + z * VXL_X * VXL_Y] = argb;
}
 
static inline uint32_t vxl_get_color(int x, int y, int z) {
    return g_server_map.color[x + y * VXL_X + z * VXL_X * VXL_Y];
}
 
static int vxl_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
 
    uint32_t magic, x_size, y_size;
    if (fread(&magic, 4, 1, f) != 1 || magic != 0x09072000) { fclose(f); return -2; }
    if (fread(&x_size, 4, 1, f) != 1 || x_size != 1024) { fclose(f); return -3; }
    if (fread(&y_size, 4, 1, f) != 1 || y_size != 1024) { fclose(f); return -3; }
 
    fread(&g_server_map.ipos, sizeof(vxl_dpoint3d), 1, f);
    fread(&g_server_map.istr, sizeof(vxl_dpoint3d), 1, f);
    fread(&g_server_map.ihei, sizeof(vxl_dpoint3d), 1, f);
    fread(&g_server_map.ifor, sizeof(vxl_dpoint3d), 1, f);
 
    if (!g_server_map.geom && !vxl_init_map()) { fclose(f); return -4; }
 
    long current_pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, current_pos, SEEK_SET);
    long rle_size = file_size - current_pos;
 
    uint8_t *vbuf = (uint8_t *)malloc(rle_size);
    if (!vbuf) { fclose(f); return -5; }
    if (fread(vbuf, rle_size, 1, f) != 1) { free(vbuf); fclose(f); return -6; }
    fclose(f);
 
    memset(g_server_map.geom, 1, VXL_MAP_SIZE);
 
    uint8_t *v = vbuf;
    for (int y = 0; y < VXL_Y; y++) {
        for (int x = 0; x < VXL_X; x++) {
            int z = 0;
            while (1) {
                for (int i = z; i < v[1]; i++) vxl_set_geom(x, y, i, 0); 
                for (z = v[1]; z <= v[2]; z++) {
                    vxl_set_color(x, y, z, *(uint32_t *)&v[(z - v[1] + 1) << 2]);
                }
                if (!v[0]) break;[cite: 1]
                z = v[2] - v[1] - v[0] + 2;[cite: 1]
                v += v[0] * 4;[cite: 1]
                int temp_z = z + v[3];
                for (; z < temp_z; z++) {
                    vxl_set_color(x, y, z, *(uint32_t *)&v[(z - v[3]) << 2]);[cite: 1]
                }
            }
            v += ((((long)v[2]) - ((long)v[1]) + 2) << 2);[cite: 1]
        }
    }
 
    free(vbuf);
    return 0; 
}
 
static int vxl_save(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
 
    uint32_t magic = 0x09072000;
    uint32_t size_x = 1024;
    uint32_t size_y = 1024;
    fwrite(&magic, 4, 1, f);
    fwrite(&size_x, 4, 1, f);
    fwrite(&size_y, 4, 1, f);
 
    fwrite(&g_server_map.ipos, sizeof(vxl_dpoint3d), 1, f);
    fwrite(&g_server_map.istr, sizeof(vxl_dpoint3d), 1, f);
    fwrite(&g_server_map.ihei, sizeof(vxl_dpoint3d), 1, f);
    fwrite(&g_server_map.ifor, sizeof(vxl_dpoint3d), 1, f);
 
    uint8_t col_buf[4096];
 
    for (int y = 0; y < VXL_Y; y++) {
        for (int x = 0; x < VXL_X; x++) {
            int z = 0;
            uint8_t *v = col_buf;
 
            while (z < VXL_Z) {
                while (z < VXL_Z && vxl_get_geom(x, y, z) == 0) z++;
                if (z >= VXL_Z) break;
 
                int z_start = z;
                while (z < VXL_Z && vxl_get_geom(x, y, z) != 0) z++;
                int z_end = z - 1;
 
                v[0] = 0; 
                v[1] = z_start;
                v[2] = z_end;
                v[3] = 0; 
 
                uint8_t *v_len = v; 
                v += 4;
 
                for (int i = z_start; i <= z_end; i++) {
                    *(uint32_t *)v = vxl_get_color(x, y, i);
                    v += 4;
                }
 
                int next_z = z;
                while (next_z < VXL_Z && vxl_get_geom(x, y, next_z) == 0) next_z++;
                if (next_z < VXL_Z) {
                    v_len[0] = (v - v_len) / 4; 
                }
            }
 
            if (v == col_buf) {
                v[0] = 0; v[1] = 255; v[2] = 254; v[3] = 0;
                v += 4;
            }
 
            fwrite(col_buf, 1, v - col_buf, f);
        }
    }
 
    fclose(f);
    return 0; 
 
#endif // VXL_H
