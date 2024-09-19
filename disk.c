#include "disk.h"
#include "fat.h"

Disk* disk_init(const char* filename, int format) {
    // Apre il file del disco
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Errore nell'apertura del file del disco");
        return NULL;
    }

    // Imposta la dimensione del file del disco
    if (lseek(fd, DISK_SIZE - 1, SEEK_SET) == -1) {
        perror("Errore durante l'impostazione della dimensione del disco");
        close(fd);
        return NULL;
    }
    if (write(fd, "", 1) != 1) {
        perror("Errore durante la scrittura nel file del disco");
        close(fd);
        return NULL;
    }

    // Mappa il file del disco in memoria
    Disk* disk = (Disk*) mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Errore durante la mappatura del disco in memoria");
        close(fd);
        return NULL;
    }

    close(fd); // Il file descriptor non è più necessario dopo la mappatura

    // Se stiamo formattando, inizializziamo la FAT e i dati
    if (format) {
        disk_format(disk);
    }

    return disk;
}

void disk_format(Disk* disk) {
    disk->size = DISK_SIZE;

    // Inizializza la FAT
    fat_init(&(disk->fat));

    // Inizializza i dati del disco a zero
    memset(disk->data, 0, DISK_SIZE - sizeof(Disk));

    if (DEBUG) {
        printf("Disco formattato con successo.\n");
    }
}

// Legge un blocco dal disco
void disk_read(Disk* disk, int block, char* buffer) {
    if (block < 0 || block >= FAT_SIZE) {
        printf("Errore: numero di blocco fuori dai limiti.\n");
        return;
    }

    memcpy(buffer, disk->data + block * BLOCK_SIZE, BLOCK_SIZE);
}

// Scrive un blocco nel disco
void disk_write(Disk* disk, int block, const char* buffer) {
    if (block < 0 || block >= FAT_SIZE) {
        printf("Errore: numero di blocco fuori dai limiti.\n");
        return;
    }

    memcpy(disk->data + block * BLOCK_SIZE, buffer, BLOCK_SIZE);
}

// Chiude il disco e sincronizza i dati
void disk_close(Disk* disk) {
    // Sincronizza le modifiche sul disco
    if (msync(disk, DISK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing disk");
    }

    // Smunmap la memoria
    if (munmap(disk, DISK_SIZE) == -1) {
        perror("Error unmapping file");
        exit(EXIT_FAILURE);
    }
}

// Salva lo stato della FAT
void disk_save_fat(Disk* disk) {
    msync(&(disk->fat), sizeof(Fat), MS_SYNC);  // Sincronizza le modifiche della FAT sul disco
    if (DEBUG) {
        printf("FAT salvata correttamente.\n");
    }
}