#include "cache.h"


void print_cache(cache_t cache)
{
    printf("Bytes offset: %d bits\n", cache.bytes_num_bits);
    printf("Set: %d bits\n", cache.sets_num_bits);
    printf("Tag: %d bits\n",cache.tags_num_bits);
    printf("Ways: %d\n", cache.ways_assoc);
    printf("Dirty-bit: %x\n", cache.D_BIT);
    printf("Valid-bit: %x\n", cache.V_BIT);
    printf("LRU line mask: %x\n", cache.LRU_line_mask);
    printf("Tag mask: %x\n", cache.tag_mask);
    printf("Set mask: %x\n", cache.set_mask);
    printf("bytes mask: %x\n", cache.bytes_mask);
}
cache_t* create_cache(int sets_num, int ways_assoc, int line_size)
{
    cache_t *cache = (cache_t*)malloc(sizeof(cache_t));
    cache->bytes_num_bits = log2(line_size);
    cache->sets_num_bits = log2(sets_num);
    cache->tags_num_bits = MEMORY_ADDRESS - cache->sets_num_bits - cache->bytes_num_bits;
    cache->ways_assoc = ways_assoc;
    cache->LRU_num_bits = log2(ways_assoc);

    cache->V_BIT = (uint16_t)(cache->tags_num_bits);
    
    cache->D_BIT = cache->V_BIT + 1;
    // cache->V_BIT = BIT(cache->V_BIT);
    // cache->D_BIT = BIT(cache->D_BIT);
    int i;
    //Create LRU_line_mask:
    for(i = 0; i < cache->LRU_num_bits; i++)
    {
        cache->LRU_line_mask |= BIT(i);
    }
    cache->LRU_line_mask = cache->LRU_line_mask << (1 + 1+ cache->tags_num_bits);
    //create bytes offset mask:
    cache->bytes_mask = 0;
    for(i = 0; i < cache->bytes_num_bits; i++)
    {
        cache->bytes_mask |= BIT(i);
    }
    //create set mask:
    cache->set_mask = 0;
    for(i = 0; i < cache->sets_num_bits; i++)
    {
        cache->set_mask |= BIT(i);
    }
    cache->set_mask = cache->set_mask << (cache->bytes_num_bits);

    //create tag_mask for extract tag from address:
    cache->tag_mask = 0;
    for(i = 0; i < cache->sets_num_bits; i++)
    {
        cache->tag_mask |= BIT(i);
    }
    cache->tag_mask = cache->tag_mask << (cache->sets_num_bits + cache->bytes_num_bits);
    // printf("> Log from cache.c:\n");
    // print_cache(*cache);

    //create sets in cache:
    cache->sets = (set_t*)malloc(sets_num * sizeof(set_t));
    
    return cache;
}
line_t* create_set(int ways_assoc)
{
    line_t *lines = (line_t*)malloc((ways_assoc) * sizeof(line_t));
    return lines;
}
uint8_t* create_line(int line_size)
{
    uint8_t *data = (uint8_t*)malloc(line_size * sizeof(uint8_t));
    return data;
}
uint16_t get_line_LRU(cache_t cache, uint16_t tag_arr)
{
    uint16_t lru = tag_arr & cache.LRU_line_mask;
    lru = lru >> (1 + 1 + cache.tags_num_bits);
    return lru;
}
uint32_t get_tag(cache_t cache, uint32_t address)
{
    uint32_t t = address & cache.tag_mask;
    t = t >> (cache.sets_num_bits + cache.bytes_num_bits);
    return t;
}
uint32_t get_set(cache_t cache, uint32_t address)
{
    uint32_t s = address & cache.set_mask;
    s = s >> (cache.bytes_num_bits);
    return s;
}
uint32_t get_bytes_offset(cache_t cache, uint32_t address)
{
    return address & cache.bytes_mask;
}


int update_line_LRU(cache_t cache, line_t* lines)
{   
    int i;
    for(i = 0; i < cache.ways_assoc; i++)
    {
        //only update valid line:
        //otherwise, just leave it.
        if(lines[i].tag_array & BIT(cache.V_BIT))
        {
            //just add 1 to lru bits.
            uint16_t tmp = cache.D_BIT + 1;
            lines[i].tag_array += BIT(tmp);
        }
    }
    return SUCCESS;
}
//Return cache read hit/miss:
int cache_L1_read(cache_t* cache, uint32_t address, uint8_t*data)
{
    
    // print_cache(*cache);
    return_t ret = 0;
    //Split tag, set, bytes offset of an address:
    uint32_t addr_bytes_offset = get_bytes_offset(*cache, address);
    uint32_t addr_set = get_set(*cache, address);
    uint32_t addr_tag = get_tag(*cache, address);

    if(!cache)
    {
        //return error
        printf("Error: Invalid cache access\n");
        return ERROR;
    }

    if((cache->sets)[addr_set].lines == NULL)
    {
        //there is no line in set:
        //read miss: ret |= READ_MISS
        //  - Create set first: ep. create [ways_assoc] lines append to set
        //  - Call cache L2 read to get 1 line(ret |= READ_L2), place it in set
        //      valid = 1, update tag, fake data = 64*DUMMY_BYTES.
        
        ret |= BIT(READ_MISS);
        int i;
        //Create the set: 
        //line_t *lines = (line_t*)malloc((cache->ways_assoc) * sizeof(line_t));
        line_t *lines = create_set(cache->ways_assoc);
        if(lines == NULL)
        {
            printf("Error: Cannot create set of %d line\n", cache->ways_assoc);
            return ERROR;
        }
        int size = pow(2, cache->bytes_num_bits);//should be 64
        for(i = 0; i < cache->ways_assoc; i++)
        {
            lines[i].tag_array = 0;
            lines[i].data = create_line(size);
        }
        (cache->sets)[addr_set].lines = lines;
        //Now the set is not null, but it all empty(all valid == 0, no data in it)
        
        uint8_t *line_data = create_line(size);
        if(cache_L2_read(cache, address, line_data) < 0)
        {
            printf("Error: Read L2 error\n");
            return ERROR;
        }

        ret |= BIT(READ_L2);
        lines[0].tag_array = 0;
        lines[0].tag_array |= BIT(cache->V_BIT); //valid = 1;

        lines[0].tag_array += addr_tag;//update tag
        for(i = 0; i < size; i++)
        {
            (lines[0].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
        }
        //Now return the byte:
        *data = (lines[0].data)[addr_bytes_offset];
        //finish a read miss.
        return ret;
    }
    else{
        //There are already some lines in set
        //Or can be no line(all valid == 0).
        //  - Use for loop to traverse the lines:
        //      count the number of valids;
        //      if valid :
        //              + if line_tag == addr_tag: ret |= READ_HIT; get *data =...
        //              + else: continue to search
        //  - exit the loop: if still no where to get data ret |= READ_MISS
        //              + if count_valids < ways_assoc, call cache_L2_read() to get line, ret |= READ_L2
        //                  place it in the first available space in line.
        //              + else count_valids == ways_assoc, -> need to replace LRU call index
        //                  if lines[LRU] is not dirty -> no need to evict, call cache_L2_read(), ret |= READ_L2
        //                      in place the line in the LRU index.
        //                  else: the lines[LRU] is dirty, call cache_L2_write() to evict, ret |= WRITE_L2;
        //                      call cache_L2_read() to get the line, ret |= READ_L2.
        int i, count = 0, hit = 0;
        line_t *lines = (cache->sets)[addr_set].lines;
        for(i = 0; i < cache->ways_assoc; i++)
        {
            if(lines[i].tag_array & BIT(cache->V_BIT))
            {
                // printf("Attemp hit\n");
                count++;
                uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
                tag_line_mask = ~tag_line_mask;
                uint16_t line_tag = lines[i].tag_array & tag_line_mask;

                if(line_tag == addr_tag){
                    ret |= BIT(READ_HIT);
                    hit = 1;
                    *data = (lines[i].data)[addr_bytes_offset];
                    if(get_line_LRU(*cache, lines[i].tag_array) != 0)
                    {
                        if(update_line_LRU(*cache, lines) < 0)
                        {
                            printf("Error: Cannot update LRU bit.\n");
                            return ERROR;
                        }
                        lines[i].tag_array &= ~cache->LRU_line_mask;
                    }
                    
                    return ret;
                }
            }
        }
        printf("count: %d\n", count);
        if(hit == 0)
        {
            ret |= BIT(READ_MISS);
            if(count < cache->ways_assoc)
            {
                //still have space to fill in.
                int index =0;
                for(i = 0; i < cache->ways_assoc; i++)
                {
                    //if()
                    if(!(lines[i].tag_array & BIT(cache->V_BIT)))
                    {
                        //this index is avaiable
                        index = i;
                        break;
                    }
                }
                
                //Update old LRU bit of old lines, before adding new line
                if(update_line_LRU(*cache, lines) < 0)
                {
                    printf("Error: Cannot update LRU bit.\n");
                    return ERROR;
                }
                // int tmp_lru = get_line_LRU(*cache, lines[0].tag_array);
                // printf("0 lru=%d\n", tmp_lru);
                //Get a line from L2 cache:
                int size = pow(2, cache->bytes_num_bits);//should be 64
                uint8_t* line_data = create_line(size);
                if(cache_L2_read(cache, address, line_data) < 0)
                {
                    printf("Error: Read L2 error\n");
                    return ERROR;
                }
                ret |= BIT(READ_L2);
                lines[index].tag_array |= BIT(cache->V_BIT); //valid = 1;
                uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
                lines[index].tag_array &= tag_line_mask;// clear old tag
                lines[index].tag_array += addr_tag;//update tag
                for(i = 0; i < size; i++)
                {
                    (lines[index].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
                }
                //Now return the byte:
                *data = (lines[index].data)[addr_bytes_offset];
                return ret;
            }
            else{
                //Now the count == ways_assoc, mean that the set is full of lines,
                //Now we need to replace one of the line in the set.

                //Update old LRU bit of old lines, before adding new line
                int index = cal_LRU(*cache, lines);
                if(update_line_LRU(*cache, lines) < 0)
                {
                    printf("Error: Cannot update LRU bit.\n");
                    return ERROR;
                }
                
                printf("lru index: %d\n", index);
                if(!(lines[index].tag_array & BIT(cache->D_BIT)))
                {
                    //Line is not dirty:
                    //Get a line from L2 cache:
                    int size = pow(2, cache->bytes_num_bits);//should be 64
                    uint8_t *line_data = create_line(size);
                    if(cache_L2_read(cache, address, line_data) < 0)
                    {
                        printf("Error: Read L2 error\n");
                        return ERROR;
                    }
                    ret |= BIT(READ_L2);
                    // lines[index].tag_array |= BIT(cache->V_BIT); //valid = 1;
                    uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
                    lines[index].tag_array &= tag_line_mask;// clear old tag
                    lines[index].tag_array += addr_tag;//update tag
                    for(i = 0; i < size; i++)
                    {
                        (lines[index].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
                    }
                }
                else{
                    //the line is dirty, now we need to evict it first:
                    if(cache_L2_write(cache, address, lines[index].data) < 0)
                    {
                        printf("Error: Cannot evict line has addr=%x\n", address);
                        return ERROR;
                    }
                    ret |= BIT(WRITE_L2);

                    //Now we read new line
                    //Get a line from L2 cache:
                    int size = pow(2, cache->bytes_num_bits);//should be 64
                    uint8_t *line_data =create_line(size);
                    if(cache_L2_read(cache, address, line_data) < 0)
                    {
                        printf("Error: Read L2 error\n");
                        return ERROR;
                    }
                    ret |= BIT(READ_L2);
                    // lines[index].tag_array |= BIT(cache->V_BIT); //valid = 1;
                    uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
                    lines[index].tag_array &= tag_line_mask;// clear old tag
                    lines[index].tag_array += addr_tag;//update tag
                    for(i = 0; i < size; i++)
                    {
                        (lines[index].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
                    }
                    return ret;
                }
            }
        }
    }

    return ret;
}
//Return cache write hit/miss:
int cache_L1_write(cache_t* cache, uint32_t address, uint8_t data)
{
    return WRITE_HIT;
}

int cache_L1_clear(cache_t* cache)
{
    free(cache);
    return SUCCESS;
}

int cache_L2_evict(cache_t* cache, uint32_t address)
{

    return EVICT_L2_OK;
}
//LRU index calculation for evicting and replacing:
int cal_LRU(cache_t cache, line_t* lines)
{
    int index = 0;
    int i;
    typedef struct query_struct {
        int lru;
        int index;
    }query_t;
    query_t list_query[cache.ways_assoc];
    int list_query_size = 0;
    for(i=0; i < cache.ways_assoc; i++)
    {
        if(lines[i].tag_array & BIT(cache.V_BIT))
        {
            list_query[list_query_size].index = i;
            list_query[list_query_size].lru = get_line_LRU(cache, lines[i].tag_array);
            list_query_size++;
        }
    }
    query_t max_lru_query;
    max_lru_query.index = list_query[0].index;
    max_lru_query.lru = list_query[0].lru;
    for(i = 1; i < list_query_size; i++)
    {
        if(list_query[i].lru > max_lru_query.lru)
        {
            max_lru_query.index = list_query[i].index;
            max_lru_query.lru = list_query[i].lru;
        }
    }
    index = max_lru_query.index;
    return index;
}
//For simulate L2 cache reference:
int get_invalidate_cache(uint32_t address)
{
    if(address >= DATA_BASE_ADDR && address <= DATA_END_ADDR)
    {
        return 1;//indicate the data cache.
    }
    if(address >= INSTR_BASE_ADDR && address <= INSTR_END_ADDR)
    {
        return 0;//indicate the instruction cache.
    }
    return ERROR;
}
int cache_L2_read(cache_t* cache, uint32_t address, uint8_t* data)
{
    //Just simulate the read from L2 cache:
    //Simply return a line with all dummy byte 0xFF
    int i;
    int size = pow(2, cache->bytes_num_bits);
    if(data == NULL)
    {
        data = (uint8_t*)malloc(size * sizeof(uint8_t));
    }
    for(i = 0; i < size; i++)
    {
        data[i] = DUMMY_BYTE;
    }
    return SUCCESS;
}

int cache_L2_write(cache_t* cache, uint32_t address, uint8_t* data)
{
    //simulate that write to L2 (due to L1 eviction) is always success
    //further code can goes here.
    return SUCCESS;
}
int cache_stat_update(cache_stat_t*stat, return_t update, uint32_t address)
{
    if(update & BIT(READ_HIT))
    {
        stat->read_hits++;
    }
    if(update & BIT(READ_MISS))
    {
        stat->read_misses++;
    }
    if(update & BIT(WRITE_HIT))
    {
        stat->write_hits++;
    }
    if(update & BIT(WRITE_MISS))
    {
        stat->write_misses++;
    }
    if(stat->mode == 2)
    {
        //Activity log mode:
        if(update & BIT(WRITE_L2))
        {
            fprintf(stat->log_file, "[MESSAGE]%s write to L2 %x\n",stat->name, address);
        }
        if(update & BIT(READ_L2))
        {
            fprintf(stat->log_file, "[MESSAGE]%s read from L2 %x\n", stat->name, address);
        }
        if(update & BIT(READ_L2_OWN))
        {
            fprintf(stat->log_file, "[MESSAGE] %s read for Ownership from L2 %x\n", stat->name, address);
        }
    }
    return SUCCESS;
}
int cache_log(cache_stat_t *stat)
{
    FILE *fp = stat->log_file;
    if(stat->count == 0)
    {
        fprintf(fp, "[LOG] Mode: %d\n", stat->mode);
    }
    fprintf(fp, "------------------------------\n");
    fprintf(fp, "> Cache: %s, log: %d\n", stat->name, stat->count);
    fprintf(fp, "> Read hits     : %d\n", stat->read_hits);
    fprintf(fp, "> Read misses   : %d\n", stat->read_misses);
    fprintf(fp, "> Write hits    : %d\n", stat->write_hits);
    fprintf(fp, "> Write misses  : %d\n", stat->write_misses);
    stat->hit_rate = (stat->read_hits + stat->write_hits)* 1.0 /
                         (stat->read_hits + stat->write_hits + stat->write_misses + stat->read_misses); 
    fprintf(fp, "> Hit rate: %.1f%%\n", stat-> hit_rate * 100);
    fprintf(fp, "------------------------------\n");
    stat->count++;
    return SUCCESS;
}

int clear_stat(cache_stat_t *stat)
{
    if(stat == NULL)
    {
        printf("Error: Stat is null.\n");
        return ERROR;
    }
    stat->read_hits = 0;
    stat->read_misses = 0;
    stat->write_hits = 0;
    stat->write_misses = 0;
    stat->hit_rate = 1;
    return SUCCESS;
}