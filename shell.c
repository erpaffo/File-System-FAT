#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "disk.h"
#include "fat.h"
#include "directory.h"
#include "file.h"
#include <readline/readline.h>
#include <readline/history.h>


#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 10

void prompt(Directory* current_dir) {
    printf("%s> ", current_dir->name);
}

int parse_input(char* input, char* args[]) {
    int arg_count = 0;
    int in_quotes = 0;
    char* ptr = input;
    char* start = NULL;

    while (*ptr != '\0') {
        while (*ptr == ' ') ptr++; // Salta gli spazi iniziali
        if (*ptr == '\0') break;

        if (*ptr == '"') {
            in_quotes = 1;
            ptr++;
            start = ptr;
            while (*ptr != '\0' && *ptr != '"') ptr++;
        } else {
            start = ptr;
            while (*ptr != '\0' && *ptr != ' ' && *ptr != '"') ptr++;
        }

        if (*ptr != '\0') {
            *ptr = '\0';
            ptr++;
        }

        args[arg_count++] = start;

        if (in_quotes) {
            in_quotes = 0;
        }
    }

    args[arg_count] = NULL;
    return arg_count;
}

void execute_command(char* input, Disk* disk, Directory** current_dir) {
    char* args[MAX_ARGS];
    int arg_count = parse_input(input, args);

    if (arg_count == 0) {
        return;
    }

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
    } else if (strcmp(args[0], "write") == 0) {
        if (arg_count < 2) {
            printf("Usage: write filename\n");
        } else {
            const char* filename = args[1];

            // Cerca se il file esiste
            FileHandle* file = open_file(disk, *current_dir, filename);
            if (file == NULL) {
                // Il file non esiste, crealo
                file = create_file(disk, filename);
                if (file == NULL) {
                    printf("Errore nella creazione del file\n");
                    return;
                }
                // Aggiorna la directory corrente
                (*current_dir)->entries[(*current_dir)->num_entries].start_block = file->start_block;
                (*current_dir)->entries[(*current_dir)->num_entries].is_directory = 0;
                strncpy((*current_dir)->entries[(*current_dir)->num_entries].name, filename, MAX_DIR_NAME - 1);
                (*current_dir)->num_entries++;
                disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
            } else {
                // Riposiziona l'handle all'inizio del file
                file->current_block = file->start_block;
                file->position = sizeof(FileMetadata);
                file->size = 0; // Sovrascrivi il contenuto esistente
            }

            // Modalità di inserimento testo
            printf("Inserisci il testo per %s (termina con una linea contenente solo \":EOF\"):\n", filename);
            char input_line[MAX_INPUT_SIZE];
            char* text = NULL;
            size_t total_size = 0;

            while (fgets(input_line, MAX_INPUT_SIZE, stdin)) {
                if (strcmp(input_line, ":EOF\n") == 0) {
                    break;
                }
                size_t len = strlen(input_line);
                char* new_text = realloc(text, total_size + len + 1);
                if (new_text == NULL) {
                    printf("Errore di memoria\n");
                    free(text);
                    break;
                }
                text = new_text;
                memcpy(text + total_size, input_line, len);
                total_size += len;
                text[total_size] = '\0';
            }

            // Scrivi il testo nel file
            if (text != NULL && total_size > 0) {
                write_file(file, disk, text, total_size);
                free(text);
            }

            free(file);
        }
    } else if (strcmp(args[0], "read") == 0) {
        if (arg_count < 2) {
            printf("Usage: read filename\n");
        } else {
            const char* filename = args[1];

            // Apri il file
            FileHandle* file = open_file(disk, *current_dir, filename);
            if (file == NULL) {
                printf("Il file %s non esiste.\n", filename);
                return;
            }

            if (file->size == 0) {
                printf("Il file è vuoto.\n");
                free(file);
                return;
            }

            // Riposiziona l'handle all'inizio del file
            file->current_block = file->start_block;
            file->position = sizeof(FileMetadata);

            // Alloca il buffer per leggere i dati
            char* buffer = (char*) malloc(file->size + 1);
            memset(buffer, 0, file->size + 1);

            read_file(file, disk, buffer, file->size);

            // Implementa la paginazione
            const int lines_per_page = 20;
            char* line = strtok(buffer, "\n");
            int line_count = 0;

            while (line != NULL) {
                printf("%s\n", line);
                line_count++;

                if (line_count % lines_per_page == 0) {
                    printf("-- Premere Invio per continuare, 'q' per uscire --\n");
                    int c = getchar();
                    if (c == 'q') {
                        break;
                    }
                }

                line = strtok(NULL, "\n");
            }

            // Libera le risorse
            free(buffer);
            free(file);
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
        // Creazione della directory root

        // Alloca il blocco 0 nella FAT
        disk->fat.entries[0].file = 1;        // Segna il blocco come utilizzato
        disk->fat.entries[0].next_block = -1; // Indica che non ci sono blocchi successivi
        disk->fat.free_blocks--;              // Decrementa il conteggio dei blocchi liberi

        // Inizializza la directory root
        memset(root_dir, 0, sizeof(Directory));
        strncpy(root_dir->name, "/", MAX_DIR_NAME - 1);
        root_dir->name[MAX_DIR_NAME - 1] = '\0';
        root_dir->start_block = 0;
        root_dir->parent_block = -1; // La directory root non ha genitore
        root_dir->num_entries = 0;

        // Scrivi la directory root sul disco
        disk_write(disk, 0, (const char*) root_dir);

        if (DEBUG) {
            printf("Root directory created and written to block 0\n");
        }
    } else {
        // Carica la directory root esistente dal disco
        disk_read(disk, 0, (char*) root_dir);
    }

    Directory* current_dir = root_dir;

    while (1) {
        // Crea il prompt con il nome della directory corrente
        char prompt_str[MAX_DIR_NAME + 4];
        snprintf(prompt_str, sizeof(prompt_str), "%s> ", current_dir->name);

        // Usa readline per leggere l'input dell'utente con il prompt personalizzato
        char* input = readline(prompt_str);

        if (input == NULL) {
            break; // EOF (Ctrl+D)
        }

        // Aggiungi il comando alla cronologia se non è vuoto
        if (strlen(input) > 0) {
            add_history(input);
        }

        execute_command(input, disk, &current_dir);

        free(input); // Libera la memoria allocata da readline
    }

    free(root_dir);
    disk_close(disk);

    return 0;
}
