#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "disk.h"
#include "fat.h"
#include "directory.h"
#include "file.h"

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 10

void prompt(Directory* current_dir) {
    printf("%s> ", current_dir->fcb.name);
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

void execute_command(char* input, Disk* disk, Directory** current_dir, const char* disk_filename, const char* temp_disk_filename) {
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
            const char* filename = args[1];
            int exists = 0;
            for (int i = 0; i < (*current_dir)->num_entries; i++) {
                if (strcmp((*current_dir)->entries[i].name, filename) == 0) {
                    printf("Il file '%s' esiste già.\n", filename);
                    exists = 1;
                    break;
                }
            }
            if (!exists) {
                FileHandle* file = create_file(disk, filename);
                if (file != NULL) {
                    (*current_dir)->entries[(*current_dir)->num_entries] = file->fcb;
                    (*current_dir)->num_entries++;
                    disk_write(disk, (*current_dir)->fcb.start_block, (const char*)*current_dir);
                    disk_save_fat(disk); // Salva la FAT
                    free(file);
                }
            }
        }
    } else if (strcmp(args[0], "mkdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: mkdir dirname\n");
        } else {
            create_dir(disk, *current_dir, args[1]);
            disk_write(disk, (*current_dir)->fcb.start_block, (const char*)*current_dir);
            disk_save_fat(disk); // Salva la FAT
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            printf("Usage: cd dirname\n");
        } else if (strcmp(args[1], "..") == 0) {
            if ((*current_dir)->parent_block != -1) {
                Directory* parent_dir = (Directory*) malloc(sizeof(Directory));
                if (parent_dir == NULL) {
                    printf("Errore di allocazione memoria per Directory\n");
                    return;
                }
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
            int found = 0;
            for (int i = 0; i < (*current_dir)->num_entries; i++) {
                if (!(*current_dir)->entries[i].is_directory && strcmp((*current_dir)->entries[i].name, args[1]) == 0) {
                    FileHandle handle;
                    handle.fcb = (*current_dir)->entries[i];
                    erase_file(disk, &handle, args[1]);
                    // Rimuovi l'entry dalla directory
                    for (int j = i; j < (*current_dir)->num_entries - 1; j++) {
                        (*current_dir)->entries[j] = (*current_dir)->entries[j + 1];
                    }
                    (*current_dir)->num_entries--;
                    disk_write(disk, (*current_dir)->parent_block, (const char*)*current_dir);
                    disk_save_fat(disk); // Salva la FAT
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
                    disk_write(disk, (*current_dir)->fcb.start_block, (const char*)*current_dir);
                    disk_save_fat(disk); // Salva la FAT
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
            printf("Usage: write filename [text]\n");
        } else {
            const char* filename = args[1];
            FileHandle* file = open_file(disk, *current_dir, filename);
            if (file == NULL) {
                printf("Il file %s non esiste.\n", filename);
                printf("Vuoi creare il file? (s/n): ");
                char response[10];
                fgets(response, sizeof(response), stdin);
                response[strcspn(response, "\n")] = 0;
                if (strcmp(response, "s") == 0 || strcmp(response, "S") == 0) {
                    file = create_file(disk, filename);
                    if (file == NULL) {
                        printf("Errore nella creazione del file\n");
                        return;
                    }
                    (*current_dir)->entries[(*current_dir)->num_entries] = file->fcb;
                    (*current_dir)->num_entries++;
                    disk_write(disk, (*current_dir)->fcb.start_block, (const char*)*current_dir);
                    disk_save_fat(disk); // Salva la FAT
                } else {
                    printf("Operazione annullata.\n");
                    return;
                }
            }

            if (file != NULL) {
                if (arg_count == 3) {
                    const char* text = args[2];
                    write_file(file, disk, text, strlen(text));
                } else {
                    printf("Inserisci il testo per %s (termina con una linea contenente solo \":EOF\"):\n", filename);
                    char input_line[MAX_INPUT_SIZE];
                    char* text = NULL;
                    size_t total_size = 0;

                    while (fgets(input_line, MAX_INPUT_SIZE, stdin)) {
                        if (strcmp(input_line, ":EOF\n") == 0 || strcmp(input_line, ":EOF\r\n") == 0) {
                            break;
                        }
                        size_t len = strlen(input_line);
                        char* new_text = realloc(text, total_size + len);
                        if (new_text == NULL) {
                            printf("Errore di memoria\n");
                            free(text);
                            break;
                        }
                        text = new_text;
                        memcpy(text + total_size, input_line, len);
                        total_size += len;
                    }

                    if (text != NULL && total_size > 0) {
                        write_file(file, disk, text, total_size);
                        free(text);
                    }
                }

                // Aggiorna l'entry nella directory
                for (int i = 0; i < (*current_dir)->num_entries; i++) {
                    if (strcmp((*current_dir)->entries[i].name, filename) == 0) {
                        (*current_dir)->entries[i] = file->fcb;
                        break;
                    }
                }
                disk_write(disk, (*current_dir)->fcb.start_block, (const char*)*current_dir);
                disk_save_fat(disk); // Salva la FAT
                free(file);
            }
        }
    } else if (strcmp(args[0], "read") == 0) {
        if (arg_count < 2) {
            printf("Usage: read filename\n");
        } else {
            const char* filename = args[1];
            FileHandle* file = open_file(disk, *current_dir, filename);
            if (file == NULL) {
                printf("Il file %s non esiste.\n", filename);
                return;
            }

            if (file->fcb.size == 0) {
                printf("Il file è vuoto.\n");
                free(file);
                return;
            }

            file->current_block = file->fcb.start_block;
            file->position = 0;

            char* buffer = (char*) malloc(file->fcb.size + 1);
            if (buffer == NULL) {
                printf("Errore di allocazione memoria per il buffer di lettura\n");
                free(file);
                return;
            }
            memset(buffer, 0, file->fcb.size + 1);
            read_file(file, disk, buffer, file->fcb.size);

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

            free(buffer);
            free(file);
        }
    } else if (strcmp(args[0], "help") == 0) {
        printf("Elenco dei comandi disponibili:\n");
        printf("ls                 - Elenca il contenuto della directory corrente\n");
        printf("touch filename     - Crea un nuovo file\n");
        printf("mkdir dirname      - Crea una nuova directory\n");
        printf("cd dirname         - Cambia la directory corrente\n");
        printf("rm filename        - Rimuove un file\n");
        printf("rmdir dirname      - Rimuove una directory\n");
        printf("write filename     - Scrive su un file\n");
        printf("read filename      - Legge da un file\n");
        printf("memory num_blocks  - Mostra lo stato dei blocchi di memoria del disco\n");
        printf("clear              - Pulisce lo schermo della shell\n");
        printf("exit               - Esce dalla shell\n");
        printf("help               - Mostra questo messaggio di aiuto\n");
    } else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        printf("Vuoi salvare le tue modifiche? (s/n): ");
        char response[10];
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = 0;
        if (strcmp(response, "s") == 0 || strcmp(response, "S") == 0) {
            disk_save_fat(disk);
            printf("Modifiche salvate.\n");
            remove(disk_filename);
            rename(temp_disk_filename, disk_filename);
        } else {
            printf("Modifiche non salvate.\n");
            remove(temp_disk_filename);
        }
        disk_close(disk);
        exit(0);
    } else if (strcmp(args[0], "memory") == 0) {
        if (arg_count < 2) {
            printf("Usage: memory num_blocks\n");
        } else {
            int num_blocks = atoi(args[1]);  
            if (num_blocks <= 0) {
                printf("Errore: numero di blocchi non valido\n");
            } else {
                FILE *file = fopen("memory.txt", "w");
                if (!file) {
                    printf("Errore: impossibile creare il file memory.txt\n");
                    return;
                }

                int blocks_per_row = 8; 
                int blocks_to_print = (num_blocks < FAT_SIZE) ? num_blocks : FAT_SIZE; 

                for (int r = 0; r < (blocks_to_print + blocks_per_row - 1) / blocks_per_row; r++) {
                    for (int j = 0; j < blocks_per_row && r * blocks_per_row + j < blocks_to_print; j++) {
                        fprintf(file, "+----------");
                    }
                    fprintf(file, "+\n|");

                    for (int j = 0; j < blocks_per_row && r * blocks_per_row + j < blocks_to_print; j++) {
                        int block_num = r * blocks_per_row + j;
                        fprintf(file, " Block %3d |", block_num);
                    }
                    fprintf(file, "\n+----------");

                    for (int j = 0; j < blocks_per_row && r * blocks_per_row + j < blocks_to_print; j++) {
                        fprintf(file, "+----------");
                    }
                    fprintf(file, "+\n|");

                    for (int j = 0; j < blocks_per_row && r * blocks_per_row + j < blocks_to_print; j++) {
                        int block_num = r * blocks_per_row + j;
                        if (disk->fat.entries[block_num].next_block == -2) { // Supponendo -2 rappresenti Free
                            fprintf(file, "   Free    |");
                        } else {
                            fprintf(file, "  Blocked  |");
                        }
                    }
                    fprintf(file, "\n+----------");

                    for (int j = 0; j < blocks_per_row && r * blocks_per_row + j < blocks_to_print; j++) {
                        fprintf(file, "+----------");
                    }
                    fprintf(file, "+\n|");

                    for (int j = 0; j < blocks_per_row && r * blocks_per_row + j < blocks_to_print; j++) {
                        int block_num = r * blocks_per_row + j;
                        if (disk->fat.entries[block_num].next_block == -1) {
                            fprintf(file, "  Next:-1  |");
                        } else {
                            fprintf(file, " Next:%3d  |", disk->fat.entries[block_num].next_block);
                        }
                    }
                    fprintf(file, "\n");
                }

                fprintf(file, "+----------");
                for (int j = 0; j < blocks_per_row && j < blocks_to_print; j++) {
                    fprintf(file, "+----------");
                }
                fprintf(file, "+\n");
                fclose(file);

                printf("Lo stato della memoria è stato scritto in memory.txt\n");
            }
        }
    } else if (strcmp(args[0], "clear") == 0) {
        system("clear");
    } else {
        printf("Comando non riconosciuto: %s\n", args[0]);
    }
}

int main() {
    char disk_filename[256];

    printf("Inserisci il nome o il percorso del disco da aprire: ");
    if (fgets(disk_filename, sizeof(disk_filename), stdin) == NULL) {
        printf("Errore nella lettura del nome del disco.\n");
        return 1;
    }
    disk_filename[strcspn(disk_filename, "\n")] = 0;

    char temp_disk_filename[260];
    snprintf(temp_disk_filename, sizeof(temp_disk_filename), "%s.tmp", disk_filename);

    int format = 0;

    FILE* original_disk = fopen(disk_filename, "rb");
    FILE* temp_disk = fopen(temp_disk_filename, "w+b");
    if (original_disk != NULL && temp_disk != NULL) {
        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), original_disk)) > 0) {
            fwrite(buffer, 1, bytes, temp_disk);
        }
        fclose(original_disk);
        fclose(temp_disk);
    } else if (original_disk == NULL && temp_disk != NULL) {
        fclose(temp_disk);
        format = 1;
        printf("Disco non trovato. Creazione di un nuovo disco...\n");
    } else {
        printf("Errore nella creazione del disco temporaneo.\n");
        return 1;
    }

    Disk* disk = disk_init(temp_disk_filename, format);
    if (disk == NULL) {
        printf("Errore nell'inizializzazione del disco.\n");
        return 1;
    }

    Directory* root_dir = (Directory*) malloc(sizeof(Directory));
    if (root_dir == NULL) {
        printf("Errore di allocazione della memoria per la directory root.\n");
        disk_close(disk);
        return 1;
    }

    if (format) {
        disk_format(disk); // Include l'inizializzazione della FAT e la marcatura del blocco 0

        // Inizializza la directory root
        memset(root_dir, 0, sizeof(Directory));
        strncpy(root_dir->fcb.name, "/", MAX_FILENAME_LENGTH - 1);
        root_dir->fcb.name[MAX_FILENAME_LENGTH - 1] = '\0';
        root_dir->fcb.start_block = fat_alloc_block(&disk->fat); // Assegna un blocco per root
        root_dir->fcb.size = 0;
        root_dir->fcb.is_directory = 1;
        root_dir->parent_block = -1;
        root_dir->num_entries = 0;

        // Scrive la directory root sul blocco assegnato
        char block_data[BLOCK_SIZE];
        memset(block_data, 0, BLOCK_SIZE);
        memcpy(block_data, root_dir, sizeof(Directory));
        disk_write(disk, root_dir->fcb.start_block, block_data);
        disk_save_fat(disk);

        if (DEBUG) {
            printf("Directory root creata e scritta nel blocco %d\n", root_dir->fcb.start_block);
        }
    } else {
        disk_read(disk, 0, (char*) root_dir); // Assumendo che il blocco 0 sia root
    }

    Directory* current_dir = root_dir;

    while (1) {
        char prompt_str[MAX_FILENAME_LENGTH + 4];
        snprintf(prompt_str, sizeof(prompt_str), "%s> ", current_dir->fcb.name);

        char* input = readline(prompt_str);

        if (input == NULL) {
            break;
        }

        if (strlen(input) > 0) {
            add_history(input);
        }

        execute_command(input, disk, &current_dir, disk_filename, temp_disk_filename);

        free(input);
    }

    free(root_dir);
    disk_close(disk);

    return 0;
}
