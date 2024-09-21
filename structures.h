#include "settings.h"
// Struttura per entry nella FAT
typedef struct {
    int next_block; // -1 per ultimo blocco
    int file;       // -2 per blocco non usato, 1 per blocco in uso
    int next_free_block; //indice del prossimo blocco libero
} FatEntry;

// Struttura File Allocation Table (FAT)
typedef struct {
    int free_block_idx; // Indice del primo blocco libero
    FatEntry entries[FAT_SIZE];
} Fat;

typedef struct {
    int size;       // Dimensione del disco
    Fat fat;        // File Allocation Table per il disco
    char data[];    // Dati del disco (mappati in memoria)
} Disk;

typedef struct {
    char name[MAX_FILENAME_LENGTH]; // Nome del file o della directory
    int start_block;                // Blocco iniziale sul disco
    int size;                       // Dimensione in byte (0 per directory)
    int is_directory;               // 1 se è una directory, 0 se è un file
} FileControlBlock;

// Struttura per gestire i file aperti
typedef struct {
    FileControlBlock fcb;  // FCB associato al file
    int current_block;     // Blocco corrente durante le operazioni di I/O
    int position;          // Posizione corrente nel file
} FileHandle;

// Struttura per gestire i file e le directory
typedef struct {
    FileControlBlock fcb;              // FCB dell'entry (file o directory)
} DirectoryEntry;

// Struttura per la directory
typedef struct {
    FileControlBlock fcb;                     // FCB della directory stessa
    int num_entries;                          // Numero di entries nella directory
    FileControlBlock entries[MAX_DIR_ENTRIES]; // Array di FCB delle entries (file o directory)
    int parent_block;                         // Blocco della directory padre
} Directory;