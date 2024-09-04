#pragma once

#include "disk.h"

#define FAT_SIZE DISK_SIZE / BLOCK_SIZE

// Struttura per entry nella FAT
typedef struct {
    int next_block; // -1 per ultimo blocco
    int file;       // -2 per blocco non usato
} FatEntry;

// struttura File Allocation Table (FAT)
typedef struct {
    int free_blocks; 
    FatEntry entries[FAT_SIZE]; 
} Fat;

void fat_init(Fat* fat);
int fat_get_free_block(Fat* fat);
int fat_alloc_block(Fat* fat);
void fat_free_block(Fat* fat, int block);
int fat_get_next_block(Fat* fat, int block);
void fat_set_next_block(Fat* fat, int block, int next_block);
