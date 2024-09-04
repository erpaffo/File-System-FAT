#include "disk.h"
#include "fat.h"

int main() {
    printf("=======================\n");
    printf("Test disk\n");
    printf("=======================\n");
    if (DEBUG) {
        printf("Inizializzazione del disco\n");
    }
    Disk* disk = disk_init("test_disk.bin", 1);

    const char* data_to_write = "Test di Prova per Disco!";
    
    if (DEBUG) {
        printf("Scrittura su disco\n");
    }
    disk_write(disk, 0, data_to_write);

    char buffer[BLOCK_SIZE];

    if (DEBUG) {
        printf("Lettura su disco\n");
    }
    disk_read(disk, 0, buffer);

    printf("Dati letti dal disco: %s\n", buffer);

    if (DEBUG) {
        printf("Chiusura del disco\n");
    }
    disk_close(disk);

    if (DEBUG) {
        printf("Test completato\n");
        printf("=======================\n");
        printf("Test FAT\n");
        printf("=======================\n");
    }

    Fat fat;

    if (DEBUG) {
        printf("Inizializzazione della FAT\n");
    }
    fat_init(&fat);

    if (DEBUG) {
        printf("Controllo se tutti i blocchi sono vuoti\n");
    }
    for (int i = 0; i < FAT_SIZE; i++) {
        if (fat.entries[i].file != -2 || fat.entries[i].next_block != -1) {
            printf("Errore: Il blocco %d non è inizializzato correttamente\n", i);
            return -1;
        }
    }

    if (DEBUG) {
        printf("Allocazione di un blocco\n");
    }
    int block = fat_alloc_block(&fat);
    if (block == -1) {
        printf("Errore: impossibile allocare un blocco\n");
        return -1;
    }

    if (DEBUG) {
        printf("Controllo se il blocco allocato è corretto\n");
    }
    if (fat.entries[block].file != -2 || fat.entries[block].next_block != -1) {
        printf("Errore: Il blocco allocato non è inizializzato correttamente\n");
        return -1;
    }

    if (DEBUG) {
        printf("Allocazione di un altro blocco\n");
    }
    int second_block = fat_alloc_block(&fat);
    if (second_block == -1) {
        printf("Errore: impossibile allocare il secondo blocco\n");
        return -1;
    }

    if (DEBUG) {
        printf("Collegamento tra i due blocchi\n");
    }
    fat_set_next_block(&fat, block, second_block);

    if (DEBUG) {
        printf("Controllo del collegamento tra i due blocchi\n");
    }
    if (fat_get_next_block(&fat, block) != second_block) {
        printf("Errore: Collegamento tra blocchi fallito\n");
        return -1;
    }
    printf("Collegamento blocco %d -> blocco %d: OK\n", block, second_block);

    if (DEBUG) 
        printf("Liberazione di un blocco\n");
    fat_free_block(&fat, block);

    if (DEBUG) {
        printf("Verfica blocco liberato correttamente");
    }
    if (fat.entries[block].file != -2 || fat.entries[block].next_block != -1) {
        printf("Errore: Il blocco %d non è stato liberato correttamente\n", block);
        return -1;
    }
    printf("Liberazione blocco %d: OK\n", block);

    if (DEBUG) {
        printf("Allocazione di tutti i blocchi");
    }

    int alloc_count = 0;
    while (fat_alloc_block(&fat) != -1) {
        alloc_count++;
    }

    if (DEBUG) {
        printf("Verfiica dell'allocazione di tutti i blocci");
    }
    if (alloc_count != FAT_SIZE - 1) {
        printf("Errore: Blocco esaurito non gestito correttamente\n");
        return -1;
    }
    printf("Esaurimento blocchi: OK\n");


    return 0;
}