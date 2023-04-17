#include <Quickdraw.h>
#include <Events.h>
#include <Files.h>
#include <Fonts.h>
#include <NumberFormatting.h>

#define SCALE 1
#define WIREFRAME 0

#define POP_BYTE(p) (*(uint8_t*)p++)
static uint16_t do_pop_word(void** out,void* in){
	*out=in+2;
	uint8_t* ptr=(uint8_t*)in;
	uint16_t ret=ptr[0]<<8^ptr[1];
	return ret;
}
#define POP_WORD(p) do_pop_word(&p,p)

PolyHandle poly;
GrafPort myPort;
int16_t polybuf[(1+sizeof(Polygon)+(sizeof(Point)*14))/sizeof(int16_t)];

Pattern* gradient[]={
	&qd.white,
	&qd.ltGray,
	&qd.gray,
	&qd.dkGray,
	&qd.black,
};

Pattern* pal[16]={};

static void* parse_pal(void* ptr){
	Pattern** p=&pal[15];
	uint16_t mask=POP_WORD(ptr);
	while(mask){
		if(mask&0x1){
			uint16_t rgb=POP_WORD(ptr);
			uint16_t b=rgb&7;
			rgb>>=4;
			uint16_t g=rgb&7;
			rgb>>=4;
			uint16_t r=rgb&7;
			//r=0;
			//g=0;
			//b=0;
			//uint16_t luma=((r+r+g+g+g+b)*4)/(7*6);
			//uint16_t luma=((r+g+b)*4)/(7*3);
			uint16_t luma=(g*3)/7;
			*p=gradient[luma];
		}
		--p;
		mask>>=1;
	}
	return ptr;
}

static void* parse_frame(void* b){
	//printf("\nframe\n");
	//SetPort(&myPort);
	//PenPat(&qd.white);
	while(1){
		int poly_desc=POP_BYTE(b);
		switch(poly_desc){
			case 0xff:
				return b;
			case 0xfe:
				return (void*)1;
			case 0xfd:
				return 0;
			default:
				break;
		}
		PolyHandle poly=OpenPoly();
		int poly_vert_count=poly_desc&0xf;
		int poly_color=poly_desc>>4;
		int first_x=POP_BYTE(b)<<SCALE;
		int first_y=POP_BYTE(b)<<SCALE;
		MoveTo(first_x,first_y);
		while(--poly_vert_count){
			int x=POP_BYTE(b)<<SCALE;
			int y=POP_BYTE(b)<<SCALE;
			LineTo(x,y);
		}
		LineTo(first_x,first_y);
		ClosePoly();
		if(WIREFRAME){
			FramePoly(poly);
		}else{
			FillPoly(poly,pal[poly_color]);
		}
		KillPoly(poly);
	}
}

uint16_t vert_buf[255][2];

static void* parse_frame_indexed(void* b){
	//printf("\nindexed frame\n");

	uint16_t vert_count=POP_BYTE(b);
	//printf("vert_count=%i\n",vert_count);
	for(int i=0;i<vert_count;++i){
		vert_buf[i][0]=POP_BYTE(b)<<SCALE;
		vert_buf[i][1]=POP_BYTE(b)<<SCALE;
	}
	//for(int i=0;i<vert_count;++i){
	//	printf("%i,%i ",
	//			vert_buf[i][0],
	//			vert_buf[i][1]);
	//}
	//printf("\n");

	//SetPort(&myPort);
	//PenPat(&qd.white);
	while(1){
		uint16_t poly_desc=POP_BYTE(b);
		switch(poly_desc){
			case 0xff:
				return b;
			case 0xfe:
				return (void*)1;
			case 0xfd:
				return 0;
			default:
				break;
		}
		PolyHandle poly=OpenPoly();
		uint16_t poly_vert_count=poly_desc&0xf;
		uint16_t poly_color=poly_desc>>4;
		uint16_t first_vert=POP_BYTE(b);
		MoveTo(vert_buf[first_vert][0],vert_buf[first_vert][1]);
		while(--poly_vert_count){
			uint16_t vert=POP_BYTE(b);
			LineTo(vert_buf[vert][0],vert_buf[vert][1]);
		}
		LineTo(vert_buf[first_vert][0],vert_buf[first_vert][1]);
		ClosePoly();
		if(WIREFRAME){
			FramePoly(poly);
		}else{
			FillPoly(poly,pal[poly_color]);
		}
		KillPoly(poly);
	}
}

static int parse_block(void* b){
	//printf("%02x ",POP_BYTE(b));
	//printf("\n");
	//for(int32_t i=0;i<256;++i){
	//	printf("%04x ",POP_WORD(b));
	//}
	//printf("\n");
	
	while(b>(void*)1){
		uint8_t flags=POP_BYTE(b);
		if(WIREFRAME||(flags&0x1)){
			//SetPort(&myPort);
			FillRect(&myPort.portRect,&qd.black);
		}
		if(flags&0x2) b=parse_pal(b);
		if(flags&0x4) b=parse_frame_indexed(b);
		else b=parse_frame(b);
		//__asm__ __volatile__(
		//		"bchg #6,0xeffffe\n\t"
		//		:::);

	}
	return b!=0;
}

int main(int argc, char** argv){
	InitGraf(&qd.thePort);
	InitCursor();
	HideCursor();

	OpenPort(&myPort);
	FillRect(&qd.screenBits.bounds,&qd.black);

	PenPat(&qd.white);
	int w=WIREFRAME+(255<<SCALE);
	int h=WIREFRAME+(199<<SCALE);
	int x=(myPort.portRect.right-w)/2;
	int y=(myPort.portRect.bottom-h)/2;
	SetOrigin(-x,-y);
	SetRect(&myPort.portRect,0,0,w,h);

	short f=0;
	FSOpen("\pscene1.bin",f,&f);

	long start_ticks=TickCount();

#define BLOCKSIZE (64L*1024L)
	long count=BLOCKSIZE;
	uint8_t b[BLOCKSIZE];
	do{
		FSRead(f,&count,&b);
		if(!count) break;
	}while(parse_block(b));

	long ticks=TickCount()-start_ticks;

	FSClose(f);

	Str255 time;
	NumToString(ticks,time);

	ClosePort(&myPort);
	OpenPort(&myPort);

	//Rect r;
	//SetRect(&r,10,10,20+StringWidth(time),30);
	//FillRect(&r,&qd.black);
	InitFonts();
	TextMode(notSrcCopy);
	MoveTo(15,25);
	DrawString(time);

	while(!Button());
	FlushEvents(everyEvent, -1);

	return 0;
}
