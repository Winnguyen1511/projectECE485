#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cache.h"


//The rest is instruction memory:
#define INSTR_BASE_ADDR 0x0
#define INSTR_END_ADDR  0xffffff
#define INSTRUCTION_CACHE               0
#define INSTRUCTION_CACHE_ASSOC_WAYS    2
#define INSTRUCTION_CACHE_NUM_SETS      16*K
#define INSTRUCTION_CACHE_LINE_SIZE     64

//data memory from 0-> 3/4 * 2^32 -1
#define DATA_BASE_ADDR  0x1000000
#define DATA_END_ADDR   0xffffffff
#define DATA_CACHE                      1
#define DATA_CACHE_ASSOC_WAYS           4
#define DATA_CACHE_NUM_SETS             16*K
#define DATA_CACHE_LINE_SIZE            64

#define MAX_SIZE    512
char* log_dir="log/";
char* log_file_name = "log";
// char* trace_file_name = "trace.txt" 
FILE *trace_file;
FILE *log_file = NULL;
cache_stat_t instruction_cache_stat, data_cache_stat;
cache_t *instruction_cache, *data_cache;

int sysInit(char*trace_file_path,char*log_file_name, int mode);
void sysDenit(void);
int get_invalidate_cache(uint32_t address);

//Receive all request to cache L1:
int cache_request(int command, uint32_t address,
                    cache_stat_t* instr_stat,
                    cache_stat_t* data_stat);

char *currTime(const char *format);
int main(int argc, char**argv)
{
    char*trace_file_path;
    int mode;
    if(argc < 2)
    {
        printf("Error: Not enough arguments.\n");
        printf("Usage: %s [input_trace] [mode(optional)]\n", argv[0]);
        return ERROR;
    }
    trace_file_path = argv[1];
    if(argc == 2)
    {    
        mode = 1;
    }
    else if (argc == 3)
    {
        if(strlen(argv[2]) > 1 || ((strcmp(argv[2], "1") != 0) && (strcmp(argv[2],"2")!= 0)))
        {
            printf("Error: Wrong arguments format.\n");
            printf("Usage: %s [input_trace] [mode(optional)]\n", argv[0]);
            return ERROR;
        } 
        mode = argv[2][0] - '0';
        
    }
    printf("Mode: %d\n",mode);
    //Initialize 2 cache, trace file, log file.
    int ret = sysInit(trace_file_path,log_file_name, mode); 
    if (ret == ERROR)
    {
        printf("Error: System Initialize failed!\n");
        return ERROR;
    }
    while(!feof(trace_file))
    {
        int command;
        uint32_t address;
        fscanf(trace_file, "%d %x",&command, &address);
        // printf("%d %x\n",command, address);
        // printf("Requesting...\n");
        int ret = cache_request(command, address, &instruction_cache_stat, &data_cache_stat);
        // printf("Request done.\n");
        if(ret == ERROR)
        {
            printf("Error: Internal error while simulating.\n");
            return ERROR;
        }
    }
    sysDenit();
    printf("> Finished.\n");
    return SUCCESS;
}

int sysInit(char*trace_file_path,char*log_file_name, int mode)
{
    printf("> Sys Init...\n");
    instruction_cache = create_cache(INSTRUCTION_CACHE_NUM_SETS,
                                     INSTRUCTION_CACHE_ASSOC_WAYS,
                                     INSTRUCTION_CACHE_LINE_SIZE);
    if(instruction_cache == NULL)
    {
        printf("Error: Cannot create instruction cache.\n");
        return ERROR;
    }
    data_cache = create_cache(DATA_CACHE_NUM_SETS,
                                DATA_CACHE_ASSOC_WAYS,
                                DATA_CACHE_LINE_SIZE);
    if(data_cache == NULL)
    {
        printf("Error: Cannot create data cache.\n");
        return ERROR;
    }

    trace_file = fopen(trace_file_path, "r");
    if(trace_file == NULL)
    {
        printf("Error: Failed to open file %s.\n", trace_file_path);
        return ERROR;
    }
    char *time_label = currTime("%F_%X");
    char log_path[MAX_SIZE] = {0};
    strcat(log_path, log_dir);
    strcat(log_path, log_file_name);
    strcat(log_path, time_label);
    strcat(log_path, ".log");
    printf("%s\n", log_path);
    log_file = fopen(log_path, "w");
    
    if(cache_stat_init(&instruction_cache_stat, "Instruction", log_file, mode) < 0)
    {
        printf("Error: Instruction stat init failed\n");
        return ERROR;
    }
    if(cache_stat_init(&data_cache_stat, "Data", log_file, mode) < 0)
    {
        printf("Error: Data stat init failed\n");
        return ERROR;
    }

    return SUCCESS;
}
void sysDenit(void)
{
    printf("> Sys Denit...\n");
    free(instruction_cache);
    free(data_cache);
    fclose(trace_file);
    if(log_file!= NULL)
    {
        fclose(log_file);
    }
}

int get_invalidate_cache(uint32_t address)
{
    if(address >= DATA_BASE_ADDR && address <= DATA_END_ADDR)
    {
        return DATA_CACHE;//indicate the data cache.
    }
    if(address >= INSTR_BASE_ADDR && address <= INSTR_END_ADDR)
    {
        return INSTRUCTION_CACHE;//indicate the instruction cache.
    }
    return ERROR;
}

//Handle request from trace file:
int cache_request(int command, uint32_t address, cache_stat_t* instruction_cache_stat, cache_stat_t* data_cache_stat)
{
    int update;
    // printf("%d %x\n",command, address);
    if(command == READ_DATA)
    {
        uint8_t data;
        update = cache_L1_read(data_cache, address,&data);
        if(cache_stat_update(data_cache_stat, update, address) < 0)
        {
            printf("Error: Stat update failed code=%d!\n", update);
            return ERROR;
        }
        return SUCCESS;
    }
    else if(command == WRITE_DATA)
    {
        uint8_t data = DUMMY_BYTE;
        update = cache_L1_write(data_cache, address, data);
        if(cache_stat_update(data_cache_stat, update, address) < 0)
        {
            printf("Error: Stat update failed code=%d!\n", update);
            return ERROR;
        }

    }
    else if(command == INSTRUCTION_FETCH)
    {
        uint8_t data;
        update = cache_L1_read(instruction_cache, address, &data);
        if(cache_stat_update(instruction_cache_stat, update, address) < 0)
        {
            printf("Error: Stat update failed code=%d!\n", update);
            return ERROR;
        }
    }
    else if(command == EVICT)
    {
        int cache_num = get_invalidate_cache(address);
        cache_t *cache;
        cache_stat_t* stat;
        if(cache_num == DATA_CACHE)
        {
            cache = data_cache;
            stat = data_cache_stat;
            // update = cache_L2_evict(data_cache, address);
        }
        else if(cache_num == INSTRUCTION_CACHE)
        {
            cache = instruction_cache;
            stat = instruction_cache_stat;
            // update = cache_L2_evict(instruction_cache, address);
        }
        else{
            printf("Error: Unknown cache.\n");
            return ERROR;
        }
        update = cache_L2_evict(cache, address);
        if(cache_stat_update(stat, update, address) < 0)
        {
            printf("Error: Stat update failed code=%d!\n", update);
            return ERROR;
        }
        return SUCCESS;
        // update = cache_L2_evict()
    }
    else if(command == CLEAR_CACHE)
    {
        if(cache_L1_clear(data_cache) < 0)
        {
            printf("Error: Cannot clear cache: %s\n", data_cache_stat->name);
            return ERROR;
        }

        if(clear_stat(data_cache_stat) < 0)
        {
            printf("Error: Cannot clear cache statistics: %s\n", data_cache_stat->name);
            return ERROR;
        }

        if(cache_L1_clear(instruction_cache) < 0)
        {
            printf("Error: Cannot clear cache state: %s\n", instruction_cache_stat->name);
            return ERROR;
        }

        if(clear_stat(instruction_cache_stat) < 0)
        {
            printf("Error: Cannot clear cache statistics: %s\n", instruction_cache_stat->name);
            return ERROR;
        }
        return SUCCESS;
    }
    else if(command == PRINT_CONTENT)
    {
        printf("Logged data cache at %d\n",data_cache_stat->count);
        if(cache_log(data_cache_stat) < 0)
        {
            printf("Error: Cannot log cache state: %s\n", data_cache_stat->name);
            return ERROR;
        }
        printf("Logged instruction cache at %d\n",instruction_cache_stat->count);
        if(cache_log(instruction_cache_stat) < 0)
        {
            printf("Error: Cannot log cache state: %s\n", instruction_cache_stat->name);
            return ERROR;
        }
        return SUCCESS;
    }
    else
    {
        //do nothing;
        printf("Error: Unknown command.\n");
        sysDenit();
        return ERROR;
    }
    
    return SUCCESS;
}

char *currTime(const char *format)
{
    static char buf[100];
    time_t t;
    size_t s;
    struct tm *tm;
    /* Nonreentrant */
    t = time(NULL);
    tm = localtime(&t);
    if (tm == NULL)
    return NULL;
    s = strftime(buf, 100, (format != NULL) ? format : "%c", tm);
    return (s == 0) ? NULL : buf;
}