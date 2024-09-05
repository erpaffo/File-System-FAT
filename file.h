#pragma once

#include "disk.h"
#include "fat.h"

// Struttura per gestire i file utilizzata per contenere la posizione del file
typedef struct {
    int start_block;
    int current_block;
    int position;
    int size;
} FileHandle;

FileHandle* create_file(Disk* disk, const char* filename);
void erase_file(Disk* disk, const char* filename);
void write_file(FileHandle* handle, Disk* disk, const void* buffer, int size);
void read_file(FileHandle* handle, Disk* disk, void* buffer, int size);
void seek_file(FileHandle* handle, int offset);
FileHandle* open_file(Disk* disk, const char* filename);
void close_file(FileHandle* handle);