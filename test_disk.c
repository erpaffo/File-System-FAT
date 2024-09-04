#include "disk.h"

int main() {
    if (DEBUG) {
        printf("=======Test disk=======\n");
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

    disk_close(disk);

    return 0;
}