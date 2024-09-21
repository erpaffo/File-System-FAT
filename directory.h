#pragma once

#include "disk.h"
#include "fat.h"
#include "file.h"

Directory* create_dir(Disk* disk, Directory* parent, const char* dirname);
void erase_dir(Disk* disk, Directory* dir);
void change_dir(Disk* disk, Directory** current_dir, const char* dirname);
void list_dir(Disk* disk, Directory* dir);
