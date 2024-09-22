#define DEBUG 0

#define DISK_SIZE (1024 * 1024)// 1MB
#define BLOCK_SIZE 512          // 512B
#define FAT_SIZE (DISK_SIZE / BLOCK_SIZE)

#define MAX_FILENAME_LENGTH 100

#define MAX_DIR_ENTRIES 3

#define MAX_ENTRIES_PER_BLOCK ((BLOCK_SIZE - sizeof(int)) / sizeof(FileControlBlock))