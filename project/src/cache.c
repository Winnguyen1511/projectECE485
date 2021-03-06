/**
  ***********************************************************************
  * @file       cache.c
  * @author     Nguyen Huynh Dang Khoa-Nguyen Thi Minh Hien
  * @brief      Cache driver.
  @verbatim
  =======================================================================
                    #### How to use this driver ####
  =======================================================================
    [..]
    The common functions contains a set of APIs that can control the
    cache, cache statistic, and LRU replacement policy.
    [..]
    The driver contains 3 APIs' categories:
        (+) Cache controls APIs.
        (+) Cache statistic APIs.
        (+) LRU policy APIs.
    
    [..] Cache controls APIs:
        (#) Create a pointer of cache_t first by create_cache().
            (++) Configure number of sets.
            (++) Configure associativity (N-way).
            (++) Configure line size (bytes).

        (#) Do not use the create_set() and create_line(), unless you
            want to control the cache manually. Otherwise, just use
            cache APIs to control the system.

        (#) There are others address infomation extract APIs.
            You can use these APIs freely for your purposes.
            (++) Get tag from address: get_tag().
            (++) Get set index from address: get_set().
            (++) Get bytes offset from address: get_bytes_offset().
            (++) Get LRU bits from any line in cache: get_line_LRU().

        (#) Control the activities of cache by these APIs:
            (++) Read request       :       cache_L1_read().
            (++) Write request      :       cache_L1_write().
            (++) Clear cache        :       cache_L1_clear().
            (++) L2 evict command   :       cache_L2_evict().
            (++) Read from L2       :       cache_L2_read().
            (++) Write to L2        :       cache_L2_write().
    
    [..] Cache statistic APIs:
        (#) Create a pointer of stat by cache_stat_create().
            or create an instance, then initialize it by cache_stat_init().
        (#) The stat instance need to be update after each access of cache.
            Call cache_stat_update(). to update the statistic.
            Note: Read more about this to use.
        (#) Log the statistic to log file by cache_log().
        (#) Clear statistic by clear_stat().
        (#) Cache statistic APIs:
            (++) Create stat        :       cache_stat_create().
            (++) Init stat          :       cache_stat_init().
            (++) Stat update        :       cache_stat_update().
            (++) Log to file        :       cache_log().
            (++) Clear stat         :       clear_stat().
            (++) Print cache state  :       print_cache().
    [..] LRU replacement APIs:
        (#) Basically these APIs is reserved, should not be used freely.
            All use of these APIs are inside cache request APIs.
            (++) Get the LRU replacement index: calLRU().
            (++) update the LRU bits of lines : update_line_LRU().

  @endverbatim
  ***********************************************************************
  * @attention
  *  
  * <h2><center>&copy; Copyright (c) 2020 Team3_16ES.
  * All rights reserved.</center></h2>
  */
/* Includes ------------------------------------------------------------*/
#include "cache.h"


/* Cache function prototypes -------------------------------------------------*/
/** @addtogroup Cache_data_structures
  * @{
  */

/* Cache Initialize functions ************************************************/
/**
  * @brief      Create a pointer of cache and return it for use.
  * @param      sets_num: number of set in the cache.
  * @param      ways_assoc: associativity of cache, for example: 4-way -> ways_assoc == 4
  * @param      line_size: line(block) size, for example: 64-byte line -> line_size == 64
  * @retval     pointer to the cache instance
  */
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

/**
  * @attention  RESTRICTED API
  * @brief      Create an array of lines and return it for use.
  * @param      ways_assoc: associativity of cache, for example: 4-way -> ways_assoc == 4
  * @retval     pointer to the lines array.
  */
line_t* create_set(int ways_assoc)
{
    line_t *lines = (line_t*)malloc((ways_assoc) * sizeof(line_t));
    return lines;
}

/**
  * @attention  RESTRICTED API
  * @brief      Create an array of data and return it for use.
  * @param      line_size: line(block) size, for example: 64-byte line -> line_size == 64
  * @retval     pointer to the data array.
  */
uint8_t* create_line(int line_size)
{
    uint8_t *data = (uint8_t*)malloc(line_size * sizeof(uint8_t));
    return data;
}

/* Address data extracting functions *****************************************/

/**
  * @brief      extract tag part from address
  * @param      cache: cache instance
  * @param      address: input address
  * @retval     tag from the address.
  */
uint32_t get_tag(cache_t cache, uint32_t address)
{
    uint32_t t = address & cache.tag_mask;
    t = t >> (cache.sets_num_bits + cache.bytes_num_bits);
    return t;
}

/**
  * @brief      extract set part from address
  * @param      cache: cache instance
  * @param      address: input address
  * @retval     set from the address.
  */
uint32_t get_set(cache_t cache, uint32_t address)
{
    uint32_t s = address & cache.set_mask;
    s = s >> (cache.bytes_num_bits);
    return s;
}

/**
  * @brief      extract bytes offset part from address
  * @param      cache: cache instance
  * @param      address: input address
  * @retval     bytes offset from the address.
  */
uint32_t get_bytes_offset(cache_t cache, uint32_t address)
{
    return address & cache.bytes_mask;
}

/**
  * @brief      extract LRU part from tag array of a line in cache
  * @param      cache: cache instance
  * @param      tag_array: tag array of a line
  * @retval     LRU bits from the line.
  */
uint16_t get_line_LRU(cache_t cache, uint16_t tag_arr)
{
    uint16_t lru = tag_arr & cache.LRU_line_mask;
    lru = lru >> (1 + 1 + cache.tags_num_bits);
    return lru;
}


/* Cache request subfunctions ************************************************/

/**
  * @brief      Read request to L1 cache.
  * @param      cache: pointer to cache instance.
  * @param      address: byte address
  * @param      data: pointer to return data.
  *             Note: this is actually the return data to deliver to CPU.
  * @retval     status of the read request:
  *                 @arg    return_t
  */
int cache_L1_read(cache_t* cache, uint32_t address, uint8_t*data)
{
    //int j;
    
    
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
        // for(i = 0; i < cache->ways_assoc; i++)
        // {
        //     printf("lru:x");
        // }
        // printf("\n");
        //Create the set: 
        
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
        // for(j = 0; j < 4; j++)
        // {
        //     if(lines[j].tag_array & BIT(cache->V_BIT))
        //         printf("lru:%d", get_line_LRU(*cache, lines[j].tag_array));
        //     else{
        //         printf("lru:x");
        //     }
        // }
        // printf("\n");
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
                    uint16_t accessed_lru = get_line_LRU(*cache, lines[i].tag_array);
                    if(update_line_LRU(*cache, lines, accessed_lru, ACCESS) < 0)
                    {
                        printf("Error: Cannot update LRU with addr=%x\n", address);
                        return ERROR;
                    }
                    return ret;
                }
            }
        }
        // printf("count: %d\n", count);
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
                if(update_line_LRU(*cache, lines,0, NEW_LINE) < 0)
                {
                    printf("Error: Cannot update LRU with addr=%x.\n", address);
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
                uint16_t accessed_lru = get_line_LRU(*cache, lines[index].tag_array);
                if(update_line_LRU(*cache, lines, accessed_lru, ACCESS) < 0)
                {
                    printf("Error: Cannot update LRU with addr=%x.\n", address);
                    return ERROR;
                }
                // printf("lru index: %d\n", index);
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
                    lines[index].tag_array |= BIT(cache->V_BIT); //valid = 1;
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
            }
        }
    }
    //line_t *tmp_set = (cache->sets)[addr_set].lines;
    
    return ret;
}

/**
  * @brief      Write request to L1 cache.
  * @param      cache: pointer to cache instance.
  * @param      address: byte address
  * @param      data: byte value.
  *             Note: this is the data that CPU write to cache line.
  * @retval     status of the write request:
  *                 @arg    return_t
  */
int cache_L1_write(cache_t* cache, uint32_t address, uint8_t data)
{
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
        //write miss: ret |= BIT(WRITE_MISS)
        //  - Create set first: ep. create [ways_assoc] lines append to set
        //  - Call cache L2 read to get 1 line(ret |= BIT(READ_L2_OWN)), place it in set
        //      valid = 1, update tag, fake data = 64*DUMMY_BYTES.
        //      write a new dummy byte to bytes_offset, dirty = 1;
        
        ret |= BIT(WRITE_MISS);
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

        ret |= BIT(READ_L2_OWN);
        lines[0].tag_array = 0;
        lines[0].tag_array |= BIT(cache->V_BIT); //valid = 1;

        lines[0].tag_array += addr_tag;//update tag
        for(i = 0; i < size; i++)
        {
            (lines[0].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
        }

        //Now we write a new bytes and set dirty = 1:
        (lines[0].data)[addr_bytes_offset] = data;
        lines[0].tag_array |= BIT(cache->D_BIT);
        return ret;
    }
    else{
        //There are already some lines in set
        //Or can be no line(all valid == 0).
        //  - Use for loop to traverse the lines:
        //      count the number of valids;
        //      if valid :
        //              + if line_tag == addr_tag: ret |= BIT(WRITE_HIT); write data.
        //              + else: continue to search
        //  - exit the loop: if still no where to get data ret |= BIT(WRITE_MISS)
        //              + if count_valids < ways_assoc, call cache_L2_read() to get line, ret |= BIT(READ_L2_OWN)
        //                  place it in the first available space in line. Then write a byte to bytes_offset.
        //              + else count_valids == ways_assoc, -> need to replace LRU call index
        //                  if lines[LRU] is not dirty -> no need to evict, call cache_L2_read(), ret |= BIT(READ_L2_OWN)
        //                      in place the line in the LRU index. Then write a byte to bytes_offset.
        //                  else: the lines[LRU] is dirty, call cache_L2_write() to evict, ret |= BIT(WRITE_L2);
        //                      call cache_L2_read() to get the line, ret |= BIT(READ_L2). write to a byte to bytes_offset.
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
                    ret |= BIT(WRITE_HIT);
                    hit = 1;
                    (lines[i].data)[addr_bytes_offset] = data;
                    lines[i].tag_array |= BIT(cache->D_BIT);//dirty = 1;

                    uint16_t accessed_lru = get_line_LRU(*cache, lines[i].tag_array);
                    if(update_line_LRU(*cache, lines, accessed_lru, ACCESS) < 0)
                    {
                        printf("Error: Cannot update LRU with addr=%x\n", address);
                        return ERROR;
                    }
                    
                    return ret;
                }
            }
        }
        // printf("count: %d\n", count);
        if(hit == 0)
        {
            ret |= BIT(WRITE_MISS);
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
                if(update_line_LRU(*cache, lines, 0, NEW_LINE) < 0)
                {
                    printf("Error: Cannot update LRU with addr=%x.\n", address);
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
                ret |= BIT(READ_L2_OWN);
                lines[index].tag_array |= BIT(cache->V_BIT); //valid = 1;
                uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
                lines[index].tag_array &= tag_line_mask;// clear old tag
                lines[index].tag_array += addr_tag;//update tag
                for(i = 0; i < size; i++)
                {
                    (lines[index].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
                }
                //Now write the byte:
                (lines[index].data)[addr_bytes_offset] = data;
                lines[index].tag_array |= BIT(cache->D_BIT);// dirty = 1
                return ret;
            }
            else {
                //Now the count == ways_assoc, mean that the set is full of lines,
                //Now we need to replace one of the line in the set.

                //Update old LRU bit of old lines, before adding new line
                int index = cal_LRU(*cache, lines);
                uint16_t accessed_lru = get_line_LRU(*cache, lines[index].tag_array);
                if(update_line_LRU(*cache, lines, accessed_lru, ACCESS) < 0)
                {
                    printf("Error: Cannot update LRU with addr=%x.\n", address);
                    return ERROR;
                }
                
                // printf("lru index: %d\n", index);
                // int j;
                // for(j = 0; j < 4; j++)
                // {
                //     printf("lru:%d", get_line_LRU(*cache, lines[j].tag_array) );
                // }
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
                    ret |= BIT(READ_L2_OWN);
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
                    ret |= BIT(READ_L2_OWN);
                    // lines[index].tag_array |= BIT(cache->V_BIT); //valid = 1;
                    uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
                    lines[index].tag_array &= tag_line_mask;// clear old tag
                    lines[index].tag_array += addr_tag;//update tag
                    for(i = 0; i < size; i++)
                    {
                        (lines[index].data)[i] = line_data[i];// copy from tmp_line to the 1st line.
                    }
                    //Now return the byte:
                    (lines[index].data)[addr_bytes_offset] = data;
                    lines[index].tag_array |= BIT(cache->D_BIT);
                    return ret;
                }
            }
        }
    }
    return ret;
}

/**
  * @brief      Clear all state of L1 cache.
  * @param      cache: pointer to cache instance.
  * @retval     SUCCESS if clear success.
  *             otherwise ERROR.
  */
int cache_L1_clear(cache_t* cache)
{
    int sets_num = pow(2, cache->sets_num_bits);
    int ways_assoc = cache->ways_assoc;
    int line_size = pow(2, cache->bytes_num_bits);
    free(cache);
    cache = create_cache(sets_num, ways_assoc,line_size);
    cache->sets = (set_t*)malloc(sets_num * sizeof(set_t));
    return SUCCESS;
}

/**
  * @brief      Evict command from L2. After this command a line should be invalidated.
  * @param      cache: pointer to cache instance.
  * @param      address: byte address.
  * @retval     status of the evict request:
  *                 @arg    return_t: EVICT_L2_OK if success
  *                         otherwise EVICT_L2_ERROR.
  */
int cache_L2_evict(cache_t* cache, uint32_t address)
{
    return_t ret = 0;
    // uint32_t addr_bytes_offset = get_bytes_offset(*cache, address);
    uint32_t addr_set = get_set(*cache, address);
    uint32_t addr_tag = get_tag(*cache, address);
    line_t* lines = (cache->sets)[addr_set].lines;
    int i;
    if(lines == NULL)
    {
        printf("Warning: The set with %x is null/empty\n", address);
        return EVICT_L2_ERROR;
    }
    for(i =0 ; i < cache->ways_assoc; i++)
    {
        if(lines[i].tag_array & BIT(cache->V_BIT))
        {
            //Only consider valid line:
            uint16_t tag_line_mask = cache->LRU_line_mask | BIT(cache->D_BIT) | BIT(cache->V_BIT);
            tag_line_mask = ~tag_line_mask;
            uint16_t line_tag = lines[i].tag_array & tag_line_mask;
            if(line_tag == addr_tag)
            {
                ret |= BIT(EVICT_L2_OK);
                //clear V bit, indicate that the line is no longer avaiable.
                uint16_t accessed_lru = get_line_LRU(*cache, lines[i].tag_array);
                if(update_line_LRU(*cache, lines,accessed_lru, EVICT_LINE) < 0)
                {
                    printf("Error: Cannot update LRU with addr=%x\n", address);
                    return ERROR;
                }
                lines[i].tag_array &= ~BIT(cache->V_BIT);

                return ret;
            }
        }
    }
    ret |= BIT(EVICT_L2_ERROR);
    printf("Warning: There is no line affected\n");
    return ret;
}

/**
  * @brief      Read request to L2. To get a line from L2.
  * @param      cache: pointer to cache instance.
  * @param      address: byte address.
  * @param      data: pointer to array of data, this array will be modify after get a line from L2
  * @retval     status of the read request L2.
  */
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

/**
  * @brief      Write request to L2. To write/evict a line from L2.
  * @param      cache: pointer to cache instance.
  * @param      address: byte address.
  * @param      data: pointer to array of data, this array will be used to modify the line in L2.
  * @retval     status of the write request L2.
  */
int cache_L2_write(cache_t* cache, uint32_t address, uint8_t* data)
{
    //simulate that write to L2 (due to L1 eviction) is always success
    //further code can goes here.
    return SUCCESS;
}

/**
  * @}
  */


/* Cache statistic function prototypes -------------------------------------------------*/
/** @addtogroup Cache_statistic
  * @{
  */

/* Statistic initialize functions ******************************************************/

/**
  * @brief      Create a cache statistic instance.
  * @param      cache_name: name of the cache. for example: "Instruction", "Data".
  * @param      log_fp: pointer to log file instance.
  *                     Note: this arg must be not null, you must open the file before 
  *                             pass to this log_fp
  * @param      mode: mode of operation
  *                 @arg    1: only log cache statistic.
  *                 @arg    2: log the L2 communication message also.
  * @retval     pointer to statistic instance.
  */
cache_stat_t* cache_stat_create(char* cache_name, FILE* log_fp, int mode)
{
    cache_stat_t * stat = (cache_stat_t*)malloc(sizeof(cache_stat_t));
    if(stat == NULL)
    {
        printf("Error: Internal error cache stat create\n");
        return NULL;
    }
    stat->name = cache_name;
    stat->count = 0;
    stat->log_file = log_fp;
    stat->mode = mode;
    stat->read_hits = 0;
    stat->read_misses = 0;
    stat->write_hits = 0;
    stat->write_misses = 0;
    stat->hit_rate = 1;
    return stat;
}

/**
  * @brief      Initialize the cache instance.
  *             Note: the instance must NOT be NULL.
  * @param      stat: pointer to the statistic instance.
  * @param      cache_name: name of the cache. for example: "Instruction", "Data".
  * @param      log_fp: pointer to log file instance.
  *                     Note: this arg must be not null, you must open the file before 
  *                             pass to this log_fp
  * @param      mode: mode of operation
  *                 @arg    1: only log cache statistic.
  *                 @arg    2: log the L2 communication message also.
  * @retval     SUCCESS if success. Otherwise ERROR.
  */
int cache_stat_init(cache_stat_t* stat,char* cache_name, FILE* log_fp, int mode)
{
    if(stat == NULL)
    {
        stat = cache_stat_create(cache_name, log_fp, mode);
        if(stat == NULL)
        {
            printf("Error: Internal error cache_stat_init\n");
            return ERROR;
        }
        return SUCCESS;
    }
    stat->name = cache_name;
    stat->count = 0;
    stat->log_file = log_fp;
    stat->mode = mode;
    stat->read_hits = 0;
    stat->read_misses = 0;
    stat->write_hits = 0;
    stat->write_misses = 0;
    stat->hit_rate = 1;
    return SUCCESS;
}

/**
  * @brief      Update cache statistic.
  *             This function should be called after any cache request, to update the
  *             cache statistic before calling any other requests.
  * @param      stat: pointer to the statistic instance.
  * @param      update: return value of cache_request();
  *                     Note: pass the return value of any request to here.
  * @param      address: address that you pass in the cache_request();
  * @retval     SUCCESS if success. Otherwise ERROR.
  */

/* Statistic activities functions ******************************************************/
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
            fprintf(stat->log_file, "[MESSAGE] %s write to L2 %x\n",stat->name, address);
        }
        if(update & BIT(READ_L2))
        {
            fprintf(stat->log_file, "[MESSAGE] %s read from L2 %x\n", stat->name, address);
        }
        if(update & BIT(READ_L2_OWN))
        {
            fprintf(stat->log_file, "[MESSAGE] %s read for Ownership from L2 %x\n", stat->name, address);
        }
    }
    return SUCCESS;
}

/**
  * @brief      Log cache statistic to file.
  *             This function can be called any place, it will not affect the cache
  *             or the cache statistic instance.
  * @param      stat: pointer to the statistic instance.
  * @retval     SUCCESS if success. Otherwise ERROR.
  */
int cache_log(cache_stat_t *stat)
{
    FILE *fp = stat->log_file;
    if(stat->count == 0)
    {
        fprintf(fp, "[LOG] Mode: %d\n", stat->mode);
    }
    int reads_num = stat->read_hits + stat->read_misses;
    int writes_num = stat->write_hits + stat->write_misses;
    fprintf(fp, "------------------------------\n");
    fprintf(fp, "> Cache: %s, log: %d\n", stat->name, stat->count);
    fprintf(fp, "> #reads        : %d\n", reads_num);
    fprintf(fp, "> #writes       : %d\n", writes_num);
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

/**
  * @brief      Clear cache statistic.
  *             This function can be called when you need to reset the
  *             statistic, it will not affect the cache.
  * @param      stat: pointer to the statistic instance.
  * @retval     SUCCESS if success. Otherwise ERROR.
  */
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

/* Statistic debug functions ***********************************************************/
/**
  * @brief      Print cache status to the console.
  *             This function can be called when you need to debug the
  *             status of the cache.
  * @param      cache: cache instance.
  * @retval     None.
  */
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
/**
  * @}
  */


/* LRU policy function prototypes -------------------------------------------------*/
/** @addtogroup LRU_policy
  * @{
  */

/**
  * @attention  RESTRICTED API.
  * @brief      Get the index of LRU replacement.
  * @param      cache: cache instance.
  * @param      lines: a set in the cache
  *                 Note: acctually you just pass the array of lines in the set_t instance.
  * @retval     SUCCESS if success. Otherwise ERROR.
  */
int cal_LRU(cache_t cache, line_t* lines)
{
    int index = 0;
    int i;
    
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

/**
  * @attention  RESTRICTED API.
  * @brief      Update LRU bits of the set.
  * @param      cache: cache instance.
  * @param      lines: a set in the cache
  *                 Note: acctually you just pass the array of lines in the set_t instance.
  * @param      accessed_lru: LRU part of the incomming replaced line in the set.
  *                 Note: this should be call before any replace happened.
  * @param      mode: mode of LRU
  *                 @arg    NEW_LINE
  *                 @arg    ACCESS
  *                 @arg    EVICT_LINE
  * @retval     SUCCESS if success. Otherwise ERROR.
  */
int update_line_LRU(cache_t cache, line_t* lines, uint16_t accessed_lru, LRU_mode_t mode)
{   
    int i;
    int lst_query_size = 0;
    int accessed_index = 0;
    // return SUCCESS;
    if(mode == ACCESS)
    {
        // +1 all valid lines.
        query_t lst_valid_not_access_lru[cache.ways_assoc];
        for(i = 0; i < cache.ways_assoc; i++)
        {
            uint16_t tmp_lru = get_line_LRU(cache,lines[i].tag_array);
            if(lines[i].tag_array & BIT(cache.V_BIT))
            {
                //only consider valid lines
                if(tmp_lru != accessed_lru)
                {
                    lst_valid_not_access_lru[lst_query_size].index = i;
                    lst_valid_not_access_lru[lst_query_size].lru = tmp_lru;
                    lst_query_size++;
                }
                else{
                    //
                    accessed_index = i;
                }
            }
        }
        for(i = 0; i < lst_query_size; i++)
        {
            if(lst_valid_not_access_lru[i].lru < accessed_lru)
            {
                int tmp_index = lst_valid_not_access_lru[i].index;
                uint16_t tmp_add = cache.D_BIT + 1;
                //add 1 to LRU bits;
                lines[tmp_index].tag_array += BIT(tmp_add);
            }
        }
        //Now turn the LRU at accessed lru = 0
        lines[accessed_index].tag_array &=~ cache.LRU_line_mask;
        return SUCCESS;
    }
    else if(mode == NEW_LINE)
    {
        // +1 all lines have get_line_LRU < accessed_lru
        for(i = 0; i < cache.ways_assoc; i++)
        {
            if(lines[i].tag_array & BIT(cache.V_BIT))
            {
                //only consider valid lines:
                uint16_t tmp_add = cache.D_BIT + 1;
                lines[i].tag_array += BIT(tmp_add);
            }
        }
        return SUCCESS;
    }
    else if(mode == EVICT_LINE)
    {
        // +1 all lines have get_line_LRU < accessed_lru
        // then -1 all valid lines
        query_t lst_valid_not_access_lru[cache.ways_assoc];
        for(i = 0; i < cache.ways_assoc; i++)
        {
            uint16_t tmp_lru = get_line_LRU(cache,lines[i].tag_array);
            if(lines[i].tag_array & BIT(cache.V_BIT))
            {
                //only consider valid lines
                if(tmp_lru != accessed_lru)
                {
                    lst_valid_not_access_lru[lst_query_size].index = i;
                    lst_valid_not_access_lru[lst_query_size].lru = tmp_lru;
                    lst_query_size++;
                }
                else{
                    //
                    accessed_index = i;
                }
            }
        }
        for(i = 0; i < lst_query_size; i++)
        {
            if(lst_valid_not_access_lru[i].lru > accessed_lru)
            {
                int tmp_index = lst_valid_not_access_lru[i].index;
                uint16_t tmp_add = cache.D_BIT + 1;
                //add 1 to LRU bits;
                lines[tmp_index].tag_array -= BIT(tmp_add);
            }
        }
        //Now turn the LRU at accessed lru = 0
        lines[accessed_index].tag_array &=~ cache.LRU_line_mask;
        return SUCCESS;
    }
    else{
        //do nothing
    }
    return ERROR;
}

/**
  * @}
  */