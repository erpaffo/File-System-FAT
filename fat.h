#pragma once

#include "disk.h"

void fat_init(Fat* fat);
int fat_get_free_block(Fat* fat);
int fat_alloc_block(Fat* fat);
void fat_free_block(Fat* fat, int block);
int fat_get_next_block(Fat* fat, int block);
void fat_set_next_block(Fat* fat, int block, int next_block);
