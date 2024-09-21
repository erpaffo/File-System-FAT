// file.c
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Crea file
FileHandle* create_file(Disk *disk, const char* filename) {
    int free_block = fat_alloc_block(&disk->fat);
    if (free_block == -1) {
        printf("Errore: Nessun blocco libero\n");
        return NULL;
    }

    // Inizializza il FileHandle
    FileHandle* handle = (FileHandle*) malloc(sizeof(FileHandle));
    if (handle == NULL) {
        printf("Errore di allocazione memoria per FileHandle\n");
        fat_free_block(&disk->fat, free_block);
        return NULL;
    }

    memset(handle, 0, sizeof(FileHandle));
    strncpy(handle->fcb.name, filename, MAX_FILENAME_LENGTH - 1);
    handle->fcb.name[MAX_FILENAME_LENGTH - 1] = '\0';
    handle->fcb.start_block = free_block;
    handle->fcb.size = 0;
    handle->fcb.is_directory = 0;
    handle->current_block = free_block;
    handle->position = 0;

    if (DEBUG) {
        printf("Creazione file %s. Primo blocco: %d\n", filename, free_block);
    }

    // Inizializza e scrivi il FileControlBlock sul disco
    char block_data[BLOCK_SIZE];
    memset(block_data, 0, BLOCK_SIZE);
    memcpy(block_data, &handle->fcb, sizeof(FileControlBlock));
    disk_write(disk, free_block, block_data);

    return handle;
}

// Scrive su file
void write_file(FileHandle* handle, Disk* disk, const void* buffer, int size) {
    int block = handle->current_block;
    int position = handle->position;
    int written = 0;

    while (written < size) {
        char block_data[BLOCK_SIZE];
        disk_read(disk, block, block_data); // Leggi il blocco corrente

        int remaining = BLOCK_SIZE - position;
        int to_write = size - written;
        int write_size = remaining < to_write ? remaining : to_write;

        // Copia i dati nel blocco
        memcpy(block_data + position, buffer + written, write_size);

        // Scrivi il blocco aggiornato sul disco
        disk_write(disk, block, block_data);

        written += write_size;
        position += write_size;

        if (position == BLOCK_SIZE) {
            // Necessario allocare un nuovo blocco
            int next_block = fat_get_next_block(&disk->fat, block);
            if (next_block == -1) {
                next_block = fat_alloc_block(&disk->fat);
                if (next_block == -1) {
                    printf("Errore: Nessun blocco libero\n");
                    return;
                }
                fat_set_next_block(&disk->fat, block, next_block);
            }
            block = next_block;
            position = 0;
        }
    }

    handle->current_block = block;
    handle->position = position;
    handle->fcb.size += size;

    // Aggiorna il FileControlBlock sul disco
    char block_data[BLOCK_SIZE];
    disk_read(disk, handle->fcb.start_block, block_data);
    memcpy(block_data, &handle->fcb, sizeof(FileControlBlock));
    disk_write(disk, handle->fcb.start_block, block_data);
}

// Legge da file
void read_file(FileHandle* handle, Disk* disk, void* buffer, int size) {
    int block = handle->current_block;
    int position = handle->position;
    int read_bytes = 0;

    // Limita la lettura alla dimensione del file
    if (size > handle->fcb.size - handle->position) {
        size = handle->fcb.size - handle->position;
    }

    while (read_bytes < size) {
        char block_data[BLOCK_SIZE];
        disk_read(disk, block, block_data);

        int remaining = BLOCK_SIZE - position;
        int to_read = size - read_bytes;
        int read_size = remaining < to_read ? remaining : to_read;

        memcpy(buffer + read_bytes, block_data + position, read_size);

        read_bytes += read_size;
        position += read_size;

        if (position == BLOCK_SIZE) {
            block = fat_get_next_block(&disk->fat, block);
            if (block == -1) {
                break; // Fine del file
            }
            position = 0;
        }
    }

    handle->current_block = block;
    handle->position = position;
}

// Cancella file
void erase_file(Disk* disk, FileHandle* handle, const char* filename) {
    int current_block = handle->fcb.start_block;
    while (current_block != -1 && current_block >= 0) {
        int next_block = fat_get_next_block(&disk->fat, current_block);
        fat_free_block(&disk->fat, current_block);
        current_block = next_block;
    }
}

// Sposta all'offset specificato
void seek_file(FileHandle* handle, Disk *disk, int offset) {
    if (offset > handle->fcb.size) {
        printf("Errore: offset supera la dimensione del file\n");
        return;
    }

    int current_block = handle->fcb.start_block;
    int current_offset = 0;

    if (DEBUG) {
        printf("[DEBUG] Seek richiesto all'offset: %d\n", offset);
        printf("[DEBUG] Dimensione file: %d\n", handle->fcb.size);
        printf("[DEBUG] Blocco iniziale: %d\n", current_block);
    }

    // Scorre attraverso i blocchi fino a raggiungere l'offset
    while (current_offset + BLOCK_SIZE <= offset) {
        current_block = fat_get_next_block(&disk->fat, current_block);
        if (current_block == -1 || current_block < 0) {
            printf("Errore: seek oltre la fine del file\n");
            return;
        }
        current_offset += BLOCK_SIZE;

        if (DEBUG) {
            printf("[DEBUG] Spostato al blocco successivo: %d\n", current_block);
            printf("[DEBUG] Offset attuale: %d\n", current_offset);
        }
    }

    // Calcola la posizione all'interno del blocco corrente
    int position = offset - current_offset;

    handle->current_block = current_block;
    handle->position = position;

    if (DEBUG) {
        printf("[DEBUG] Posizione aggiornata. Blocco: %d, Posizione: %d\n", handle->current_block, handle->position);
    }
}

// Apre il file
FileHandle* open_file(Disk* disk, Directory* dir, const char* filename) {
    // Cerca il file nella directory corrente
    for (int i = 0; i < dir->num_entries; i++) {
        if (!dir->entries[i].is_directory && strcmp(dir->entries[i].name, filename) == 0) {
            FileHandle* handle = (FileHandle*) malloc(sizeof(FileHandle));
            if (handle == NULL) {
                printf("Errore di allocazione memoria per FileHandle\n");
                return NULL;
            }

            handle->fcb = dir->entries[i];
            handle->current_block = handle->fcb.start_block;
            handle->position = 0; // Iniziare dalla posizione 0

            return handle;
        }
    }
    return NULL;
}

void close_file(FileHandle* handle) {
    if (handle != NULL) {
        free(handle);
    }
}

