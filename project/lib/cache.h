/**
  ***********************************************************************
  * @file       cache.h
  * @author     Nguyen Huynh Dang Khoa-Nguyen Thi Minh Hien
  * @brief      This file contains all the functions prototypes for
  *             cache, cache statistic, and LRU replacement policy.
  ***********************************************************************
  * @attention
  *  
  * <h2><center>&copy; Copyright (c) 2020 Team3_16ES.
  * All rights reserved.</center></h2>
  */


/* Define to prevent recursive inclusion -------------------------------*/
#ifndef CACHE_H
#define CACHE_H
/* Includes ------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "memory_generic.h"

/** @defgroup Function utilities
  * @{
  */
#define SUCCESS     0
#define ERROR       -1

#define TRUE    0
#define FALSE   -1

#define BIT(X)  (1 << X)
/**
  * @}
  */

/* Cache data structures ----------------------------------------------*/
/** @defgroup Cache_data_structures
  * @brief    data struct hierachy: cache->set->line->uint8_t
  * @{
  */

/* Cache line */
/**
  * @brief    Contain tag array(LRU, D, V, tag), and data array
  */
typedef struct line_struct {
    uint16_t tag_array;
    uint8_t* data; 
}line_t;

/* Cache set */
/**
  * @brief    Contain array of cache lines.
  */
typedef struct set_struct {
    line_t* lines;
}set_t;

/* Cache */
/**
  * @brief    Contain array of sets, and others infomation 
  *           for data processing 
  */
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

/**
  * @}
  */

/* Cache request utilities ----------------------------------------------*/
/** @defgroup Cache_requests
  * @brief    command_enum: command argument for cache_request();
  *           return_enum : return type of cache_request();
  * @{
  */

/* Command */
/**
  * @brief    READ_DATA        : Read request to L1 data cache.
  *           WRITE_DATA       : Write request to L1 data cache.
  *           INSTRUCTION_FETCH: Read request to L1 instruction cache.
  *           EVICT            : Evict command from L2 cache.
  *           CLEAR_CACHE      : Clear all state of cache.
  *           PRINT_CONTENT    : Log all statistic to file.
  */
typedef enum command_enum {
    READ_DATA=0,
    WRITE_DATA,
    INSTRUCTION_FETCH,
    EVICT,
    CLEAR_CACHE=8,
    PRINT_CONTENT
}command_t;

/* Return of cache_request(); */
/**
  * @brief    Indicate the result of the request.
  */
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

/**
  * @}
  */

/* Cache statistic -------------------------------------------------------------*/
/** @defgroup Cache_statistic
  * @brief    Statistic of the cache: #reads, #writes, hits, misses, hit rate.
  * @{
  */

/* Statistic data structure */
/**
  * @brief    This data struct hold required statistic of L1 cache.
  */
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
/**
  * @}
  */


/* LRU policy -------------------------------------------------------------*/
/** @defgroup LRU_policy
  * @brief    handle LRU replacement policy.
  * @{
  */


/* LRU mode */
/**
  * @brief    NEW_LINE  : When cache read a new line from L2 and place it in 
  *                       the empty space in the set.
  *           ACCESS    : Any read, write access to the line.
  *           EVICT_LINE: When L2 sends an evict command to L1.
  */
typedef enum LRU_mode{
    NEW_LINE=0,
    ACCESS,
    EVICT_LINE
}LRU_mode_t;

/* LRU query */
/**
  * @brief    query to save only LRU bits and corresponding index.
  *           useful for LRU subfunctions.
  */
typedef struct query_struct {
    int lru;
    int index;
}query_t;
/**
  * @}
  */

/* Cache function prototypes -------------------------------------------------*/
/** @addtogroup Cache_data_structures
  * @{
  */

/* Cache Initialize functions ************************************************/
cache_t* create_cache(int sets_num, int ways_assoc, int line_size);
line_t* create_set(int ways_assoc);
uint8_t* create_line(int line_size);

/* Address data extracting functions *****************************************/
uint32_t get_tag(cache_t cache, uint32_t address);
uint32_t get_set(cache_t cache, uint32_t address);
uint32_t get_bytes_offset(cache_t cache, uint32_t address);
uint16_t get_line_LRU(cache_t cache, uint16_t tag_arr);

/* Cache request subfunctions ************************************************/
int cache_L1_read(cache_t* cache, uint32_t address, uint8_t*data);
int cache_L1_write(cache_t* cache, uint32_t address, uint8_t data);
int cache_L2_evict(cache_t* cache, uint32_t address);
int cache_L1_clear(cache_t* cache);

/* Cache L2 request functions ************************************************/
int cache_L2_read(cache_t* cache, uint32_t address, uint8_t* data);
int cache_L2_write(cache_t* cache, uint32_t address, uint8_t* data);

/**
  * @}
  */

/* Cache statistic function prototypes -------------------------------------------------*/
/** @addtogroup Cache_statistic
  * @{
  */

/* Statistic initialize functions ******************************************************/
cache_stat_t* cache_stat_create(char* cache_name, FILE* log_fp, int mode);
int cache_stat_init(cache_stat_t* stat,char* cache_name, FILE* log_fp, int mode);

/* Statistic activities functions ******************************************************/
int cache_stat_update(cache_stat_t*stat, return_t update, uint32_t address);
int cache_log(cache_stat_t *stat);
int clear_stat(cache_stat_t *stat);

/* Statistic debug functions ***********************************************************/
void print_cache(cache_t cache);

/**
  * @}
  */

/* LRU policy function prototypes -------------------------------------------------*/
/** @addtogroup LRU_policy
  * @{
  */
int cal_LRU(cache_t cache, line_t* lines);
int update_line_LRU(cache_t cache, line_t* lines, uint16_t accessed_lru, LRU_mode_t mode);
/**
  * @}
  */

#endif
