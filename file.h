#pragma once

#include "disk.h"
#include "fat.h"
#include "directory.h"

typedef struct {
    char name[MAX_FILENAME_LENGTH]; // Nome del file o della directory
    int start_block;                // Blocco iniziale sul disco
    int size;                       // Dimensione in byte (0 per directory)
    int is_directory;               // 1 se è una directory, 0 se è un file
} FileControlBlock;

// Struttura per gestire i file aperti
typedef struct {
    FileControlBlock fcb;  // FCB associato al file
    int current_block;     // Blocco corrente durante le operazioni di I/O
    int position;          // Posizione corrente nel file
} FileHandle;

FileHandle* create_file(Disk* disk, const char* filename);
void write_file(FileHandle* handle, Disk* disk, const void* buffer, int size);
void read_file(FileHandle* handle, Disk* disk, void* buffer, int size);
void erase_file(Disk* disk, FileHandle* handle, const char* filename);
void seek_file(FileHandle* handle, Disk *disk, int offset);
FileHandle* open_file(Disk* disk, Directory* dir, const char* filename);
void close_file(FileHandle* handle);