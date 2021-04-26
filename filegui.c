#include <stdint.h>
#include "filegui.h"

#if 0
#define DebugDebounceAndPause(title)
#else
static void DebugDebounceAndPause(const char* title) {
	int x, y, key;
	x = 18;
	y = 36;
	PrintMini(&x, &y, title, 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
	PrintMini(&x, &y, "  ", 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
	GetKey(&key);
}
#endif

static int sort_folder(FBL_FileItem*a, FBL_FileItem *b)
{
	return (a->info.fsize == 0);
}

static void itemholder_push(itemholder* i, FBL_FileItem* newitem) {
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

static void itemholder_pop(itemholder* i) {
	i->size--;
	// Does not change memory allocated or capacity
}

// =================== Begin old FBL_Scroller class ============================
static struct FBL_Scroller_Data* FBL_Scroller_cons(int ytop, int ybottom) {
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

static void FBL_Scroller_destr(struct FBL_Scroller_Data* fblsd) {
	free(fblsd);
}

static void FBL_Filelist_data_render(struct FBL_Filelist_Data* fblfd, char* buffer,
                              int index, int selected, unsigned int pix_x, unsigned int pix_y);
static void FBL_Scroller_render(struct FBL_Filelist_Data* fblfd) {
	struct FBL_Scroller_Data* fblsd = fblfd->fblsd;
	int size = fblsd->ybottom - fblsd->ytop + 1;
	//     Bdisp_AllClr_VRAM();
	//     SetBackGround(0x0D);
	int l = fblsd->data_length > size ? size : fblsd->data_length;
	char buffer[30];
	buffer[0] = ' ';
	buffer[1] = ' ';
	DebugDebounceAndPause("srender 1\n");
	int i;
	for(i = 0; i < l; i++) {
		FBL_Filelist_data_render(fblfd, buffer+2, i + fblsd->start, i + fblsd->start == fblsd->sel, 8,24*(i+fblsd->ytop));
		PrintXY(3,fblsd->ytop+i,buffer,(i + fblsd->start == fblsd->sel) ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
	}
	DebugDebounceAndPause("srender 2\n");
	fblsd->sb.indicatorpos = fblsd->start;
	Scrollbar(&(fblsd->sb));
	DebugDebounceAndPause("srender 3\n");
}

static void FBL_Scroller_bounds_check(struct FBL_Scroller_Data* fblsd);
static void FBL_Scroller_key_up(struct FBL_Scroller_Data* fblsd) {
	if(fblsd->sel == 0)
		fblsd->sel = fblsd->data_length - 1;
	else
		fblsd->sel--;
	
	FBL_Scroller_bounds_check(fblsd);
}

static void FBL_Scroller_key_down(struct FBL_Scroller_Data* fblsd) {
	fblsd->sel++;
	if(fblsd->sel >= fblsd->data_length)
	fblsd->sel = 0;
	
	FBL_Scroller_bounds_check(fblsd);
}
static void FBL_Scroller_bounds_check(struct FBL_Scroller_Data* fblsd) {
	int size = fblsd->ybottom - fblsd->ytop;
	if(fblsd->sel < fblsd->start)
	fblsd->start = fblsd->sel;
	if(fblsd->sel - size > fblsd->start)
	fblsd->start = fblsd->sel - size;
}
// =================== End old FBL_Scroller class ============================

// =================== Begin old FBL_Filelist class ============================
extern uint16_t*VRAM_ADDR;
static void FBL_Filelist_chdir(struct FBL_Filelist_Data* fblfd);
struct FBL_Filelist_Data* FBL_Filelist_cons(const char* listpath,const char *filter,const char* title){ // Must not be static
	char buf[256];
	DebugDebounceAndPause("Constructing...");
	sprintf(buf, "VADDR: %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	struct FBL_Filelist_Data* fblfd = malloc(sizeof(struct FBL_Filelist_Data));
	fblfd->fblsd = FBL_Scroller_cons(2,7);
	sprintf(buf, "VADDR1 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);

	int currentpathlen = strlen(listpath)+1;
	fblfd->currentpath = malloc(currentpathlen);
	memcpy(fblfd->currentpath,listpath,currentpathlen);
	sprintf(buf, "VADDR2 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);

	int titlelen = strlen(title)+1;
	fblfd->title = malloc(titlelen);
	memcpy(fblfd->title,title,titlelen);
	sprintf(buf, "VADDR3 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);

	int filterlen = strlen(filter)+1;
	fblfd->filter = malloc(filterlen);
	memcpy(fblfd->filter,filter,filterlen);
	sprintf(buf, "VADDR4 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	
	fblfd->ih.size = 0;
	fblfd->ih.capacity = 0;
	fblfd->ih.items = NULL;
	sprintf(buf, "VADDR5 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	
	FBL_Filelist_chdir(fblfd);
	DebugDebounceAndPause("Constructed.");
	sprintf(buf, "VADDR6 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	return fblfd;
}

static void FBL_Filelist_clear(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_destr(struct FBL_Filelist_Data* fblfd) { // Must not be static
	FBL_Filelist_clear(fblfd);
	
	free(fblfd->currentpath);
	free(fblfd->title);
	free(fblfd->filter);
	
	free(fblfd);
}

static void FBL_Filelist_setupStatusBar(struct FBL_Filelist_Data* fblfd);
static void FBL_Filelist_bake(struct FBL_Filelist_Data* fblfd, FBL_FileItem *item);
static void FBL_Filelist_chdir(struct FBL_Filelist_Data* fblfd) {
	char buf[256];
	FBL_Filelist_clear(fblfd);
	sprintf(buf, "VADR0 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	
	FBL_FileItem *item;
	unsigned short path[271], found[271];
	unsigned char buffer[271];

	// make the buffer
	strcpy((char*)buffer, fblfd->currentpath);
	strcat((char*)buffer, "*");
	sprintf(buf, "VADR1 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	DebugDebounceAndPause(buffer);
	//     strcat((char*)buffer, filter);
	
	item = malloc(sizeof(FBL_FileItem));
	
	Bfile_StrToName_ncpy(path, buffer, 270);
	sprintf(buf, "VADR2 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	sprintf(buf, "sizeof %d", sizeof(file_type_t));
	DebugDebounceAndPause(buf);
	int ret = Bfile_FindFirst(path, &(fblfd->find_handle), found, &item->info);

	sprintf(buf, "VADR3 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);

	sprintf(buf, "id %d", item->info.id);
	DebugDebounceAndPause(buf);

	sprintf(buf, "type %d", item->info.type);
	DebugDebounceAndPause(buf);

	sprintf(buf, "fsize %d", item->info.fsize);
	DebugDebounceAndPause(buf);

	sprintf(buf, "dsize %d", item->info.dsize);
	DebugDebounceAndPause(buf);

	sprintf(buf, "property %d", item->info.property);
	DebugDebounceAndPause(buf);

	sprintf(buf, "address %p", item->info.address);
	DebugDebounceAndPause(buf);
	


	Bfile_StrToName_ncpy(path, (unsigned char*)(fblfd->filter), 270);
	sprintf(buf, "VADR4 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	while(!ret) {
		Bfile_NameToStr_ncpy(buffer, found, 270);
		sprintf(buf, "VADR5 %p", VRAM_ADDR);
		DebugDebounceAndPause(buf);
		if(!(strcmp((char*)buffer, "..") == 0 || strcmp((char*)buffer, ".") == 0) &&
		   (item->info.fsize == 0 || Bfile_Name_MatchMask(path, found)))
		{
			sprintf(buf, "VADR6 %p", VRAM_ADDR);
			DebugDebounceAndPause(buf);
			item->name = malloc(sizeof(char)*(strlen((char*)buffer) + 1));
			sprintf(buf, "VADR7 %p", VRAM_ADDR);
			DebugDebounceAndPause(buf);
			strcpy(item->name, (char*)buffer);
			sprintf(buf, "VADR8 %p", VRAM_ADDR);
			DebugDebounceAndPause(buf);

//#error remember, strcpy is probably broken. Currently editing here

			//         if(item.info.id == 0)
			FBL_Filelist_bake(fblfd,item);
			sprintf(buf, "VADR9 %p", VRAM_ADDR);
			DebugDebounceAndPause(buf);
			
			itemholder_push(&(fblfd->ih),item);
			sprintf(buf, "VADR10 %p", VRAM_ADDR);
			DebugDebounceAndPause(buf);
			item = malloc(sizeof(FBL_FileItem));
			sprintf(buf, "VADR11 %p", VRAM_ADDR);
			DebugDebounceAndPause(buf);
		}
		ret = Bfile_FindNext((fblfd->find_handle), found, &item->info);
		sprintf(buf, "VADR12 %p", VRAM_ADDR);
		DebugDebounceAndPause(buf);
	}
	sprintf(buf, "VADR13 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);

	Bfile_FindClose(fblfd->find_handle);
	sprintf(buf, "VADR14 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	fblfd->fblsd->data_length = fblfd->ih.size;
	fblfd->fblsd->sb.indicatormaximum = fblfd->fblsd->data_length;
	fblfd->menu_id = 0;
	fblfd->result = 0;
	FBL_Filelist_setupStatusBar(fblfd);
	fblfd->fblsd->start = 0;
	fblfd->fblsd->sel = 0;
	sprintf(buf, "VADR15 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	
	SetBackGround(0x0D);
	SaveVRAM_1();
	sprintf(buf, "VADR16 %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	// FIXME TODO XXX Sort
	//sort(items.begin(), items.end(), sort_folder);
}

static void FBL_Filelist_clear(struct FBL_Filelist_Data* fblfd) {
	while(fblfd->ih.size > 0) {
		free(fblfd->ih.items[(fblfd->ih.size)-1]->name);
		free(fblfd->ih.items[(fblfd->ih.size)-1]);
		itemholder_pop(&(fblfd->ih));
	}
}

static void FBL_Filelist_bake(struct FBL_Filelist_Data* fblfd, FBL_FileItem *item) {
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
static const color_t folder[] __attribute__((aligned(4))) = {
	0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
	0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
	0x7cf8,0x4bd6,0x53d6,0x53d6,0x53d6,0x53d6,0x4bd6,0x5c37,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
	0x4b94,0x8dbd,0x8d9c,0x8d9c,0x8dbc,0x8dbd,0x8dbd,0x4bd6,0x53f6,0x53f6,0x4bf6,0x53f6,0x53f6,0x53f6,0x53f6,0x53f6,0x53f6,0x4bd6,0x53f6,0x4bd6,0x6477,0xffff,
	0x3b32,0x857c,0x5c9a,0x5c9a,0x5c9a,0x649a,0x7d3b,0x8d7c,0x857b,0x857b,0x857b,0x857b,0x855b,0x855b,0x855b,0x7d5b,0x7d3b,0x7d3b,0x7d3b,0x7d3b,0x53f6,0xf7df,
	0x32d0,0x7d1a,0x5458,0x5c59,0x5c59,0x5459,0x5c59,0x5458,0x5438,0x5438,0x5438,0x5438,0x5438,0x5c38,0x5438,0x5438,0x5438,0x5438,0x5438,0x6cb9,0x4395,0xf7bf,
	0x2a6e,0x6cd9,0x5417,0x5417,0x5417,0x5417,0x5417,0x4bd6,0x3b33,0x32d2,0x2ad1,0x32f1,0x32d1,0x32d1,0x32d1,0x32d1,0x32d1,0x32d1,0x32d1,0x4373,0x2ab0,0xffdf,
	0x2a6e,0x4bd5,0x3313,0x3313,0x3313,0x3313,0x3313,0x2ab1,0x95ba,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xbedd,0xb6be,0x9dfa,
	0x5c35,0xbede,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb6bd,0xb69d,0x9dfc,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d9b,0x8d9b,0x63d2,
	0x53f5,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d9b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x959b,0x5bd2,
	0x53f5,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x8d7b,0x5bb2,
	0x53d5,0x8d7b,0x855b,0x8d7b,0x857b,0x8d7b,0x857b,0x8d7b,0x855b,0x857b,0x8d7b,0x855b,0x8d7b,0x8d7b,0x857b,0x8d7b,0x855b,0x857b,0x8d7b,0x8d5b,0x8d7b,0x5bb2,
	0x53d5,0x8d7b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x8d5b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x8d5b,0x855b,0x855b,0x855b,0x8d7b,0x5bb2,
	0x53d5,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x5bb2,
	0x53d5,0x855b,0x853b,0x853b,0x855b,0x853b,0x853b,0x853b,0x853b,0x853b,0x853b,0x853b,0x855b,0x853b,0x853b,0x853b,0x853b,0x853b,0x853b,0x853b,0x855b,0x5b92,
	0x53d5,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x855b,0x5bb2,
	0x2af2,0x3355,0x3354,0x3354,0x2b54,0x3354,0x3354,0x3354,0x3354,0x3354,0x3354,0x3354,0x2b54,0x3354,0x3354,0x3354,0x2b54,0x3354,0x3354,0x3354,0x3355,0x2270,
	0x3b53,0x43d6,0x43d6,0x43d6,0x43d6,0x43d6,0x43d6,0x4bd6,0x43d6,0x43d6,0x43d6,0x4bd6,0x4bd6,0x43d6,0x43d6,0x43d6,0x4bd6,0x43d6,0x4bd6,0x43d6,0x4bf6,0x2a8f,
	0x5c36,0x8d7b,0x855b,0x8d5b,0x855b,0x8d5b,0x8d5b,0x855b,0x855b,0x855b,0x8d7b,0x855b,0x855b,0x8d5b,0x855b,0x855b,0x855b,0x855b,0x8d5b,0x855b,0x857b,0x63f3,
	0x53f5,0x74b9,0x6cb9,0x74d9,0x6cb9,0x74b9,0x74d9,0x74d9,0x74d9,0x6cb9,0x74d9,0x74d9,0x74d9,0x74b9,0x74d9,0x74d9,0x74d9,0x6cb9,0x6cd9,0x74d9,0x74d9,0x5b92,
	0xdedb,0x7c53,0x7c53,0x7c32,0x7432,0x7432,0x7412,0x7411,0x73f1,0x73f1,0x6bf1,0x6bf1,0x6bf1,0x73f1,0x7411,0x7412,0x7432,0x7432,0x7c52,0x7c52,0x7c53,0xdefb,
	0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff
};
static void cpySpriteNoclipmul2(const unsigned* __restrict__ sprite,unsigned x,unsigned y,unsigned wdiv2,unsigned h){
	unsigned * __restrict__ vram = (unsigned*)VRAM_ADDR;
	DebugDebounceAndPause("vram");
	x /= 2;
	DebugDebounceAndPause("x");
	vram += (y * (384 / 2))+x;
	DebugDebounceAndPause("vram offset");
	do{
		unsigned w=wdiv2;
		DebugDebounceAndPause("w");
		do{
			unsigned val = *sprite++;
			DebugDebounceAndPause("val");
			*vram++ = val;
			DebugDebounceAndPause("pixel");
		}while(--w);
		DebugDebounceAndPause("xloop");
		vram+=(384/2)-wdiv2;
		DebugDebounceAndPause("yoffset");
	}while(--h);//Do while may be faster as it most resembles loop instruction some CPUs
	DebugDebounceAndPause("done");
}
static void FBL_Filelist_data_render(struct FBL_Filelist_Data* fblfd, char* buffer,
                              int index, int selected, unsigned int pix_x, unsigned int pix_y)
{
	FBL_FileItem *item = fblfd->ih.items[index];
	//nio_printf(&globalconsole,"Item %d of %d at addr %p\n",index,fblfd->ih.size,fblfd->ih.items[index]);
	DebugDebounceAndPause("FBL_FL_D_R");
	strcpy(buffer, item->buffer);
	DebugDebounceAndPause("FBL_FL_D_R 2");
	if(item->info.fsize == 0) {
		DebugDebounceAndPause("FBL_FL_D_R 2A");
		if ((pix_x < 362) && (pix_y < 194)) {
			//cpySpriteNoclipmul2((const unsigned*)folder, pix_x, pix_y, 11, 22);
		}
	}
	DebugDebounceAndPause("FBL_FL_D_R 3");
}

static void FBL_Filelist_render(struct FBL_Filelist_Data* fblfd) {
	char buf[256];
	int x = 0, y = 0;
	DebugDebounceAndPause("render 1\n");
	sprintf(buf, "VADDR: %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	LoadVRAM_1();
	DebugDebounceAndPause("render 2\n");
	sprintf(buf, "VADDR: %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	FBL_Scroller_render(fblfd);
	DebugDebounceAndPause("render 3\n");
	sprintf(buf, "VADDR: %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	PrintMini(&x, &y, (fblfd->currentpath)+6, 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_WHITE, 1, 0);
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

static void FBL_Filelist_key_enter(struct FBL_Filelist_Data* fblfd) {
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

static void FBL_Filelist_key_exit(struct FBL_Filelist_Data* fblfd);

static void FBL_Filelist_key_menu(struct FBL_Filelist_Data* fblfd, int x) {
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

static void FBL_Filelist_key_exit(struct FBL_Filelist_Data* fblfd) {
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

static int FBL_Filelist_isDone(struct FBL_Filelist_Data* fblfd) {
	return (fblfd->result != 0);
}

static FILE *FBL_Filelist_getFile(struct FBL_Filelist_Data* fblfd, char *mode) {
	char* path = fblfd->currentpath;
	char* name = fblfd->ih.items[fblfd->fblsd->sel]->name;
	char* tempbuf = malloc(strlen(path)+strlen(name)+1);
	strcpy(tempbuf,path);
	strcat(tempbuf,name);
	
	FILE* fh = fopen(tempbuf, mode);
	free(tempbuf);
	return fh;
}

char* FBL_Filelist_getFilename(struct FBL_Filelist_Data* fblfd, char* str, int maxlen) { // Must not be static.
	strncpy(str, fblfd->currentpath, maxlen);
	strncat(str, fblfd->ih.items[fblfd->fblsd->sel]->name, maxlen-strlen(fblfd->currentpath));
	return str;
}

static void FBL_Filelist_initBackground(struct FBL_Filelist_Data* fblfd) {
	Bdisp_AllClr_VRAM();
	SetBackGround(0xD);
	SaveVRAM_1();
}

void FBL_Filelist_go(struct FBL_Filelist_Data* fblfd) { // Must not be static.
	char buf[256];
	DebugDebounceAndPause("go 1.");
	sprintf(buf, "VADDR: %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);

	FBL_Filelist_initBackground(fblfd);

	DebugDebounceAndPause("go 2.");
	sprintf(buf, "VADDR: %p", VRAM_ADDR);
	DebugDebounceAndPause(buf);
	
	int key;
	while(!FBL_Filelist_isDone(fblfd)) {
		FBL_Filelist_render(fblfd);
		DebugDebounceAndPause("go 3.");
		sprintf(buf, "VADDR: %p", VRAM_ADDR);
		DebugDebounceAndPause(buf);
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


static void FBL_Filelist_setupStatusBar(struct FBL_Filelist_Data* fblfd) {
	EnableStatusArea(0);
	DefineStatusAreaFlags(DSA_SETDEFAULT, 0, 0, 0);
	DefineStatusAreaFlags(3, SAF_BATTERY | SAF_TEXT | SAF_GLYPH | SAF_ALPHA_SHIFT, 0, 0);
	DefineStatusMessage(fblfd->title, 0, TEXT_COLOR_BLACK, 0);
}

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
