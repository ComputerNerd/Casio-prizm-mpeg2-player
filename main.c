/*
 * sample6.c
 * Copyright (C) 2003	  Regis Duchesne <hpreg@zoy.org>
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This program reads a MPEG-2 stream, and saves each of its frames as
 * an image file using the PPM format (color).
 *
 * It demonstrates how to use the following features of libmpeg2:
 * - Output buffers use the RGB 24-bit chunky format.
 * - Output buffers are allocated and managed by the caller.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <fxcg/keyboard.h>
#include <fxcg/display.h>
#include <fxcg/rtc.h>
#include <fxcg/misc.h>

#include "filegui.h"

#include "mpeg2.h"
#include "mpeg2convert.h"

static void waitCasio(void){
	int key;
	GetKey(&key);
}


//The file functions are borrowed from CGPLAYER

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The whole sound doesn't fit onto the RAM.
//Reading per parts is not possible as this is synchronous player (there would be silences when reading).
//So I read each page (4KB)of the wav file and try to find it in the flash. 
//Simply finding start of the file is not enough because of fragmentation.

//Seach the whole flash, do not assume FS start at 0xA1000000 
//(already tried interval 0xA1000000 - 0xA1FFFFFF, but some parts of the file were outside of this interval)
#define FLASH_START 0xA0000000  
//page has 4 KB (I hope)
#define FLASH_PAGE_SIZE 4096
//8K pages
#define FLASH_PAGE_COUNT (4096*2)

//allocate 128 items for max 128 fragments of the file.
// 640 KB should to be enough for everyone ;-)
#define MAX_FRAGMENTS 2048
//descriptor for 1 fragment
typedef struct
{
	short msOffset;//page index (0 ~ 8K)
	short msCount;//count of pages in this fragment
}FileMappingItem;

typedef struct
{
	FileMappingItem mTable[MAX_FRAGMENTS];//table of fragments
	int miItemCount;
	int miTotalLength;//length of the file
	int miCurrentLength;//currently returned length (by GetNextdata() )
	int miCurrentItem;//active fragment (to be returned by GetNextdata() )

}FileMapping;

//reset reading to start
static void ResetData(FileMapping *pMap)
{
	pMap->miCurrentItem = 0;
	pMap->miCurrentLength = 0;
}

//get pointer to next fragment of the file
static int GetNextData(FileMapping *pMap,int *piLength,unsigned char **ppBuffer)
{
	unsigned char *pFlashFS = (unsigned char *)FLASH_START;
	FileMappingItem *pItem = &(pMap->mTable[pMap->miCurrentItem]);
	if(pMap->miCurrentItem >= pMap->miItemCount)
	{
		//eof ?
		*piLength = 0;
		return -1;
	}
	pMap->miCurrentItem++;
	*ppBuffer = pFlashFS + (((int)pItem->msOffset)*FLASH_PAGE_SIZE);
	*piLength = ((int)pItem->msCount)*FLASH_PAGE_SIZE;
	 pMap->miCurrentLength += *piLength;
	if(pMap->miCurrentLength >  pMap->miTotalLength)
	{
		//last page is only partially used
		*piLength -= pMap->miCurrentLength - pMap->miTotalLength;
		pMap->miCurrentLength = pMap->miTotalLength;
	}
	return 0;
}

#define Xmemcmp memcmp
//I was able to use memcmp(), it compiled but it seems it returned always 0 
// quick fix fot now
/*int Xmemcmp(const char *p1,const char *p2,int len)
{//May be needed if you use syscall memcmpy the version of libfxcg that I used to compile this does not
	while(len)
	{
		if(*p1 != *p2)
		{
			return 1;
		}
		p1++;
		p2++;
		len--;
	}
	return 0;
}*/


static int CreateFileMapping(const unsigned short *pFileName,FileMapping *pMap){
	int iResult = 0;
	char cBuffer[FLASH_PAGE_SIZE];
	int hFile = Bfile_OpenFile_OS(pFileName,0,0);
	int iLength,j;
	char *pFlashFS = (char *)FLASH_START;

	pMap->miItemCount = 0;
	pMap->miTotalLength = 0;
	iLength = Bfile_ReadFile_OS(hFile,cBuffer,FLASH_PAGE_SIZE,-1);
	while(iLength > 0)
	{
		//do not optimize (= do not move these 2 variables before loop)!
		// fx-cg allocates pages for file in <random> order so page from the end of the file 
		//can have lower index than page from the beginning
		const char *pTgt = pFlashFS;
		int iPageIndx = 0;

		for(;iPageIndx < FLASH_PAGE_COUNT;iPageIndx++)
		{
			if(!Xmemcmp(pTgt,cBuffer,iLength))
			{
				break;
			}
			pTgt += FLASH_PAGE_SIZE;
		}
		if(iPageIndx == FLASH_PAGE_COUNT)
		{
			//page not found !
			iResult = -2;
			goto lbExit;
		}
		pMap->miItemCount ++;
		if(pMap->miItemCount >= MAX_FRAGMENTS)
		{
			//file too fragmented !
			iResult = -3;
			goto lbExit;
		}
		pMap->mTable[pMap->miItemCount-1].msOffset = (short)iPageIndx;
		pMap->mTable[pMap->miItemCount-1].msCount = 0;
		//assume fragment has more pages
		for(;;)
		{
			pMap->mTable[pMap->miItemCount-1].msCount++;
			pMap->miTotalLength += iLength;
			iPageIndx++;
			pTgt += FLASH_PAGE_SIZE;

			if(iLength < FLASH_PAGE_SIZE)
			{
				//this was the last page
				iResult = pMap->miTotalLength;
				goto lbExit;
			}
			iLength = Bfile_ReadFile_OS(hFile,cBuffer,FLASH_PAGE_SIZE,-1);
			if(iLength <= 0)
			{
				break;
			}
			if(Xmemcmp(pTgt,cBuffer,iLength))
			{
				break;
			}
		}
	}
	if(iLength < 0)
	{
		iResult = -1;
	}
	else
	{
		iResult = pMap->miTotalLength;
	}

lbExit:
	Bfile_CloseFile_OS(hFile);
	return iResult;

}
// Module Stop Register 0
#define MSTPCR0	(volatile unsigned *)0xA4150030
// DMA0 operation register
#define DMA0_DMAOR	(volatile unsigned short*)0xFE008060
#define DMA0_SAR_0	(volatile unsigned *)0xFE008020
#define DMA0_DAR_0  (volatile unsigned *)0xFE008024
#define DMA0_TCR_0	(volatile unsigned *)0xFE008028
#define DMA0_CHCR_0	(volatile unsigned *)0xFE00802C
/* DMA register offsets
destination address register_0*/
//#define DAR_0		0x04
// transfer count register_0
//#define TCR_0		0x08
// channel control register_0
//#define CHCR_0		0x0C
#define LCD_BASE	0xB4000000
uint16_t* VRAM_ADDR;
#define SYNCO() __asm__ volatile("SYNCO\n\t":::"memory"); 
#define PRDR *(volatile unsigned char*)0xA405013C
#define LCDC *(volatile unsigned short*)0xB4000000

#define LCD_GRAM_X 0x200
#define LCD_GRAM_Y 0x201
#define LCD_GRAM 0x202
#define LCD_WINDOW_LEFT 0x210
#define LCD_WINDOW_RIGHT 0x211
#define LCD_WINDOW_TOP 0x212
#define LCD_WINDOW_BOTTOM 0x213

static void SelectLCDReg(unsigned short reg){
	SYNCO();
	PRDR &= ~0x10;
	SYNCO();
	LCDC = reg;
	SYNCO();
	PRDR |= 0x10;
	SYNCO();
	return;
}

static void WriteLCDReg(unsigned short reg, unsigned short value){
	SelectLCDReg(reg);
	LCDC = value;
	return;
}

static unsigned short ReadLCDReg(unsigned short reg){
	SelectLCDReg(reg);
	return LCDC;
}
static void DmaWaitNext(void){
	while(1){
		if((*DMA0_DMAOR)&4)//Address error has occurred stop looping
			break;
		if((*DMA0_CHCR_0)&2)//Transfer is done
			break;
	}
	SYNCO();
	*DMA0_CHCR_0&=~1;
	*DMA0_DMAOR=0;
}

static void DoDMAlcdNonblock(void){
	Bdisp_WriteDDRegister3_bit7(1);
	Bdisp_DefineDMARange(6,389,0,215);
	Bdisp_DDRegisterSelect(LCD_GRAM);

	*MSTPCR0&=~(1<<21);//Clear bit 21
	*DMA0_CHCR_0&=~1;//Disable DMA on channel 0
	*DMA0_DMAOR=0;//Disable all DMA
	*DMA0_SAR_0=((unsigned)VRAM_ADDR)&0x1FFFFFFF;//Source address is VRAM
	*DMA0_DAR_0=LCD_BASE&0x1FFFFFFF;//Destination is LCD
	*DMA0_TCR_0=(216*384)/16;//Transfer count bytes/32
	*DMA0_CHCR_0=0x00101400;
	*DMA0_DMAOR|=1;//Enable DMA on all channels
	*DMA0_DMAOR&=~6;//Clear flags
	*DMA0_CHCR_0|=1;//Enable channel0 DMA
}

#define MaxW 384
#define MaxH 216
static void DisplayFrame(int w1,int h1,const uint16_t * buf){
	uint16_t*vram=VRAM_ADDR;
	if((w1==384)&&(h1<=216)){
		//Simply center image and copy data
		vram+=((216-h1)/2)*384;
		DmaWaitNext();
		memcpy(vram,buf,384*2*h1);
	}else{
		int w2,h2,centerx,centery;
		int xpick=(int)((384<<16)/w1)+1,ypick=(int)((216<<16)/h1)+1;
		if(xpick==ypick){
			w2=384;
			h2=216;
			centerx=centery=0;
		}else if(xpick<ypick){
			w2=384;
			h2=h1*384/w1;
			centerx=0;
			centery=(216-h2)/2;
		}else{
			w2=w1*216/h1;
			h2=216;
			centerx=(384-w2)/2;
			centery=0;
		}
		// EDIT: added +1 to account for an early rounding problem
		int x_ratio = (int)((w1<<16)/w2)+1;
		int y_ratio = (int)((h1<<16)/h2)+1;
		int x2,y2,i,j;
		DmaWaitNext();
		vram+=(centery*384)+centerx;
		for (i=0;i<h2;++i){
			for (j=0;j<w2;++j){
				x2 = ((j*x_ratio)>>16);
				y2 = ((i*y_ratio)>>16);
				*vram++=buf[(y2*w1)+x2];
			}
			vram+=384-w2;	
		}
	}
	//Bdisp_PutDisp_DD();
	DoDMAlcdNonblock();
	//DmaWaitNext();
	//waitCasio();
}
static uint8_t pixbuf[MaxW*MaxH*2*3];
static struct fbuf_s {
	uint8_t * rgb[3];
	int used;
} fbuf[3];

static void casioError(void){
	int key;
	while(1) GetKey(&key);
}
static struct fbuf_s * get_fbuf (void){
	int i;

	for (i = 0; i < 3; i++)
	if (!fbuf[i].used) {
		fbuf[i].used = 1;
		return fbuf + i;
	}
	fputs(stderr, "Could not find a free fbuf.\n");
	casioError();
}
static int key_down(int basic_keycode){
	const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
	int row, col, word, bit;
	row = basic_keycode%10;
	col = basic_keycode/10-1;
	word = row>>1;
	bit = col + ((row&1)<<3);
	return (0 != (keyboard_register[word] & 1<<bit));
}
void getStrn(int x,int y, char * buffer,int n){
	int start = 0; // Used for scrolling left and right
	int cursor = 0; // Cursor position
	buffer[0] = '\0'; // This sets the first character to \0, also represented by "", an empty string

	DisplayMBString((unsigned char*)buffer, start, cursor, x, y); // Last to parameters are X,Y coords (not in pixels)

	int key;
	while(1){
		GetKey(&key); // Blocking is GOOD.  This gets standard keys processed and, possibly, powers down the CPU while waiting
		if(key == KEY_CTRL_EXE){
			// Ok
			break;
		}else if(key == KEY_CTRL_EXIT){
			// Aborted
			break;
		}else if(key && key < 30000){
			cursor = EditMBStringChar((unsigned char*)buffer, n, cursor, key);
			DisplayMBString((unsigned char*)buffer, start, cursor, x,y);
		}else{
			EditMBStringCtrl((unsigned char*)buffer, n, &start, &cursor, &key, x, y);
		}
	}
}
//#define BUFFER_SIZE 4096
//static uint8_t buffer[BUFFER_SIZE];
static inline unsigned hackRET(unsigned char*x){
	return (unsigned)x;
}
int main (void){
	VRAM_ADDR = GetVRAMAddress();
	Bdisp_EnableColor(1);
	//Bdisp_AllClr_VRAM();
	mpeg2dec_t * decoder;
	const mpeg2_info_t * info;
	mpeg2_state_t state;
	size_t size;
	int framenum = 0;
	int pixels;
	int i;
	struct fbuf_s * current_fbuf;
	while(1){
		//First of all pick file
		struct FBL_Filelist_Data *list = FBL_Filelist_cons("\\\\fls0\\", "*.m2v", "Open file (*.m2v)");

		// Actual GUI happens here
		FBL_Filelist_go(list);

		// Optional
		DrawFrame(COLOR_BLACK);
		
		// Check if a file was returned
		if(list->result == 1) {
			char buf[128];
			FBL_Filelist_getFilename(list,buf,127);
			if (!strncmp(buf,"\\\\fls0\\",7)){
				unsigned char *gpBuffer;
				int giLength=-1;
				unsigned short NAME[128];
				Bfile_StrToName_ncpy(NAME,buf,strlen(buf)+1);
				FileMapping aMap;
				memset(&aMap,0,sizeof(FileMapping));
				int iRes = CreateFileMapping(NAME,&aMap);
				switch(iRes)
				{
				case -1:
					puts("File read error");
					waitCasio();
					goto lbExit;
					break;
				case -2:
					puts("Page not found");
					waitCasio();
					goto lbExit;
					break;
				case -3:
					puts("File too fragmented");
					waitCasio();
					goto lbExit;
					break;
				default:
					break;
				}
				// TODO: PROCESS FLASH FILE HERE
				while(1){
					//puts("Starting...");
					//waitCasio();
					uint8_t * SaveVramAddr=getSecondaryVramAddress();
					if(hackRET(SaveVramAddr)&3)
						SaveVramAddr+=4-(hackRET(SaveVramAddr)&3);//Align address
					decoder = mpeg2_init(SaveVramAddr);
					//puts("Done");
					//waitCasio();
					if (decoder == NULL) {
						fputs(stderr, "Could not allocate a decoder object.\n");
						casioError();
					}
					info = mpeg2_info (decoder);
					//puts("info");
					//waitCasio();
					size = 0;
					int ticksPerFrame;
					memset(VRAM_ADDR,0,384*216*2);
					PrintXY(1,1,"  Enter ticks per frame",0x20,TEXT_COLOR_WHITE);
					PrintXY(1,2,"  128 ticks in a second",0x20,TEXT_COLOR_WHITE);
					//DoDMAlcdNonblock(0,215);
					//DmaWaitNext();
					{char buf[16];
					getStrn(1,3,buf,16);
					ticksPerFrame=atoi(buf);}
					if(ticksPerFrame<0)
						ticksPerFrame=0;
					memset(VRAM_ADDR,0,384*24*4*2);
					//DoDMAlcdNonblock();
					//DmaWaitNext();
					int iRes=0;
					int ticks=RTC_GetTicks();
					mpeg2_custom_fbuf(decoder, 1);
					do {
						if(key_down(KEY_PRGM_EXIT))
							break;
						state = mpeg2_parse (decoder);
						//printf("State %d\n",state);
						//Bdisp_PutDisp_DD();
						switch (state) {
						case STATE_BUFFER:
							/*size = fread (buffer, 1, BUFFER_SIZE, mpgfile);
							mpeg2_buffer (decoder, buffer, buffer + size);*/
							/*if(sizeof(casiompg)-size>BUFFER_SIZE){
								memcpy(buffer,casiompg+size,BUFFER_SIZE);
								mpeg2_buffer(decoder,buffer,buffer+BUFFER_SIZE);
								size+=BUFFER_SIZE;
							}else{
								memcpy(buffer,casiompg+size,sizeof(casiompg)-size);
								mpeg2_buffer(decoder,buffer,buffer+sizeof(casiompg)-size);
								size+=sizeof(casiompg)-size;
							}*/
							iRes=GetNextData(&aMap,&giLength,&gpBuffer);
							mpeg2_buffer (decoder, gpBuffer, gpBuffer + giLength);
							break;
						case STATE_SEQUENCE:
							mpeg2_convert (decoder, mpeg2convert_rgb16, NULL);
							mpeg2_custom_fbuf (decoder, 1);
							pixels = info->sequence->width * info->sequence->height;
							if(pixels>(MaxW*MaxH)){
								char buf[16];
								fputs(stderr,"Error too much ram usage\nMax ram usage per frame is ");
								itoa(MaxW*MaxH*2,buf);
								fputs(stderr,buf);
								fputs(stderr,"\nYou used ");
								itoa(pixels*2,buf);
								fputs(stderr,buf);
								fputc('\n',stderr);
								casioError();
							}
							for (i = 0; i < 3; i++) {
							fbuf[i].rgb[0] = &pixbuf[i*info->sequence->width*info->sequence->height*2];
							fbuf[i].rgb[1] = fbuf[i].rgb[2] = NULL;
							if (!fbuf[i].rgb[0]) {
								fputs(stderr, "Could not allocate an output buffer.\n");
								//exit (1);
								casioError();
							}
							fbuf[i].used = 0;
							}
							for (i = 0; i < 2; i++) {
							current_fbuf = get_fbuf ();
							mpeg2_set_buf (decoder, current_fbuf->rgb, current_fbuf);
							}
							break;
						case STATE_PICTURE:
							current_fbuf = get_fbuf ();
							mpeg2_set_buf (decoder, current_fbuf->rgb, current_fbuf);
							break;
						case STATE_SLICE:
						case STATE_END:
						case STATE_INVALID_END:
							/*if (info->display_fbuf)
							save_ppm (info->sequence->width, info->sequence->height,
								  info->display_fbuf->buf[0], framenum++);*/
							if (info->display_fbuf){
								while((ticks+ticksPerFrame)>RTC_GetTicks());
									ticks=RTC_GetTicks();
								DisplayFrame(info->sequence->width, info->sequence->height,(const uint16_t*)info->display_fbuf->buf[0]);
							}
							framenum++;
							if (info->discard_fbuf)
									((struct fbuf_s *)info->discard_fbuf->id)->used = 0;
							/*if (state != STATE_SLICE)
							for (i = 0; i < 3; i++)
								free (fbuf[i].rgb[0]);*/
							break;
						default:
							break;
						}
					} while(!iRes);
					mpeg2_close (decoder);
					ResetData(&aMap);
					if(key_down(KEY_PRGM_EXIT))
							break;
				}
				DmaWaitNext();
			}
		}
lbExit:
		FBL_Filelist_destr(list);
		
	}
	return 0;
}
