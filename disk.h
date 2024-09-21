#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "structures.h"

Disk* disk_init(const char* filename, int format);
void disk_format(Disk* disk);
void disk_read(Disk* disk, int block, char* buffer);
void disk_write(Disk* disk, int block, const char* buffer);
void disk_close(Disk* disk);
void disk_save_fat(Disk* disk);
