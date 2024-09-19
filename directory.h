#pragma once

#include "disk.h"
#include "fat.h"

// Struttura per la directory
typedef struct {
    FileControlBlock fcb;              // FCB della directory stessa
    int num_entries;                   // Numero di entries nella directory
    FileControlBlock entries[MAX_DIR_ENTRIES]; // Array di FCB dell'entry (file o directory)
} Directory;

Directory* create_dir(Disk* disk, Directory* parent, const char* dirname);
void erase_dir(Disk* disk, Directory* dir);
void change_dir(Disk* disk, Directory** current_dir, const char* dirname);
void list_dir(Disk* disk, Directory* dir);
