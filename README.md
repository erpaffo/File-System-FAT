This is a project for the course "Sistemi Operativi". This is the trace:

```text
L2. File System FAT .
 Implement a file system that uses a pseudo "FAT" on an mmapped buffer.
   The functions to implement are

   createFile
   eraseFile
   write (potentially extending the file boundaries)
   read
   seek
   createDir
   eraseDir
   changeDir
   listDir

   The opening of a file should return a "FileHandle" that stores the position in a file.
```

To execute the code use these commands:
```bash
gcc -o test test.c disk.c fat.c file.c
./test
```