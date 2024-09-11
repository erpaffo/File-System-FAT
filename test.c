#include "disk.h"
#include "fat.h"
#include "file.h"
#include "directory.h"

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

    // Richiesta di seek a 10 byte
    seek_file(file, disk, 10);

    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);  // Azzera il buffer per sicurezza

    read_file(file, disk, buffer, strlen(data_to_write) - 10);  // Legge dal byte 10 in poi

    if (DEBUG) {
        printf("[DEBUG] Dati letti: '%s'\n", buffer);
        printf("[DEBUG] Dati attesi: '%s'\n", data_to_write + 10);
    }

    // Confronto del contenuto dei buffer
    if (strcmp(buffer, data_to_write + 10) == 0) {
        test_pass("test_seek_file");
    } else {
        printf("[DEBUG] Buffer read does not match expected!\n");
        test_fail("test_seek_file");
    }

    free(file);
    disk_close(disk);
}


void test_create_directory() {
    printf("\n=======================\n");
    printf("Test create_directory\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    Directory* root = create_dir(disk, NULL, "root");

    if (root != NULL && strcmp(root->name, "root") == 0) {
        test_pass("test_create_directory");
    } else {
        test_fail("test_create_directory");
    }

    free(root);
    disk_close(disk);
}

void test_create_subdirectory() {
    printf("\n=======================\n");
    printf("Test create_subdirectory\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    Directory* root = create_dir(disk, NULL, "root");
    Directory* subdir = create_dir(disk, root, "subdir");

    if (subdir != NULL && strcmp(subdir->name, "subdir") == 0 && root->num_entries == 1) {
        test_pass("test_create_subdirectory");
    } else {
        test_fail("test_create_subdirectory");
    }

    free(subdir);
    free(root);
    disk_close(disk);
}

void test_list_directory() {
    printf("\n=======================\n");
    printf("Test list_directory\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    Directory* root = create_dir(disk, NULL, "root");
    create_dir(disk, root, "subdir1");
    create_dir(disk, root, "subdir2");

    printf("Listing contents of root directory:\n");
    list_dir(disk, root);

    if (root->num_entries == 2) {
        test_pass("test_list_directory");
    } else {
        test_fail("test_list_directory");
    }

    free(root);
    disk_close(disk);
}

void test_change_directory() {
    printf("\n=======================\n");
    printf("Test change_directory\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    Directory* root = create_dir(disk, NULL, "root");
    Directory* subdir = create_dir(disk, root, "subdir");

    Directory* current_dir = root;
    change_dir(disk, &current_dir, "subdir");

    if (current_dir != NULL && strcmp(current_dir->name, "subdir") == 0) {
        test_pass("test_change_directory");
    } else {
        test_fail("test_change_directory");
    }

    free(subdir);
    free(root);
    disk_close(disk);
}

void test_erase_directory() {
    printf("\n=======================\n");
    printf("Test erase_directory\n");
    printf("=======================\n");

    Disk* disk = disk_init("test_disk.bin", 1);
    Directory* root = create_dir(disk, NULL, "root");
    Directory* subdir = create_dir(disk, root, "subdir");

    erase_dir(disk, subdir);

    if (disk->fat.entries[subdir->start_block].file == -2) {
        test_pass("test_erase_directory");
    } else {
        test_fail("test_erase_directory");
    }

    free(root);
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

    test_create_directory();
    test_create_subdirectory();
    test_list_directory();
    test_change_directory();
    test_erase_directory();

    // Riassunto dei risultati
    printf("\n=======================\n");
    printf("Test Summary\n");
    printf("=======================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    return 0;
}
