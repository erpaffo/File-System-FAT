#include "fat.h"

// Inizializza la FAT
void fat_init(Fat* fat) {
    fat->free_block_idx = 1; // Lasciamo il blocco 0 alla root directory

    for (int i = 0; i < FAT_SIZE; i++) {
        fat->entries[i].next_block = -1;  // -1 indica che non c'è un blocco successivo
        fat->entries[i].file = -2;        // -2 indica che il blocco è libero
        if (DEBUG) {
            printf("[DEBUG] Blocco %d inizializzato. next_block = %d, file = %d\n", i, fat->entries[i].next_block, fat->entries[i].file);
        }
    }
}

// Restituisce il primo blocco libero
int fat_get_free_block(Fat* fat) {
    if (fat->free_block_idx == -1) {
        printf("[DEBUG] Nessun blocco libero disponibile\n");
        return -1;
    }

    if (DEBUG) {
        printf("[DEBUG] Ricerca del primo blocco libero\n");
    }
    
    int free_block = fat->free_block_idx;
    fat->free_block_idx = fat->entries[free_block].next_free_block;

    fat->entries[free_block].next_free_block = -1; 
    if (DEBUG) {
        printf("[DEBUG] Nessun blocco libero trovato\n");
    }

    return free_block;  
}

// Alloca un blocco
int fat_alloc_block(Fat* fat) {
    int free_block = fat->free_block_idx;
    if (free_block == -1) {
        return -1; // Nessun blocco libero
    }

    fat->free_block_idx = fat->entries[free_block].next_free_block;
    fat->entries[free_block].file = 1;
    fat->entries[free_block].next_block = -1;
    fat->entries[free_block].next_free_block = -1;
    return free_block;
}

// Libera un blocco
void fat_free_block(Fat* fat, int block) {
    // Verifica che il blocco sia valido prima di liberarlo
    if (block < 0 || block >= FAT_SIZE) {
        printf("Errore: blocco non valido\n");
        return;
    }

    // Verifica che il blocco sia attualmente in uso
    if (fat->entries[block].file == -2) {
        printf("[DEBUG] Il blocco %d è già libero!\n", block);
        return;
    }

    fat->entries[block].file = -2;  // Imposta il blocco come libero
    fat->entries[block].next_block = -1;  // Resetta il puntatore al blocco successivo
    fat->entries[block].next_free_block = fat->free_block_idx;
    fat->free_block_idx = block;

    if (DEBUG) {
        printf("[DEBUG] Blocco %d liberato e aggiunto alla free list.\n", block);
    }
}

// Restituisce il blocco successivo
int fat_get_next_block(Fat* fat, int block) {
    if (block < 0 || block >= FAT_SIZE) {
        printf("Errore: blocco non valido\n");
        return -1;
    }
    if (DEBUG) {
        printf("[DEBUG] Blocco successivo a %d: %d\n", block, fat->entries[block].next_block);
    }
    return fat->entries[block].next_block;
}

// Imposta il blocco successivo
void fat_set_next_block(Fat* fat, int block, int next_block) {
    if (block < 0 || block >= FAT_SIZE || (next_block != -1 && (next_block < 0 || next_block >= FAT_SIZE))) {
        printf("Errore: blocco non valido\n");
        return;
    }

    fat->entries[block].next_block = next_block;
    if (DEBUG) {
        printf("[DEBUG] Impostato blocco successivo per %d: %d\n", block, next_block);
    }
}