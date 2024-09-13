#pragma once

#include "disk.h"
#include "fat.h"

#define MAX_DIR_NAME 100
#define MAX_FILES_PER_DIR 20

// Elemento della directory (file o subdirectory)
typedef struct {
    char name[MAX_DIR_NAME];
    int start_block;
    int is_directory;  // 1 se è una directory, 0 se è un file
} DirEntry;

// Struttura per directory
typedef struct {
    char name[MAX_DIR_NAME];
    int start_block;
    int parent_block;
    int num_entries;
    DirEntry entries[MAX_FILES_PER_DIR];  
} Directory;

Directory* create_dir(Disk* disk, Directory* parent, const char* dirname);
void erase_dir(Disk* disk, Directory* dir);
void change_dir(Disk* disk, Directory** current_dir, const char* dirname);
void list_dir(Disk* disk, Directory* dir);
