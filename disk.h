#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "settings.h"

// Struttura per entry nella FAT
typedef struct {
    int next_block; // -1 per ultimo blocco
    int file;       // -2 per blocco non usato, 1 per blocco in uso
    int next_free_block; //indice del prossimo blocco libero
} FatEntry;

// Struttura File Allocation Table (FAT)
typedef struct {
    int free_block_idx; // Indice del primo blocco libero
    FatEntry entries[FAT_SIZE];
} Fat;

typedef struct {
    int size;       // Dimensione del disco
    Fat fat;        // File Allocation Table per il disco
    char data[];    // Dati del disco (mappati in memoria)
} Disk;

Disk* disk_init(const char* filename, int format);
void disk_format(Disk* disk);
void disk_read(Disk* disk, int block, char* buffer);
void disk_write(Disk* disk, int block, const char* buffer);
void disk_close(Disk* disk);
void disk_save_fat(Disk* disk);
