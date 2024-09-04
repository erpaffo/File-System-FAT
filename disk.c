#include "disk.h"

// Inizializza il disco 
Disk* disk_init(const char* filename, int format) {
    // Apre file
    if (DEBUG) {
        printf("Opening file %s\n", filename);
    }

    int fd = open(filename, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Imposta dimensione file
    if (DEBUG) {
        printf("Setting file size\n");
    }
    int ret = ftruncate(fd, DISK_SIZE);
    if ( ret == -1) {
        perror("Error setting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mappa file in memoria
    if (DEBUG) {
        printf("Mapping file\n");
    }
    char* buffer = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    if (DEBUG) {
        printf("File mapped at address %p\n", buffer);
        printf("Closing file\n");
    }
    close(fd);

    Disk* disk = (Disk*) buffer;
    
    // Formatta disco se format = 1
    if (format) {
        if (DEBUG) {
            printf("Formatting disk\n");
        }
        disk_format(disk);
    }
    if (DEBUG) {
        printf("Disk initialized\n");
    }

    return disk;
}

// Formatta il disco (principalmente è un helper per disk_init)
void disk_format(Disk* disk) {
    memset(disk->data, 0, DISK_SIZE-sizeof(Disk));
    disk->size = DISK_SIZE;
    for (int i = 0; i < FAT_SIZE; i++) {
        disk->fat.entries[i].next_block = -2;  // -2 indica blocco non usato
        disk->fat.entries[i].file = -2;        // -2 indica blocco non usato
    }
}

// Legge un blocco dal disco
void disk_read(Disk* disk, int block, char* buffer) {
    if (block >= FAT_SIZE) {
        perror("Error reading block: block exceeds FAT size");
        exit(EXIT_FAILURE);
    }

    if (disk->fat.entries[block].file == -2) {
        perror("Error reading block: block not used");
        exit(EXIT_FAILURE);
    }

    memcpy(buffer, disk->data + block * BLOCK_SIZE, BLOCK_SIZE);
}

// scrive un blocco sul disco
void disk_write(Disk* disk, int block, const char* buffer) {
    if (block >= FAT_SIZE) {
        perror("Error writing block: block exceeds FAT size");
        exit(EXIT_FAILURE);
    }

    memcpy(disk->data + block * BLOCK_SIZE, buffer, BLOCK_SIZE);

    if (disk->fat.entries[block].file == -2) {
        disk->fat.entries[block].file = 1;
        disk->fat.free_blocks--;
    }
}

// Chiude il disco liberando la memoria
void disk_close(Disk* disk) {
    int ret = munmap(disk, DISK_SIZE);
    if (ret == -1) {
        perror("Error unmapping file");
        exit(EXIT_FAILURE);
    }

}
