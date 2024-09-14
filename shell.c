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
            for (int i = 0; i < (*current_dir)->num_entries; i++) {
                if (strcmp((*current_dir)->entries[i].name, filename) == 0) {
                    printf("Il file '%s' esiste già.\n", filename);
                    return;
                }
            }
            FileHandle* file = create_file(disk, filename);
            if (file != NULL) {
                (*current_dir)->entries[(*current_dir)->num_entries].start_block = file->start_block;
                (*current_dir)->entries[(*current_dir)->num_entries].is_directory = 0;
                strncpy((*current_dir)->entries[(*current_dir)->num_entries].name, filename, MAX_DIR_NAME - 1);
                (*current_dir)->num_entries++;
                disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
                disk_save_fat(disk); // Salva la FAT
                free(file);
            }
        }
    } else if (strcmp(args[0], "mkdir") == 0) {
        if (arg_count < 2) {
            printf("Usage: mkdir dirname\n");
        } else {
            create_dir(disk, *current_dir, args[1]);
            disk_save_fat(disk); // Salva la FAT
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            printf("Usage: cd dirname\n");
        } else if (strcmp(args[1], "..") == 0) {
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
            int found = 0;
            for (int i = 0; i < (*current_dir)->num_entries; i++) {
                if (!(*current_dir)->entries[i].is_directory && strcmp((*current_dir)->entries[i].name, args[1]) == 0) {
                    FileHandle handle;
                    handle.start_block = (*current_dir)->entries[i].start_block;
                    erase_file(disk, &handle, args[1]);
                    for (int j = i; j < (*current_dir)->num_entries - 1; j++) {
                        (*current_dir)->entries[j] = (*current_dir)->entries[j + 1];
                    }
                    (*current_dir)->num_entries--;
                    disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
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
                    for (int j = i; j < (*current_dir)->num_entries - 1; j++) {
                        (*current_dir)->entries[j] = (*current_dir)->entries[j + 1];
                    }
                    (*current_dir)->num_entries--;
                    disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
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
                    (*current_dir)->entries[(*current_dir)->num_entries].start_block = file->start_block;
                    (*current_dir)->entries[(*current_dir)->num_entries].is_directory = 0;
                    strncpy((*current_dir)->entries[(*current_dir)->num_entries].name, filename, MAX_DIR_NAME - 1);
                    (*current_dir)->num_entries++;
                    disk_write(disk, (*current_dir)->start_block, (const char*)*current_dir);
                    disk_save_fat(disk); // Salva la FAT
                } else {
                    printf("Operazione annullata.\n");
                    return;
                }
            } else {
                file->current_block = file->start_block;
                file->position = sizeof(FileMetadata);
                file->size = 0;
            }

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

                if (text != NULL && total_size > 0) {
                    write_file(file, disk, text, total_size);
                    free(text);
                }
            }

            free(file);
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

            if (file->size == 0) {
                printf("Il file è vuoto.\n");
                free(file);
                return;
            }

            file->current_block = file->start_block;
            file->position = sizeof(FileMetadata);

            char* buffer = (char*) malloc(file->size + 1);
            memset(buffer, 0, file->size + 1);
            read_file(file, disk, buffer, file->size);

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
    } else {
        printf("Comando non riconosciuto: %s\n", args[0]);
    }
}

int main() {
    char disk_filename[256];

    printf("Inserisci il nome o il percorso del disco da aprire: ");
    fgets(disk_filename, sizeof(disk_filename), stdin);
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

    Directory* root_dir = (Directory*) malloc(sizeof(Directory));

    if (format) {
        disk->fat.entries[0].file = 1;
        disk->fat.entries[0].next_block = -1;
        disk->fat.free_blocks--;

        memset(root_dir, 0, sizeof(Directory));
        strncpy(root_dir->name, "/", MAX_DIR_NAME - 1);
        root_dir->name[MAX_DIR_NAME - 1] = '\0';
        root_dir->start_block = 0;
        root_dir->parent_block = -1;
        root_dir->num_entries = 0;

        disk_write(disk, 0, (const char*) root_dir);
        disk_save_fat(disk);

        if (DEBUG) {
            printf("Directory root creata e scritta nel blocco 0\n");
        }
    } else {
        disk_read(disk, 0, (char*) root_dir);
    }

    Directory* current_dir = root_dir;

    while (1) {
        char prompt_str[MAX_DIR_NAME + 4];
        snprintf(prompt_str, sizeof(prompt_str), "%s> ", current_dir->name);

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
