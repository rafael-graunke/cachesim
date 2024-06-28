#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include <stdbool.h>

enum WritePolicy
{
    WRITE_THROUGH = 0,
    WRITE_BACK = 1
};

enum OverwritePolicy
{
    LRU = 0,
    LFU = 1,
    RANDOM = 2
};

typedef struct cache_line
{
    int tag;
    int c_access; // Access Counter
    int t_access; // Access Timer
    bool dirty;
} CacheLine;

typedef struct cache_config
{
    int wpolicy;
    int owpolicy;
    int linecount;
    int linesize;
    int lperg; // Lines per group
} CacheConfig;

typedef struct cache
{
    CacheConfig config;
    int cr_hit;
    int cr_miss;
    int cw_hit;
    int cw_miss;
    int mp_write;
    int mp_read;
    CacheLine *tags;
} Cache;

Cache *cache_create(CacheConfig config);
void cache_destroy(Cache *cache);
void cache_fetch(Cache *cache, uint32_t addr);
void cache_write(Cache *cache, uint32_t addr);

#endif