// directory.c
#include "directory.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Crea una nuova directory
Directory* create_dir(Disk* disk, Directory* parent, const char* dirname) {
    int free_block = fat_alloc_block(&disk->fat);
    if (free_block == -1) {
        printf("Errore: Nessun blocco libero\n");
        return NULL;
    }

    Directory* dir = (Directory*) malloc(sizeof(Directory));
    if (dir == NULL) {
        printf("Errore di allocazione memoria per Directory\n");
        fat_free_block(&disk->fat, free_block);
        return NULL;
    }

    memset(dir, 0, sizeof(Directory));
    strncpy(dir->fcb.name, dirname, MAX_FILENAME_LENGTH - 1);
    dir->fcb.name[MAX_FILENAME_LENGTH - 1] = '\0';
    dir->fcb.start_block = free_block;
    dir->fcb.size = 0;
    dir->fcb.is_directory = 1;
    dir->num_entries = 0;

    dir->parent_block = (parent != NULL) ? parent->fcb.start_block : -1;

    if (DEBUG) {
        printf("Creazione directory %s. Primo blocco: %d\n", dirname, free_block);
    }

    // Scrivi il Directory sul disco
    char block_data[BLOCK_SIZE];
    memset(block_data, 0, BLOCK_SIZE);
    memcpy(block_data, dir, sizeof(Directory));
    disk_write(disk, free_block, block_data);

    if (parent != NULL) {
        // Aggiorna la directory padre
        parent->entries[parent->num_entries].start_block = free_block;
        parent->entries[parent->num_entries].is_directory = 1;
        strncpy(parent->entries[(parent)->num_entries].name, dirname, MAX_FILENAME_LENGTH - 1);
        parent->num_entries++;
        disk_write(disk, parent->fcb.start_block, (const char*)parent);
    }

    return dir;
}

// Cancella una directory e i suoi file
void erase_dir(Disk* disk, Directory* dir) {
    for (int i = 0; i < dir->num_entries; i++) {
        if (dir->entries[i].is_directory) {
            Directory sub_dir;
            disk_read(disk, dir->entries[i].start_block, (char*)&sub_dir);
            erase_dir(disk, &sub_dir);
        } else {
            FileHandle handle;
            handle.fcb = dir->entries[i];
            erase_file(disk, &handle, dir->entries[i].name);
        }
    }

    fat_free_block(&disk->fat, dir->fcb.start_block);
    if (DEBUG) {
        printf("Directory %s cancellata\n", dir->fcb.name);
    }

    free(dir);
}

// Cambia directory
void change_dir(Disk* disk, Directory** current_dir, const char* dirname) {
    for (int i = 0; i < (*current_dir)->num_entries; i++) {
        if ((*current_dir)->entries[i].is_directory && strcmp((*current_dir)->entries[i].name, dirname) == 0) {
            Directory* new_dir = (Directory*) malloc(sizeof(Directory));
            if (new_dir == NULL) {
                printf("Errore di allocazione memoria per Directory\n");
                return;
            }
            disk_read(disk, (*current_dir)->entries[i].start_block, (char*) new_dir);
            *current_dir = new_dir;

            if (DEBUG) {
                printf("Cambiata directory a %s\n", dirname);
            }
            return;
        }
    }
    printf("Errore: directory %s non trovata\n", dirname);
}

// Elenca i contenuti della directory
void list_dir(Disk* disk, Directory* dir) {
    printf("Contenuto della directory %s:\n", dir->fcb.name);
    for (int i = 0; i < (dir)->num_entries; i++) {
        printf("%s%s\n", dir->entries[i].name, dir->entries[i].is_directory ? "/" : "");
    }
}
