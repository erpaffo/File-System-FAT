#pragma once

#include "disk.h"
#include "fat.h"
#include "directory.h"

FileHandle* create_file(Disk* disk, const char* filename);
void write_file(FileHandle* handle, Disk* disk, const void* buffer, int size);
void read_file(FileHandle* handle, Disk* disk, void* buffer, int size);
void erase_file(Disk* disk, FileHandle* handle, const char* filename);
void seek_file(FileHandle* handle, Disk *disk, int offset);
FileHandle* open_file(Disk* disk, Directory* dir, const char* filename);
void close_file(FileHandle* handle);