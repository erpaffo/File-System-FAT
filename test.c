#include "disk.h"
#include "fat.h"
#include "file.h"

// Variabili globali per il conteggio dei test passati e falliti
int tests_passed = 0;
int tests_failed = 0;

// Funzione per registrare il passaggio di un test
void test_pass(const char* test_name) {
    printf("[PASSED] %s\n", test_name);
    tests_passed++;
}

// Funzione per registrare il fallimento di un test
void test_fail(const char* test_name) {
    printf("[FAILED] %s\n", test_name);
    tests_failed++;
}

void test_disk() {
    printf("\n=======================\n");
    printf("Test disk\n");
    printf("=======================\n");
    Disk* disk = disk_init("test_disk.bin", 1);

    const char* data_to_write = "Test di Prova per Disco!";
    disk_write(disk, 0, data_to_write);

    char buffer[BLOCK_SIZE];
    disk_read(disk, 0, buffer);

    if (strcmp(buffer, data_to_write) == 0) {
        test_pass("test_disk");
    } else {
        test_fail("test_disk");
    }

    disk_close(disk);
}

void test_fat() {
    printf("\n=======================\n");
    printf("Test FAT\n");
    printf("=======================\n");

    Fat fat;
    fat_init(&fat);

    for (int i = 0; i < FAT_SIZE; i++) {
        if (fat.entries[i].file != -2 || fat.entries[i].next_block != -1) {
            test_fail("test_fat");
            return;
        }
    }

    int block = fat_alloc_block(&fat);
    if (block == -1 || fat.entries[block].file == -2) {
        test_fail("test_fat");
        return;
    }

    int second_block = fat_alloc_block(&fat);
    if (second_block == -1) {
        test_fail("test_fat");
        return;
    }

    fat_set_next_block(&fat, block, second_block);
    if (fat_get_next_block(&fat, block) != second_block) {
        test_fail("test_fat");
        return;
    }

    fat_free_block(&fat, block);
    if (fat.entries[block].file != -2 || fat.entries[block].next_block != -1) {
        test_fail("test_fat");
        return;
    }

    int alloc_count = 0;
    while (fat_alloc_block(&fat) != -1) {
        alloc_count++;
    }

    if (alloc_count == FAT_SIZE - 1) {
        test_pass("test_fat");
    } else {
        test_fail("test_fat");
    }
}

void test_create_file() {
    printf("\n=======================\n");
    printf("Test create_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    if (file != NULL) {
        test_pass("test_create_file");
    } else {
        test_fail("test_create_file");
    }

    free(file);
    disk_close(disk);
}

void test_write_file() {
    printf("\n=======================\n");
    printf("Test write_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    const char* data_to_write = "Test di Prova per File!";
    write_file(file, disk, data_to_write, strlen(data_to_write) + 1);

    char buffer[BLOCK_SIZE];
    disk_read(disk, file->start_block, buffer);

    if (strcmp(buffer, data_to_write) == 0) {
        test_pass("test_write_file");
    } else {
        test_fail("test_write_file");
    }

    free(file);
    disk_close(disk);
}

void test_read_file() {
    printf("\n=======================\n");
    printf("Test read_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    const char* data_to_write = "Test di Prova per File!";
    write_file(file, disk, data_to_write, strlen(data_to_write) + 1);

    char buffer[BLOCK_SIZE];
    read_file(file, disk, buffer, strlen(data_to_write) + 1);

    if (strcmp(buffer, data_to_write) == 0) {
        test_pass("test_read_file");
    } else {
        test_fail("test_read_file");
    }

    free(file);
    disk_close(disk);
}

void test_erase_file() {
    printf("\n=======================\n");
    printf("Test erase_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    const char* data_to_write = "Dati da cancellare!";
    write_file(file, disk, data_to_write, strlen(data_to_write) + 1);

    erase_file(disk, file, "test_file");

    if (disk->fat.entries[file->start_block].file == -2) {
        test_pass("test_erase_file");
    } else {
        test_fail("test_erase_file");
    }

    free(file);
    disk_close(disk);
}

void test_seek_file() {
    printf("\n=======================\n");
    printf("Test seek_file\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    FileHandle* file = create_file(disk, "test_file");

    const char* data_to_write = "Questa è una stringa lunga per testare seek!";
    write_file(file, disk, data_to_write, strlen(data_to_write) + 1);

    seek_file(file, disk, 10);  // Sposta la posizione a 10 byte

    char buffer[BLOCK_SIZE];
    read_file(file, disk, buffer, strlen(data_to_write) - 10);  // Legge dal byte 10 in poi

    if (DEBUG) {
        printf("[DEBUG] Dati letti: %s\n", buffer);
        printf("[DEBUG] Dati attesi: %s\n", data_to_write + 10);
    }
    if (strcmp(buffer, data_to_write + 10) == 0) {
        test_pass("test_seek_file");
    } else {
        test_fail("test_seek_file");
    }

    free(file);
    disk_close(disk);
}

int main() {
    test_disk();
    test_fat();
    test_create_file();
    test_write_file();
    test_read_file();
    test_erase_file();
    test_seek_file();

    // Riassunto dei risultati
    printf("\n=======================\n");
    printf("Test Summary\n");
    printf("=======================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    return 0;
}
