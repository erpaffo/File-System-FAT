#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#define DISK_SIZE 1024*1024 // 1MB
#define BLOCK_SIZE 512 // 512B
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

typedef struct {
    int size;       // Dimensione disco
    Fat fat;        // File Allocation Table per disco
    char data[];    // Dati del disco
} Disk;

Disk* disk_init(const char* filename, int format);
void disk_format(Disk* disk);
