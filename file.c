#include "file.h"

// Crea file
FileHandle* create_file(Disk *disk, const char* filename) {
    int free_block = fat_alloc_block(&disk->fat);
    if (free_block == -1) {
        printf("Errore: Nessun blocco libero\n");
        return NULL;
    }

    FileHandle* handle = (FileHandle*) malloc(sizeof(FileHandle));
    handle->start_block = free_block;
    handle->current_block = free_block;
    handle->position = 0;
    handle->size = 0;

    if (DEBUG) {
        printf("Creazione file %s. Primo blocco: %d\n", filename, free_block);
    }

    disk_write(disk, free_block, (const char*) handle);

    return handle;
}


// Scrive su file
void write_file(FileHandle* handle, Disk* disk, const void* buffer, int size) {
    int block = handle->current_block;
    int position = handle->position;
    int written = 0;

    while (written < size) {
        int remaining = BLOCK_SIZE - position;
        int to_write = size - written;
        int write_size = remaining < to_write ? remaining : to_write;

        disk_write(disk, block, buffer + written);

        written += write_size;
        position += write_size;

        if (position == BLOCK_SIZE) {
            int next_block = fat_alloc_block(&disk->fat);
            if (next_block == -1) {
                printf("Errore: Nessun blocco libero\n");
                return;
            }
            fat_set_next_block(&disk->fat, block, next_block);
            block = next_block;
            position = 0;
        }
    }

    handle->current_block = block;
    handle->position = position;
    handle->size += size;
}

// Legge da file
void read_file(FileHandle* handle, Disk* disk, void* buffer, int size) {
    int block = handle->current_block;
    int position = handle->position;  
    int read = 0;

    if (DEBUG) {
        printf("[DEBUG] Inizio lettura del file. Dimensione richiesta: %d, Blocco corrente: %d, Posizione: %d\n", size, block, position);
        printf("[DEBUG] Dimensione totale del file: %d\n", handle->size);
    }

    if (position + size > handle->size) {
        printf("[DEBUG] Errore: tentativo di leggere oltre la fine del file\n");
        size = handle->size - position;  
    }

    while (read < size && block != -1) {
        char block_data[BLOCK_SIZE];  
        disk_read(disk, block, block_data);

        if (DEBUG) {
            printf("[DEBUG] Blocco letto: %d, Dati letti: %.*s\n", block, BLOCK_SIZE, block_data);
        }

        int remaining_in_block = BLOCK_SIZE - position;
        int to_read = size - read;
        int read_size = (remaining_in_block < to_read) ? remaining_in_block : to_read;

        memcpy(buffer + read, block_data + position, read_size);

        read += read_size;
        position += read_size;

        if (DEBUG) {
            printf("[DEBUG] Letti %d byte dal file. Posizione attuale: %d, Byte letti in totale: %d\n", read_size, position, read);
        }

        if (position >= BLOCK_SIZE) {
            block = fat_get_next_block(&disk->fat, block);  
            if (block == -1) {
                printf("[DEBUG] Nessun blocco successivo, fine lettura.\n");
                break;  
            }
            position = 0;  
            printf("[DEBUG] Passaggio al blocco successivo: %d\n", block);
        }
    }

    handle->current_block = block;
    handle->position = position;  

    if (DEBUG) {
        printf("[DEBUG] Lettura completata. Dati totali letti: %d, Posizione finale nel file: %d\n", read, position);
    }
}

// Cancella file
void erase_file(Disk* disk, FileHandle* handle, const char* filename) {
    int current_block = handle->start_block;
    while (current_block != -1 && current_block >= 0) {
        int next_block = fat_get_next_block(&disk->fat, current_block);
        fat_free_block(&disk->fat, current_block);
        current_block = next_block;
    }
}

// Sposta all'offset specificato
void seek_file(FileHandle* handle, Disk* disk, int offset) {
    if (offset > handle->size) {
        printf("Errore: offset supera la dimensione del file\n");
        return;
    }

    int current_block = handle->start_block;
    int current_offset = 0;

        if (DEBUG) {
        printf("[DEBUG] Seek richiesto all'offset: %d\n", offset);
        printf("[DEBUG] Dimensione file: %d\n", handle->size);
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
