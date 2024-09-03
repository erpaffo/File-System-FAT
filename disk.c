#include "disk.h"

// Inizializza il disco 
Disk* disk_init(const char* filename, int format) {
    // Apre file
    int fd = open(filename, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Imposta dimensione file
    int ret = ftruncate(fd, DISK_SIZE);
    if ( ret == -1) {
        perror("Error setting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mappa file in memoria
    char* buffer = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    Disk* disk = (Disk*) buffer;
    
    // Formatta disco se format = 1
    if (format) {
        disk_format(disk);
    }

    return disk;
}

void disk_format(Disk* disk) {
    memset(disk->data, 0, DISK_SIZE);
    disk->size = DISK_SIZE;
    for (int i = 0; i < FAT_SIZE; i++) {
        disk->fat.entries[i].next_block = -2;  // -2 indica blocco non usato
        disk->fat.entries[i].file = -2;        // -2 indica blocco non usato
    }
}


