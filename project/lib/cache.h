#ifndef CACHE_H
#define CACHE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory_generic.h"
#include <math.h>

#define SUCCESS     0
#define ERROR       -1

#define TRUE    0
#define FALSE   -1

#define BIT(X)  (1 << X)

typedef enum command_enum {
    READ_DATA=0,
    WRITE_DATA,
    INSTRUCTION_FETCH,
    EVICT,
    CLEAR_CACHE=8,
    PRINT_CONTENT
}command_t;

typedef enum return_enum {
    READ_HIT=0,
    READ_MISS,
    WRITE_HIT,
    WRITE_MISS,
    WRITE_L2,
    READ_L2,
    READ_L2_OWN,
    EVICT_L2_OK,
    EVICT_L2_ERROR
}return_t;

typedef struct line_struct {
    uint16_t tag_array;
    uint8_t* data; 
}line_t;

typedef struct set_struct {
    line_t* lines;
}set_t;

typedef struct cache_struct {
    int bytes_num_bits;
    int sets_num_bits;
    int tags_num_bits;
    int ways_assoc;
    int LRU_num_bits;
    // int line_size;

    uint16_t D_BIT;
    uint16_t V_BIT;
    uint16_t LRU_line_mask;
    uint32_t tag_mask;
    uint32_t set_mask;
    uint32_t bytes_mask;
    set_t* sets;
}cache_t;


typedef struct cache_stat_struct {
    int count;
    char*name;
    int mode;
    FILE *log_file;
    int read_hits;
    int read_misses;
    int write_hits;
    int write_misses;
    double hit_rate;
}cache_stat_t;

void print_cache(cache_t cache);
cache_t* create_cache(int sets_num, int ways_assoc, int line_size);
line_t* create_set(int ways_assoc);
uint8_t* create_line(int line_size);

uint32_t get_tag(cache_t cache, uint32_t address);
uint32_t get_set(cache_t cache, uint32_t address);
uint32_t get_bytes_offset(cache_t cache, uint32_t address);

//LRU calculation:
uint16_t get_line_LRU(cache_t cache, uint16_t tag_arr);
int update_line_LRU(cache_t cache, line_t* lines);
//Return cache read hit/miss:
int cache_L1_read(cache_t* cache, uint32_t address, uint8_t*data);
//Return cache write hit/miss:
int cache_L1_write(cache_t* cache, uint32_t address, uint8_t data);
//update line LRU bit
int update_line_LRU(cache_t cache, line_t* lines);

//Clear cache:
int cache_L1_clear(cache_t* cache);
//Find which cache need to receive the invalidate command from L2:

//
int cache_L2_evict(cache_t* cache, uint32_t address);


//LRU index calculation for evicting and replacing:
int cal_LRU(cache_t cache, line_t* lines);
//For simulate L2 cache reference:
int cache_L2_read(cache_t* cache, uint32_t address, uint8_t* data);
// int cache_L2_write_line(cache_t* cache, )
int cache_L2_write(cache_t* cache, uint32_t address, uint8_t* data);

//Log activity:
cache_stat_t* cache_stat_create(char* cache_name, FILE* log_fp, int mode);
int cache_stat_init(cache_stat_t* stat,char* cache_name, FILE* log_fp, int mode);
int cache_stat_update(cache_stat_t*stat, return_t update, uint32_t address);
int cache_log(cache_stat_t *stat);
int clear_stat(cache_stat_t *stat);

#endif
