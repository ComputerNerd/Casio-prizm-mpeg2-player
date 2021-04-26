#ifndef FILEGUI_H
#define FILEGUI_H

#include <fxcg/misc.h>
#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/file.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
//#include <vector>
//#include <string>
//#include <algorithm>
//#include "icons.h"

//#define true 1
//#define false 0

typedef struct
{
	// This struct must be packed.
	uint16_t id, type;
	uint32_t fsize, dsize;
	uint32_t property;
	void* address;
} file_type_t;

typedef struct
{
	char *name;
	file_type_t info;
	char buffer[24];
} FBL_FileItem;

struct FBL_Scroller_Data {
	int start;
	int sel;
	struct scrollbar sb;
	char *dlgTitle;
	int ytop, ybottom;
	// data
	int data_length;
};

typedef struct {
	int size;
	int capacity;
	FBL_FileItem** items;
} itemholder;

struct FBL_Filelist_Data {
	struct FBL_Scroller_Data* fblsd;
	int result;
	int menu_id;
	int find_handle;
	char* filter;
	char* currentpath;
	char* title;
	itemholder ih;
};
	

struct FBL_Filelist_Data* FBL_Filelist_cons(const char* listpath,const char *filter,const char* title);
char* FBL_Filelist_getFilename(struct FBL_Filelist_Data* fblfd, char* str, int maxlen);
void FBL_Filelist_go(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_destr(struct FBL_Filelist_Data* fblfd);

#endif //define FILEGUI_H
