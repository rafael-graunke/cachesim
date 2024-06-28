#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cache.h"

Cache *cache_create(CacheConfig config)
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    if (cache == NULL)
    {
        printf("Error allocating cache.\n");
        exit(EXIT_FAILURE);
    }

    cache->config = config;
    cache->cr_hit = 0;
    cache->cr_miss = 0;
    cache->mp_read = 0;
    cache->mp_write = 0;
    cache->tags = (CacheLine *)malloc(sizeof(CacheLine) * config.linecount);

    if (cache->tags == NULL)
    {
        printf("Error allocating cache tags.\n");
        free(cache);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < config.linecount; i++)
    {
        cache->tags[i].tag = -1;
        cache->tags[i].c_access = 0;
        cache->tags[i].t_access = INT32_MAX;
        cache->tags[i].dirty = false;
    }

    return cache;
}

void cache_destroy(Cache *cache)
{
    free(cache->tags);
    free(cache);
}

CacheLine *random_reference(Cache *cache, uint32_t group)
{
    for (int i = 0; i < cache->config.lperg; i++)
    {
        CacheLine *curr = &cache->tags[group * cache->config.lperg + i];
        if (curr->tag == -1)
            return curr;
    }

    int index = rand() % cache->config.lperg;
    return &cache->tags[group * cache->config.lperg + index];
}

CacheLine *lfu_reference(Cache *cache, uint32_t group)
{
    int min = INT32_MAX;
    int posmin = 0;
    for (int i = 0; i < cache->config.lperg; i++)
    {
        CacheLine *curr = &cache->tags[group * cache->config.lperg + i];
        if (curr->tag == -1)
            return curr;

        if (curr->c_access < min)
        {
            min = curr->c_access;
            posmin = i;
        }
    }

    return &cache->tags[group * cache->config.lperg + posmin];
}

CacheLine *lru_reference(Cache *cache, uint32_t group)
{
    int max = 0;
    int posmax = 0;
    for (int i = 0; i < cache->config.lperg; i++)
    {
        CacheLine *curr = &cache->tags[group * cache->config.lperg + i];

        if (curr->tag == -1)
            return curr;

        if (curr->t_access > max)
        {
            max = curr->t_access;
            posmax = i;
        }
    }
    return &cache->tags[group * cache->config.lperg + posmax];
}

CacheLine *ref2replace(Cache *cache, uint32_t group)
{
    CacheLine *replace;
    switch (cache->config.owpolicy)
    {
    case LRU:
        replace = lru_reference(cache, group);
        break;
    case LFU:
        replace = lfu_reference(cache, group);
        break;
    default:
        replace = random_reference(cache, group);
        break;
    }

    return replace;
}

uint32_t get_group(Cache *cache, uint32_t addr)
{
    int b_word, b_group = 0;
    b_word = (int)log2f((float)cache->config.linesize);
    b_group = (int)log2f((float)cache->config.linecount / (float)cache->config.lperg);

    if (!b_group)
        return 0;

    addr >>= b_word;

    return addr & (0xFFFFFFFF >> (32 - b_group));
}

uint32_t get_tag(Cache *cache, uint32_t addr)
{
    uint32_t b_word, b_group;
    b_word = (uint32_t)log2f((float)cache->config.linesize);
    b_group = (uint32_t)log2f((float)cache->config.linecount / cache->config.lperg);

    return addr >> (b_word + b_group);
}

void cache_fetch(Cache *cache, uint32_t addr)
{
    uint32_t group = get_group(cache, addr);
    uint32_t tag = get_tag(cache, addr);

    bool found = false;
    CacheLine *cursor;
    for (int i = 0; i < cache->config.lperg; i++)
    {
        cursor = &cache->tags[group * cache->config.lperg + i];
        if (cursor->tag == tag) // This is a cache hit
        {
            cursor->c_access++; // Increment de access counter
            cache->cr_hit++;
            found = true;
            continue;
        }
        cursor->t_access++; // Increment the remaining line time "since last access"
    }

    if (found)
        return;

    cache->cr_miss++;
    cache->mp_read++;

    CacheLine *replace = ref2replace(cache, group);
    replace->c_access = 0;
    replace->t_access = 0;
    replace->tag = tag;

    if (cache->config.wpolicy == WRITE_BACK && replace->dirty)
    {
        cache->mp_write++;
        replace->dirty = false;
    }
}

void cache_write(Cache *cache, uint32_t addr)
{
    uint32_t group = get_group(cache, addr);
    uint32_t tag = get_tag(cache, addr);
    CacheLine *match;

    bool found = false;
    CacheLine *cursor;
    for (int i = 0; i < cache->config.lperg; i++)
    {
        cursor = &cache->tags[group * cache->config.lperg + i];
        if (cursor->tag == tag) // This is a cache hit
        {
            found = true;
            cache->cw_hit++;
            match = cursor;
            match->c_access++;
            continue;
        }
        cursor->t_access++; // Increment the remaining line time "since last access"
    }

    if (!found)
        cache->cw_miss++;

    if (cache->config.wpolicy == WRITE_THROUGH)
    {
        cache->mp_write++;
        return;
    }

    if (found)
    {
        match->dirty = true;
        return;
    }

    CacheLine *replace = ref2replace(cache, group);
    replace->c_access = 0;
    replace->t_access = 0;
    replace->tag = tag;

    if (replace->dirty)
        cache->mp_write++;

    cache->mp_read++;
    replace->dirty = true;
}
