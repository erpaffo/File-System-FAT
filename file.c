#include "file.h"

// Crea file
FileHandle* create_file(Disk *disk, const char* filename) {

    int free_block = fat_alloc_block(&disk->fat);
    if (free_block == -1) {
        printf("Errore: Nessun blocco libero\n");
        return NULL;
    }

    FileHandle* handle = (FileHandle*) malloc(sizeof(FileHandle));
    handle -> start_block = free_block;
    handle -> current_block = free_block;
    handle->position = 0;
    handle -> size = 0;

    if (DEBUG) {
        printf("Creazione file %s. Primo blocco: %d\n", filename, free_block);
    }
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
    int block = handle->start_block;
    int position = handle->position;
    int read = 0;

    while (read < size) {
        int remaining = BLOCK_SIZE - position;
        int to_read = size - read;
        int read_size = remaining < to_read ? remaining : to_read;

        disk_read(disk, block, buffer + read);

        read += read_size;
        position += read_size;

        if (position == BLOCK_SIZE) {
            block = fat_get_next_block(&disk->fat, block);
            position = 0;
        }
    }

    handle->position = position;
}

// Cancella file
void erase_file(Disk* disk, FileHandle* handle, const char* filename) {
    int current_block = handle->start_block;
    while (current_block != -1) {
        int next_block = fat_get_next_block(&disk->fat, current_block);
        fat_free_block(&disk->fat, current_block);
        current_block = next_block;
    }
}

// Sposta all'offset specificato
void seek_file(FileHandle* handle, Disk* disk, int offset) {
    handle->position = offset;

    int current_block = handle->start_block;
    int current_offset = 0;

    while (current_offset + BLOCK_SIZE <= offset) {
        current_block = fat_get_next_block(&disk->fat, current_block);
        current_offset += BLOCK_SIZE;
    }

    int position = offset - current_offset;

    handle->current_block = current_block;
    handle->position = position;
}