#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

int main(void)
{
    srand(time(NULL));
    // INIT CACHE
    CacheConfig config;
    scanf("%d %d %d %d %d", &config.wpolicy, &config.owpolicy, &config.linecount, &config.linesize, &config.lperg);

    Cache *cache = cache_create(config);

    // LOOKUP FILE
    FILE *input = fopen("input/oficial.cache", "r");

    uint32_t address;
    char operation;
    while (fscanf(input, "%x %c", &address, &operation) != EOF)
    {
        if (operation == 'R')
        {
            cache_fetch(cache, address);
            continue;
        }

        if(operation == 'W')
        {
            cache_write(cache, address);
            continue;
        }

        printf("Achou um caracter nada a ver ---> %c", operation);
        break;
    }

    if (cache->config.wpolicy == WRITE_BACK)
        for (int i = 0; i < cache->config.linecount; i++)
            if (cache->tags[i].dirty)
                cache->mp_write++;

    printf("### Configuracao ###\n");
    printf("Politica de Escrita: ");

    if (cache->config.wpolicy)
        printf("WRITE_BACK\n");
    else
        printf("WRITE_THROUGH\n");

    printf("Politica de Substituicao: ");
    if (cache->config.owpolicy == 0)
        printf("LRU\n");
    else if (cache->config.owpolicy == 1)
        printf("LFU\n");
    else
        printf("RANDOM\n");

    printf("Num. Blocos: %d\n", cache->config.linecount);
    printf("Tamanho do Bloco: %d\n", cache->config.linesize);
    printf("Associatividade: %d\n\n", cache->config.lperg);

    printf("### Resultados ###\n");
    int total_reads = cache->cr_hit + cache->cr_miss;
    int total_writes = cache->cw_hit + cache->cw_miss;
    printf("Leituras: %d\n", total_reads);
    printf("Escritas: %d\n", total_writes);
    printf("Hit Leitura: %d\n", cache->cr_hit);
    printf("Miss Leitura: %d\n", cache->cr_miss);
    printf("Hits Totais: %d\n", cache->cr_hit + cache->cw_hit);
    printf("Read na MP: %d\n", cache->mp_read);
    printf("Write na MP: %d\n", cache->mp_write);
    printf("Acessos na MP: %d\n", cache->mp_read + cache->mp_write);

    float hit_rate = ((cache->cr_hit + cache->cw_hit) * 100.0 / (total_reads + total_writes));
    float average_time = 10.0 + (1 - hit_rate / 100) * 60.0;

    printf("Taxa de Hits: %f\n", hit_rate);
    printf("Tempo de acesso medio (ns): %f\n", average_time);

    return EXIT_SUCCESS;
}
