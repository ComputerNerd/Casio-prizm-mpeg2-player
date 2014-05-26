#include "filegui.h"
//#include "../lua_prizm.h"

int sort_folder(FBL_FileItem*a, FBL_FileItem *b)
{
	return (a->info.fsize == 0);
}

void itemholder_push(itemholder* i, FBL_FileItem* newitem) {
	i->size++;
	if (i->size > i->capacity) {
		int oldcap = i->capacity;
		i->capacity = 1+2*(i->capacity);
		FBL_FileItem** temp = malloc(sizeof(FBL_FileItem*)*(i->capacity));
		if (oldcap > 0) {
			memcpy(temp,i->items,oldcap*sizeof(FBL_FileItem*));
			free(i->items);
		}
		i->items = temp;
	}
	i->items[(i->size)-1] = newitem;
}

void itemholder_pop(itemholder* i) {
	i->size--;
	// Does not change memory allocated or capacity
}

/*
void FBL_Rename(char *path, char *name) {
	Bdisp_AllClr_VRAM();
	DefineStatusMessage("", 0, COLOR_BLACK, 0);
	PrintXY(1,1,"  Rename From", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);
	locate_OS(1,2);
	Print_OS((unsigned char*)name, 0, TEXT_COLOR_BLUE);
	PrintXY(1,3,"  Rename To", TEXT_MODE_NORMAL, TEXT_COLOR_BLUE);

	char buffer[100];
	int start = 0;
	int cursor = 0;

	DisplayMBString((unsigned char*)buffer, start, cursor, 1, 4);

	while(1) {
		int key;
		
		GetKey(&key);
		if(key == KEY_CTRL_EXIT)
			break;
		if(key == KEY_CTRL_EXE) {
			unsigned short *renbuf1 = (unsigned short*)malloc(sizeof(unsigned short)*(strlen(path)+strlen(name)+1));
			unsigned short *renbuf2 = (unsigned short*)malloc(sizeof(unsigned short)*(strlen(path)+strlen(buffer)+1));

			char* s = malloc(strlen(path)+strlen(name)+2);
			strcpy(s,path);
			strcat(s,name);			
			Bfile_StrToName_ncpy(renbuf1, (unsigned char*)s, strlen(s)+1);
			free(s);
			
			s = malloc(strlen(path)+strlen(buffer)+2);
			strcpy(s,path);
			strcat(s,buffer);
			Bfile_StrToName_ncpy(renbuf2, (unsigned char*)s, strlen(s)+1);
			free(s);
			
			Bfile_RenameEntry(renbuf1, renbuf2);

			free(renbuf1);
			free(renbuf2);
			break;
		}
		{
			EditMBStringCtrl((unsigned char*)buffer, 100, &start, &cursor, &key, 1, 4);
			//       DisplayMBString(buffer, start, cursor, 1, 4);
		}
		
	}
}
*/

// =================== Begin old FBL_Scroller class ============================
struct FBL_Scroller_Data* FBL_Scroller_cons(int ytop, int ybottom) {
	struct FBL_Scroller_Data* fblsd = malloc(sizeof(struct FBL_Scroller_Data));
	fblsd->start = 0;
	fblsd->sel = 0;
	fblsd->data_length = 0;
	fblsd->sb.I1 = 0;
	fblsd->sb.I5 = 0;
	fblsd->sb.indicatormaximum = 0;
	fblsd->sb.indicatorheight = ybottom - ytop + 1;
	fblsd->sb.indicatorpos = 0;
	fblsd->sb.barheight = LCD_HEIGHT_PX - 2*26;
	fblsd->sb.bartop = 0;
	fblsd->sb.barleft = LCD_WIDTH_PX - 7;
	fblsd->sb.barwidth = 7;
	fblsd->ytop = ytop;
	fblsd->ybottom = ybottom;
	return fblsd;
}

void FBL_Scroller_destr(struct FBL_Scroller_Data* fblsd) {
	free(fblsd);
}

void FBL_Scroller_render(struct FBL_Filelist_Data* fblfd) {
	struct FBL_Scroller_Data* fblsd = fblfd->fblsd;
	int size = fblsd->ybottom - fblsd->ytop + 1;
	//     Bdisp_AllClr_VRAM();
	//     SetBackGround(0x0D);
	int l = fblsd->data_length > size ? size : fblsd->data_length;
	char buffer[30];
	buffer[0] = ' ';
	buffer[1] = ' ';
	//DebugDebounceAndPause("srender 1\n");
	int i;
	for(i = 0; i < l; i++) {
		FBL_Filelist_data_render(fblfd, buffer+2, i + fblsd->start, i + fblsd->start == fblsd->sel, 8,24*(i+fblsd->ytop));
		PrintXY(3,fblsd->ytop+i,buffer,(i + fblsd->start == fblsd->sel) ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
	}
	//DebugDebounceAndPause("srender 2\n");
	fblsd->sb.indicatorpos = fblsd->start;
	Scrollbar(&(fblsd->sb));
	//DebugDebounceAndPause("srender 3\n");
}

void FBL_Scroller_key_up(struct FBL_Scroller_Data* fblsd) {
	if(fblsd->sel == 0)
		fblsd->sel = fblsd->data_length - 1;
	else
		fblsd->sel--;
	
	FBL_Scroller_bounds_check(fblsd);
}

void FBL_Scroller_key_down(struct FBL_Scroller_Data* fblsd) {
	fblsd->sel++;
	if(fblsd->sel >= fblsd->data_length)
	fblsd->sel = 0;
	
	FBL_Scroller_bounds_check(fblsd);
}
void FBL_Scroller_bounds_check(struct FBL_Scroller_Data* fblsd) {
	int size = fblsd->ybottom - fblsd->ytop;
	if(fblsd->sel < fblsd->start)
	fblsd->start = fblsd->sel;
	if(fblsd->sel - size > fblsd->start)
	fblsd->start = fblsd->sel - size;
}
// =================== End old FBL_Scroller class ============================

// =================== Begin old FBL_Filelist class ============================
struct FBL_Filelist_Data* FBL_Filelist_cons(const char* listpath,const char *filter,const char* title){
	//DebugDebounceAndPause("Constructing...");
	struct FBL_Filelist_Data* fblfd = malloc(sizeof(struct FBL_Filelist_Data));
	fblfd->fblsd = FBL_Scroller_cons(2,7);
	int currentpathlen=strlen(listpath)+1;
	fblfd->currentpath = malloc(currentpathlen);
	memcpy(fblfd->currentpath,listpath,currentpathlen);
	int titlelen=strlen(title)+1;
	fblfd->title = malloc(titlelen);
	memcpy(fblfd->title,title,titlelen);
	int filterlen=strlen(filter)+1;
	fblfd->filter = malloc(filterlen);
	memcpy(fblfd->filter,filter,filterlen);
	
	fblfd->ih.size = 0;
	fblfd->ih.capacity = 0;
	fblfd->ih.items = NULL;
	
	FBL_Filelist_chdir(fblfd);
	//DebugDebounceAndPause("Constructed.");
	return fblfd;
}

void FBL_Filelist_destr(struct FBL_Filelist_Data* fblfd) {
	FBL_Filelist_clear(fblfd);
	
	free(fblfd->currentpath);
	free(fblfd->title);
	free(fblfd->filter);
	
	free(fblfd);
}

void FBL_Filelist_chdir(struct FBL_Filelist_Data* fblfd) {
	FBL_Filelist_clear(fblfd);
	
	FBL_FileItem *item;
	unsigned short path[0x10A], found[0x10A];
	unsigned char buffer[0x10A];

	// make the buffer
	strcpy((char*)buffer, fblfd->currentpath);
	strcat((char*)buffer, "*");
	//     strcat((char*)buffer, filter);
	
	item = malloc(sizeof(FBL_FileItem));
	
	Bfile_StrToName_ncpy(path, buffer, 0x10A);
	int ret = Bfile_FindFirst_NON_SMEM(path, &(fblfd->find_handle), found, &item->info);
	Bfile_StrToName_ncpy(path, (unsigned char*)(fblfd->filter), 0x10A);
	while(!ret) {
		Bfile_NameToStr_ncpy(buffer, found, 0x10A);
		if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0) &&
		   (item->info.fsize == 0 || Bfile_Name_MatchMask(path, found)))
		{
			item->name = malloc(sizeof(char)*(strlen((char*)buffer) + 1));
			strcpy(item->name, (char*)buffer);

//#error remember, strcpy is probably broken. Currently editing here

			//         if(item.info.id == 0)
			FBL_Filelist_bake(fblfd,item);
			
			itemholder_push(&(fblfd->ih),item);
			item = malloc(sizeof(FBL_FileItem));
		}
		ret = Bfile_FindNext_NON_SMEM((fblfd->find_handle), found, &item->info);
	}

	Bfile_FindClose(fblfd->find_handle);
	fblfd->fblsd->data_length = fblfd->ih.size;
	fblfd->fblsd->sb.indicatormaximum = fblfd->fblsd->data_length;
	fblfd->menu_id = 0;
	fblfd->result = 0;
	FBL_Filelist_setupStatusBar(fblfd);
	fblfd->fblsd->start = 0;
	fblfd->fblsd->sel = 0;
	
	SetBackGround(0x0D);
	SaveVRAM_1();
	// FIXME TODO XXX Sort
	//sort(items.begin(), items.end(), sort_folder);
}

void FBL_Filelist_clear(struct FBL_Filelist_Data* fblfd) {
	while(fblfd->ih.size > 0) {
		free(fblfd->ih.items[(fblfd->ih.size)-1]->name);
		free(fblfd->ih.items[(fblfd->ih.size)-1]);
		itemholder_pop(&(fblfd->ih));
	}
}

void FBL_Filelist_bake(struct FBL_Filelist_Data* fblfd, FBL_FileItem *item) {
	int type = 0;
	char buffer2[10];
	int size = item->info.fsize;
	
	memset(item->buffer, ' ', 20);
	memcpy(item->buffer, item->name, strlen(item->name) > 20 ? 20 : strlen(item->name));
	
	if(size) {
		while(size > 1024) {
			size /= 1024;
			type++;
		}
		
		switch(type)
		{
		case 1:
			item->buffer[18] = 'K';
			break;
		case 2:
			item->buffer[18] = 'M';
			break;
		}
		
		buffer2[0] = ':';
		itoa(size, (unsigned char*)buffer2+1);
		memcpy(item->buffer + (!type ? 19 : 18)-strlen(buffer2), buffer2, strlen(buffer2));
		item->buffer[19] = 0;
	}
}

void FBL_Filelist_data_render(struct FBL_Filelist_Data* fblfd, char* buffer,
                              int index, int selected, int pix_x, int pix_y)
{
	FBL_FileItem *item = fblfd->ih.items[index];
	//nio_printf(&globalconsole,"Item %d of %d at addr %p\n",index,fblfd->ih.size,fblfd->ih.items[index]);
	//DebugDebounceAndPause("FBL_FL_D_R\n");
	strcpy(buffer, item->buffer);
	//DebugDebounceAndPause("FBL_FL_D_R 2\n");
	if(item->info.fsize == 0) {
		//DebugDebounceAndPause("FBL_FL_D_R 2A\n");
		VRAM_CopySprite(folder, pix_x, pix_y, 22, 22);
	}
	//DebugDebounceAndPause("FBL_FL_D_R 3\n");
}

void FBL_Filelist_render(struct FBL_Filelist_Data* fblfd) {
	int x = 0, y = 0;
	//DebugDebounceAndPause("render 1\n");
	LoadVRAM_1();
	//DebugDebounceAndPause("render 2\n");
	FBL_Scroller_render(fblfd);
	//DebugDebounceAndPause("render 3\n");
	PrintMini(&x, &y, (unsigned char*)(fblfd->currentpath)+6, 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
	void* bitmap;
	
	switch(fblfd->menu_id) {
		case 0:
			GetFKeyPtr(0x3B1, &bitmap);
			FKey_Display(0, bitmap);
			GetFKeyPtr(0x2B1, &bitmap);
			FKey_Display(1, bitmap);
			GetFKeyPtr(0x000, &bitmap);
			FKey_Display(2, bitmap);
			GetFKeyPtr(0x000, &bitmap);
			FKey_Display(3, bitmap);
			GetFKeyPtr(0x000, &bitmap);
			FKey_Display(4, bitmap);
			GetFKeyPtr(0x000, &bitmap);
			FKey_Display(5, bitmap);
			break;
/*
		case 1:
			GetFKeyPtr(0x38E, &bitmap);
			FKey_Display(0, bitmap);
			GetFKeyPtr(0x3B0, &bitmap);
			FKey_Display(1, bitmap);
			break;
*/
	}
	if(fblfd->ih.size == 0) {
		PrintXY(1,3,"  No files", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_RED);
	}
	
	DisplayStatusArea();
}

void FBL_Filelist_key_enter(struct FBL_Filelist_Data* fblfd) {
	if(fblfd->ih.size == 0)
	return;
	
	if(fblfd->ih.items[fblfd->fblsd->sel]->info.fsize == 0) {
		char* tmp = malloc(strlen(fblfd->currentpath)+strlen(fblfd->ih.items[fblfd->fblsd->sel]->name)+2);
		strcpy(tmp,fblfd->currentpath);
		strcat(tmp,fblfd->ih.items[fblfd->fblsd->sel]->name);
		strcat(tmp,"\\");
		free(fblfd->currentpath);
		fblfd->currentpath = tmp;
		FBL_Filelist_chdir(fblfd);
	}
	else
	{
		fblfd->result = 1;
	}
}

void FBL_Filelist_key_menu(struct FBL_Filelist_Data* fblfd, int x) {
/*
	switch(fblfd->menu_id) {
		case 0:
*/
			switch(x)
			{
			case 0:
				FBL_Filelist_key_enter(fblfd);
				break;
			case 1:
				FBL_Filelist_key_exit(fblfd);
				/*
				if(fblfd->ih.size) {
					FBL_Rename(fblfd->currentpath, fblfd->ih.items[fblfd->fblsd->sel]->name);
					FBL_Filelist_chdir(fblfd);
				}
				*/
				break;
			case 2:

				break;
			case 3:
				//fblfd->menu_id = 1;
				break;
			case 4:

				break;
			case 5:

				break;
			}
/*
			break;
		case 1:
			switch(x)
			{
			case 0:
				if(fblfd->ih.items[fblfd->fblsd->sel]->info.fsize == 0) {
					FBL_Rename(fblfd->currentpath, fblfd->ih.items[fblfd->fblsd->sel]->name);
					FBL_Filelist_chdir(fblfd);
				}
				break;
			case 1:
				if(fblfd->ih.items[fblfd->fblsd->sel]->info.fsize == 0) {
					
				}
				break;
			}
			break;
	}
*/
}

void FBL_Filelist_key_exit(struct FBL_Filelist_Data* fblfd) {
	if(fblfd->menu_id == 1) {
		fblfd->menu_id = 0;
		return;
	}
	if(!strcmp(fblfd->currentpath,"\\\\fls0\\")) {
		fblfd->result = -1;
		return;
	}
	
	int i=strlen(fblfd->currentpath)-2;
	while (i>=0 && fblfd->currentpath[i] != '\\')
		i--;
	if (fblfd->currentpath[i] == '\\') {
		char* tmp = malloc(sizeof(char)*(i+2));
		memcpy(tmp,fblfd->currentpath,i+1);
		tmp[i+1] = '\0';
		free(fblfd->currentpath);
		fblfd->currentpath = tmp;
	}
	
	FBL_Filelist_chdir(fblfd);
}

int FBL_Filelist_isDone(struct FBL_Filelist_Data* fblfd) {
	return (fblfd->result != 0);
}

FILE *FBL_Filelist_getFile(struct FBL_Filelist_Data* fblfd, char *mode) {
	char* path = fblfd->currentpath;
	char* name = fblfd->ih.items[fblfd->fblsd->sel]->name;
	char* tempbuf = malloc(strlen(path)+strlen(name)+1);
	strcpy(tempbuf,path);
	strcat(tempbuf,name);
	
	FILE* fh = fopen(tempbuf, mode);
	free(tempbuf);
	return fh;
}

char* FBL_Filelist_getFilename(struct FBL_Filelist_Data* fblfd, char* str, int maxlen) {
	strncpy(str, fblfd->currentpath, maxlen);
	strncat(str, fblfd->ih.items[fblfd->fblsd->sel]->name, maxlen-strlen(fblfd->currentpath));
	return str;
}

void FBL_Filelist_go(struct FBL_Filelist_Data* fblfd) {
	//DebugDebounceAndPause("go 1.");
	FBL_Filelist_initBackground(fblfd);
	//DebugDebounceAndPause("go 2.");
	
	int key;
	while(!FBL_Filelist_isDone(fblfd)) {
		FBL_Filelist_render(fblfd);
	//DebugDebounceAndPause("go 3.");
		GetKey(&key);
		switch(key) {
			case KEY_CTRL_UP:
				FBL_Scroller_key_up(fblfd->fblsd);
				break;
			case KEY_CTRL_DOWN:
				FBL_Scroller_key_down(fblfd->fblsd);
				break;
			case KEY_CTRL_EXE:
				FBL_Filelist_key_enter(fblfd);
				break;
			case KEY_CTRL_F1:
			case KEY_CTRL_F2:
			case KEY_CTRL_F3:
			case KEY_CTRL_F4:
			case KEY_CTRL_F5:
			case KEY_CTRL_F6:
				FBL_Filelist_key_menu(fblfd, key - KEY_CTRL_F1);
				break;
			case KEY_CTRL_EXIT:
				FBL_Filelist_key_exit(fblfd);
				break;
		}
	}
}

void FBL_Filelist_initBackground(struct FBL_Filelist_Data* fblfd) {
	Bdisp_AllClr_VRAM();
	SetBackGround(0xD);
	SaveVRAM_1();
}

void FBL_Filelist_setupStatusBar(struct FBL_Filelist_Data* fblfd) {
	EnableStatusArea(0);
	DefineStatusAreaFlags(DSA_SETDEFAULT, 0, 0, 0);
	DefineStatusAreaFlags(3, SAF_BATTERY | SAF_TEXT | SAF_GLYPH | SAF_ALPHA_SHIFT, 0, 0);
	DefineStatusMessage(fblfd->title, 0, TEXT_COLOR_BLACK, 0);
}

/*
int main()
{

	TScrollbar sb;
	sb.I1 = 0;
	sb.I5 = 0;
	sb.indicatormaximum = 10;
	sb.indicatorheight = 9;
	sb.indicatorpos = 0;
	sb.barheight = 100;
	sb.bartop = 0;
	sb.barleft = 0;
	sb.barwidth = 15;

	Bdisp_EnableColor(1);
	Bdisp_AllClr_VRAM();

	int key;

	struct FBL_Filelist_Data *list = FBL_Filelist_cons("\\\\fls0\\", "*.g3a", "Open file (*.g3a)");

	FBL_Filelist_go(list);

	Bdisp_AllClr_VRAM();
	if(list->result == 1) {
		char buf[21];
		locate_OS(1, 1);
		Print_OS((unsigned char*)FBL_Filelist_getFilename(list,buf,20), 0, 0);
	}
	FBL_Filelist_destr(list);

	while(1) GetKey(&key);
}
*/

/*
* 0x38 - Delete
* 0xA5 - Search (white)
* 0x186 - New
* 0x187 - Search
* 0x188 - Rename
* 0x18D - Symbol
* 0x18C - Menu
* 0x196 - (key)
* 0x1D1 - (next menu)
* 0x2A9 - SD
* 0x2A1 - Char
* 0x2B1 - Back
* 0x316 - Size
* 0x38D - Copy
* 0x38E - Mkefldr
* 0x3B0 - Renfldr
* 0x3B1 - Open
* 0x3B2 - Save-as
* 0x3E8 - Strgmem
* 0x3EF - Card
* 0x3F5 - Memory (white)
* 0x490 - Save (white)
* 0x491 - Folder
* 0x49B - V-Mem
* 0x4D3 - (key, black submenu)
*/
