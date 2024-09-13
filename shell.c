#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "disk.h"
#include "fat.h"
#include "directory.h"
#include "file.h"

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 10

void prompt(Directory* current_dir) {
    printf("%s> ", current_dir->name);
}

void execute_command(char* input, Disk* disk, Directory** current_dir) {
    char* args[MAX_ARGS];
    int arg_count = 0;

    // Tokenizza l'input
    char* token = strtok(input, " ");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
        return; // Nessun comando inserito
    }

    // Mappa dei comandi
    if (strcmp(args[0], "ls") == 0) {
        list_dir(disk, *current_dir);
    } else if (strcmp(args[0], "touch") == 0) {
        if (arg_count < 2) {
            printf("Usage: touch filename\n");
        } else {
            // Creazione del file
            FileHandle* file = create_file(disk, args[1]);
            if (file != NULL) {
                // Aggiorna la directory corrente
                (*current_dir)->entries[(*current_dir)->num_entries].start_block = file->start_block;
                (*current_dir)->entries[(*current_dir)->num_entries].is_directory = 0;
                strncpy((*current_dir)->entries[(*current_dir)->num_entries].name, args[1], MAX_DIR_NAME - 1);
                (*current_dir)->num_entries++;
                disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
                free(file);
            }
        }
    } else if (strcmp(args[0], "mkdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: mkdir dirname\n");
        } else {
            create_dir(disk, *current_dir, args[1]);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            printf("Usage: cd dirname\n");
        } else if (strcmp(args[1], "..") == 0) {
            // Torna alla directory padre
            if ((*current_dir)->parent_block != -1) {
                Directory* parent_dir = (Directory*) malloc(sizeof(Directory));
                disk_read(disk, (*current_dir)->parent_block, (char*) parent_dir);
                *current_dir = parent_dir;
            } else {
                printf("Sei nella directory root\n");
            }
        } else {
            change_dir(disk, current_dir, args[1]);
        }
    } else if (strcmp(args[0], "rm") == 0) {
        if (arg_count < 2) {
            printf("Usage: rm filename\n");
        } else {
            // Rimozione del file
            int found = 0;
            for (int i = 0; i < (*current_dir)->num_entries; i++) {
                if (!(*current_dir)->entries[i].is_directory && strcmp((*current_dir)->entries[i].name, args[1]) == 0) {
                    FileHandle handle;
                    handle.start_block = (*current_dir)->entries[i].start_block;
                    erase_file(disk, &handle, args[1]);
                    // Rimuovi l'entry dalla directory
                    for (int j = i; j < (*current_dir)->num_entries - 1; j++) {
                        (*current_dir)->entries[j] = (*current_dir)->entries[j + 1];
                    }
                    (*current_dir)->num_entries--;
                    disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                printf("File non trovato\n");
            }
        }
    } else if (strcmp(args[0], "rmdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: rmdir dirname\n");
        } else {
            // Rimozione della directory
            int found = 0;
            for (int i = 0; i < (*current_dir)->num_entries; i++) {
                if ((*current_dir)->entries[i].is_directory && strcmp((*current_dir)->entries[i].name, args[1]) == 0) {
                    Directory dir_to_delete;
                    disk_read(disk, (*current_dir)->entries[i].start_block, (char*)&dir_to_delete);
                    erase_dir(disk, &dir_to_delete);
                    // Rimuovi l'entry dalla directory
                    for (int j = i; j < (*current_dir)->num_entries - 1; j++) {
                        (*current_dir)->entries[j] = (*current_dir)->entries[j + 1];
                    }
                    (*current_dir)->num_entries--;
                    disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                printf("Directory non trovata\n");
            }
        }
    } else if (strcmp(args[0], "exit") == 0) {
        // Libera le risorse e termina
        disk_close(disk);
        exit(0);
    } else {
        printf("Comando non riconosciuto: %s\n", args[0]);
    }
}

int main() {
    int format = 1; // Imposta a 1 per formattare, 0 per non formattare
    Disk* disk = disk_init("disk.img", format);

    // Alloca e inizializza la directory root
    Directory* root_dir = (Directory*) malloc(sizeof(Directory));

    if (format) {
        // **Creazione della directory root**

        // **Alloca il blocco 0 nella FAT**
        disk->fat.entries[0].file = 1;       // Segna il blocco come utilizzato
        disk->fat.entries[0].next_block = -1; // Indica che non ci sono blocchi successivi
        disk->fat.free_blocks--;             // Decrementa il conteggio dei blocchi liberi

        // **Inizializza la directory root**
        memset(root_dir, 0, sizeof(Directory));
        strncpy(root_dir->name, "/", MAX_DIR_NAME - 1);
        root_dir->name[MAX_DIR_NAME - 1] = '\0';
        root_dir->start_block = 0;
        root_dir->parent_block = -1; // La directory root non ha genitore
        root_dir->num_entries = 0;

        // **Scrivi la directory root sul disco**
        disk_write(disk, 0, (const char*) root_dir);

        if (DEBUG) {
            printf("Root directory created and written to block 0\n");
        }
    } else {
        // **Carica la directory root esistente dal disco**
        disk_read(disk, 0, (char*) root_dir);
    }

    Directory* current_dir = root_dir;

    char input[MAX_INPUT_SIZE];

    while (1) {
        prompt(current_dir);
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break; // EOF
        }

        input[strcspn(input, "\n")] = 0;

        execute_command(input, disk, &current_dir);
    }

    free(root_dir);
    disk_close(disk);

    return 0;
}