#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "fat.h"

#define DEBUG 1

#define DISK_SIZE 1024*1024 // 1MB
#define BLOCK_SIZE 512 // 512B

typedef struct {
    int size;       // Dimensione disco
    Fat fat;        // File Allocation Table per disco
    char data[];    // Dati del disco
} Disk;

Disk* disk_init(const char* filename, int format);
void disk_format(Disk* disk);
void disk_read(Disk* disk, int block, char* buffer);
void disk_write(Disk* disk, int block, const char* buffer);
void disk_close(Disk* disk);