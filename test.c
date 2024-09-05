#include "disk.h"
#include "fat.h"
#include "file.h"

void test_disk() {
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
    }
}

void test_fat() {
    printf("=======================\n");
    printf("Test FAT\n");
    printf("=======================\n");

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
            return;
        }
    }

    if (DEBUG) {
        printf("Allocazione di un blocco\n");
    }
    int block = fat_alloc_block(&fat);
    if (block == -1) {
        printf("Errore: impossibile allocare un blocco\n");
        return;
    }

    if (DEBUG) {
        printf("Controllo se il blocco allocato è corretto\n");
    }
    if (fat.entries[block].file != -2 || fat.entries[block].next_block != -1) {
        printf("Errore: Il blocco allocato non è inizializzato correttamente\n");
        return;
    }

    if (DEBUG) {
        printf("Allocazione di un altro blocco\n");
    }
    int second_block = fat_alloc_block(&fat);
    if (second_block == -1) {
        printf("Errore: impossibile allocare il secondo blocco\n");
        return;
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
        return;
    }
    printf("Collegamento blocco %d -> blocco %d: OK\n", block, second_block);

    if (DEBUG) 
        printf("Liberazione di un blocco\n");
    fat_free_block(&fat, block);

    if (DEBUG) {
        printf("Verifica blocco liberato correttamente\n");
    }
    if (fat.entries[block].file != -2 || fat.entries[block].next_block != -1) {
        printf("Errore: Il blocco %d non è stato liberato correttamente\n", block);
        return;
    }
    printf("Liberazione blocco %d: OK\n", block);

    if (DEBUG) {
        printf("Allocazione di tutti i blocchi\n");
    }

    int alloc_count = 0;
    while (fat_alloc_block(&fat) != -1) {
        alloc_count++;
    }

    if (DEBUG) {
        printf("Verifica dell'allocazione di tutti i blocchi\n");
    }
    if (alloc_count != FAT_SIZE - 1) {
        printf("Errore: Blocco esaurito non gestito correttamente. Alloc count: %d\n", alloc_count);
        return;
    }
    printf("Esaurimento blocchi: OK\n");
}

void test_create_file() {
    printf("=======================\n");
    printf("Test create_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    if (file == NULL) {
        printf("Errore: create_file ha restituito NULL\n");
        return;
    }

    printf("create_file: OK\n");

    free(file);
    disk_close(disk);
}

void test_write_file() {
    printf("=======================\n");
    printf("Test write_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    const char* data_to_write = "Test di Prova per File!";
    write_file(file, disk, data_to_write, strlen(data_to_write) + 1);

    char buffer[BLOCK_SIZE];
    disk_read(disk, file->start_block, buffer);

    if (strcmp(buffer, data_to_write) != 0) {
        printf("Errore: I dati scritti non corrispondono\n");
        return;
    }

    printf("write_file: OK\n");

    free(file);
    disk_close(disk);
}

void test_read_file() {
    printf("=======================\n");
    printf("Test read_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    const char* data_to_write = "Test di Prova per File!";
    write_file(file, disk, data_to_write, strlen(data_to_write) + 1);

    char buffer[BLOCK_SIZE];
    read_file(file, disk, buffer, strlen(data_to_write) + 1);

    if (strcmp(buffer, data_to_write) != 0) {
        printf("Errore: I dati letti non corrispondono\n");
        return;
    }

    printf("read_file: OK\n");

    free(file);
    disk_close(disk);
}

int main() {
    test_disk();
    test_fat();
    test_create_file();
    test_write_file();
    test_read_file();
    return 0;
}