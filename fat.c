#include "fat.h"

// Inizializza la FAT
void fat_init(Fat* fat) {
    fat->free_blocks = FAT_SIZE - 1; // -1 perchè il primo blocco è riservato per la FAT
    for (int i = 0; i < FAT_SIZE; i++) {
        fat->entries[i].next_block = -1;  
        fat->entries[i].file = -2;
    }
}

// Restituisce il primo blocco libero
int fat_get_free_block(Fat* fat) {
    for (int i = 1; i < FAT_SIZE; i++) {
        if (fat->entries[i].file == -2) {
            return i;
        }
    }
    return -1;
}

// Alloca un blocco
int fat_alloc_block(Fat* fat) {
    if (fat->free_blocks == 0) {
        printf("No free blocks\n");
        return -1;
    }
    int block = fat_get_free_block(fat);
    fat->free_blocks--;
    return block;
}

// Libera un blocco
void fat_free_block(Fat* fat, int block) {
    fat->entries[block].file = -2;
    fat->free_blocks++;
}
