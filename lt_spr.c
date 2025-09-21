/*Compile And run sprites functions from XLIB - Mode X graphics library
; Written By Themie Gouthas                  
; Aeronautical Research Laboratory           
; Defence Science and Technology Organisation
; Australia                                  
;
; egg@dstos3.dsto.gov.au
; teg@bart.dsto.gov.au
*/

#include "lt__eng.h"

#define SC_INDEX            0x03c4    // VGA sequence controller
#define SC_DATA             0x03c5

#define GC_INDEX            0x03ce    // VGA graphics controller
#define GC_DATA             0x03cf

#define MAP_MASK            0x02
#define ALL_PLANES          0xff02
 

SPRITE far *sprite;

//Player
byte tile_number = 0;		//Tile a sprite is on
byte tilecol_number = 0;	//Collision Tile a sprite is on
byte tilecol_number_down = 0;//Collision Tile down
byte tile_number_VR = 0;	//Tile vertical right
byte tile_number_VL = 0;	//Tile vertical left
byte tile_number_HU = 0;	//Tile horizontal up
byte tile_number_HD = 0;	//Tile horizontal down

byte LT_Sprite_Stack = 0;
byte LT_Sprite_Stack_Table[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte LT_AI_Enabled = 0;
byte LT_Active_AI_Sprites[] = {0,0,0,0,0,0,0};
byte Lt_AI_Sprite[] = {0,0,0,0,0,0,0,0};
byte LT_Sprite_Size_Table[] = {8,8,8,8,8,8,8,8,16,16,16,16,16,16,16,16,32,32,32,32,64};
//positions in ram of 8 copies of EGA sprites, for smooth panning
word LT_Sprite_Size_Table_EGA_8[] = {0,32,64,96,128,160,192,224};	
word LT_Sprite_Size_Table_EGA_16[] = {0,96,192,288,384,480,576,672};	
word LT_Sprite_Size_Table_EGA_32[] = {0,320,640,960,1280,1600,1920,2240};
word LT_Sprite_Size_Table_EGA_64[] = {0,576,1152,1728,2304,2880,3456,4032};


//Positions of sprite bkg in array, for cga and tandy
word tandy_sprite_bkg_table[20] = {0,48,96,144,192,240,288,336, 384,544,704,864,1024,1184,1344,1504, 1664,2240,2816,3392};

byte far *sprite_id_table;
byte sprite_size = 2;

int LT_SPRITE_PLAYER = 0;
byte selected_AI_sprite;
int LT_number_of_ai = 0;
unsigned char far *LT_sprite_data;
unsigned char far *tandy_bkg_data;
extern byte LT_AI_Sprite[];
extern byte LT_VIDEO_MODE;
extern byte far *LT_map_collision;
extern word far *LT_map_data; 
extern unsigned char far *LT_tile_tempdata; 
extern unsigned char far *LT_tile_tempdata2;  
dword LT_sprite_data_offset; //just after loading animation, 4kb for vga, 512 bytes for EGA; 
void LT_Error(char *error, char *file);
extern byte *LT_Filename;
extern word LT_VRAM_Logical_Width;
void LT_VGA_Enable_4Planes();
void LT_VGA_Return_4Planes();
void LT_TGA_MapPage();
extern word TILE_VRAM;
extern word LT_FrameSkip;
extern byte tga_page;
//player movement modes
//MODE TOP = 0;
//MODE PLATFORM = 1;
//MODE PUZZLE = 2;
//MODE SIDESCROLL = 3;
byte LT_EGA_SPRITES_TRANSLUCENT = 0;
byte LT_MODE = 0;
byte LT_ENEMY_DIST_X = 15;
byte LT_ENEMY_DIST_Y = 15;
byte LT_Player_Dir = 0;
byte LT_Slope_Timer = 0; //A dirty way of making big slopes or long conveyor belts impossible to cross

// Player variables: 
//tile, collision tile. collision x-y,hit,jump,action,move,ground
//move = 0 1 2 3 4 = Stop U D L R 
byte LT_Player_State[9] = {0,0,0,0,0,0,0,0,0};

int x_adjust = 160;
int LT_Jump_Release = 1;
int LT_player_jump_pos[] = {
	-3,-3,-3,-2,-2,-2,-2,-2,-1,-2,-2,-2,-1,-2,-2,-1,-2,-1,-1,-2,
	-1,-1,-1,-1,-1, 0,-1,-1,-1, 0,-1, 0,
	-1, 0, 0, 1,
	 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1,
	 2, 1, 1, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 3, 3, 3
};

int LT_Spr_speed_float = 0;
int far LT_Spr_speed[] = {
	//Add speeds for
	-2,-2,-3,-2,-2,-3,-2,-2,
	-2,-2,-2,-2,-1,-2,-2,-2,
	-2,-2,-2,-1,-1,-2,-2,-2,
	-2,-1,-2,-1,-2,-2,-2,-1,
	-2,-1,-2,-1,-2,-1,-2,-1,
	-2,-1,-1,-2,-1,-1,-2,-1,
	-2,-1,-1,-1,-2,-1,-1,-1,
	-2,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1, 0,-1,-1,-1,
	-1,-1,-1, 0, 0,-1,-1,-1,
	-1, 0,-1,-1,-1, 0,-1, 0,
	-1, 0,-1, 0,-1, 0,-1, 0,
	-1, 0, 0,-1, 0, 0,-1, 0,	
	-1, 0, 0, 0,-1, 0, 0, 0,
	-1, 0, 0, 0, 0, 0, 0, 0,
	
	0,0,0,0,0,0,0,0,	//8x16
	
	1,0,0,0,0,0,0,0,
	1,0,0,0,1,0,0,0,
	1,0,0,1,0,0,1,0,
	1,0,1,0,1,0,1,0,
	1,0,1,1,1,0,1,0,
	1,1,1,0,0,1,1,1,
	1,1,1,1,0,1,1,1,
	1,1,1,1,1,1,1,1,
	2,1,1,1,1,1,1,1,
	2,1,1,1,2,1,1,1,
	2,1,1,2,1,1,2,1,
	2,1,2,1,2,1,2,1,
	2,1,2,1,2,2,2,1,
	2,2,2,1,1,2,2,2,
	2,2,2,2,1,2,2,2,
	2,2,3,2,2,3,2,2,	//8x32
};


int LT_wmap = 0;
int LT_hmap = 0;//91.888
int LT_Scroll_Camera_float = 0;
int LT_Scroll_Camera_array[135] = {
	0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,
	8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

int LT_Scroll_Camera_speed[] = {
	0,0,0,0,0,0,0,0,
	1,0,0,0,1,0,0,0,
	1,0,1,0,1,0,1,0,
	1,1,1,0,1,1,1,0,
	1,1,1,1,1,1,1,1,
	2,1,1,1,2,1,1,1,
	2,1,2,1,2,1,2,1,
	2,2,2,1,2,2,2,1,
	2,2,2,2,2,2,2,2,
};

int _abs(int x) {
    int result;
    asm mov ax, x
    asm cwd            // DX is sign of AX
    asm xor ax, dx
    asm sub ax, dx
    asm mov result, ax
    return result;
}

void _memcpy(void *dest, void *src, word number);

void LT_scroll_follow(int sprite_number) {
    SPRITE *s = &sprite[sprite_number];
    int x, y, x1, y1;
    int speed_x, speed_y;
    int idx_x, idx_y;

    LT_SPRITE_PLAYER = sprite_number;

    // Facing direction adjustment
    if (s->state == 3 && x_adjust < 190) x_adjust++;
    else if (s->state == 4 && x_adjust > 130) x_adjust--;

    // Relative position
    y  = (s->pos_y - SCR_Y) - 120;
    y1 = _abs(y);
    x  = (s->pos_x - SCR_X) - x_adjust;
    x1 = _abs(x);

    // Only scroll if camera inside map
    if (SCR_X >= 0 && (SCR_X + 319) < LT_wmap &&
        SCR_Y >= 0 && (SCR_Y + 200) < LT_hmap)
    {
        if (LT_Scroll_Camera_float == 8) LT_Scroll_Camera_float = 0;

        idx_x = (LT_Scroll_Camera_array[x1] << 3) + LT_Scroll_Camera_float;
        idx_y = (LT_Scroll_Camera_array[y1] << 3) + LT_Scroll_Camera_float;

        speed_x = LT_Scroll_Camera_speed[idx_x];
        speed_y = LT_Scroll_Camera_speed[idx_y];

        if (x < 0) SCR_X -= speed_x;
        else if (x > 0) SCR_X += speed_x;

        if (y < 0) {
            if (!(LT_MODE == 1 && LT_VIDEO_MODE > 1) || s->ground)
                SCR_Y -= speed_y;
        } else if (y > 0) {
            SCR_Y += speed_y;
        }

        LT_Scroll_Camera_float++; // increment here, after using it
    }

    // Clamp to map
    if (SCR_X < 0) SCR_X = 0;
    else if ((SCR_X + 320) > LT_wmap) SCR_X = LT_wmap - 320;

    if (SCR_Y < 0) SCR_Y = 0;
    else if ((SCR_Y + 201) > LT_hmap) SCR_Y = LT_hmap - 201;
}

void LT_Load_BMP(char *file, char *dat_string, int mode, int sprite_number);

word Compile_Bitmap(word logical_width, unsigned char *bitmap, unsigned char *output){
	word bwidth,scanx,scany,outputx,outputy,column,set_column,input_size,code_size;

	asm push si
	asm push di
	asm push ds

	asm mov word ptr [scanx],0
	asm mov word ptr [scany],0
	asm mov word ptr [outputx],0
	asm mov word ptr [outputy],0
	asm mov word ptr [column],0
	asm mov word ptr [set_column],0

	asm lds si,[bitmap]     // 32-bit pointer to source bitmap

	asm les di,[output]     // 32-bit pointer to destination stream

	asm lodsb               // load width byte
	asm xor ah, ah          // convert to word
	asm mov [bwidth], ax    // save for future reference
	asm mov bl, al          // copy width byte to bl
	asm lodsb               // load height byte -- already a word since ah=0
	asm mul bl              // mult height word by width byte
	asm mov [input_size], ax//  to get pixel total 

_MainLoop:
	asm mov bx, [scanx]     // position in original bitmap
	asm add bx, [scany]

	asm mov al, [si+bx]     // get pixel
	asm or  al, al          // skip empty pixels
	asm jnz _NoAdvance
	asm jmp _Advance
_NoAdvance:

	asm mov dx, [set_column]
	asm cmp dx, [column]
	asm je _SameColumn
_ColumnLoop:
	asm mov word ptr es:[di],0c0d0h// emit code to move to new column
	asm add di,2
	asm mov word ptr es:[di],0d683h
	asm add di,2
	asm mov byte ptr es:[di],0
	asm inc di

	asm inc dx
	asm cmp dx, [column]
	asm jl _ColumnLoop

	asm mov byte ptr es:[di],0eeh
	asm inc di// emit code to set VGA mask for new column
	
	asm mov [set_column], dx
_SameColumn:
	asm mov dx, [outputy]   // calculate output position
	asm add dx, [outputx]
	asm sub dx, 128

	asm add word ptr [scanx], 4
	asm mov cx, [scanx]     // within four pixels of right edge?
	asm cmp cx, [bwidth]
	asm jge _OnePixel

	asm inc word ptr [outputx]
	asm mov ah, [si+bx+4]   // get second pixel
	asm or ah, ah
	asm jnz _TwoPixels
_OnePixel:
	asm cmp dx, 127         // can we use shorter form?
	asm jg _OnePixLarge
	asm cmp dx, -128
	asm jl _OnePixLarge
	asm mov word ptr es:[di],044c6h
	asm add di,2
	
	asm mov byte ptr es:[di],dl
	asm inc di// 8-bit position in output
	
	asm jmp _EmitOnePixel
_OnePixLarge:
	asm mov word ptr es:[di],084c6h
	asm add di,2
	asm mov word ptr es:[di],dx
	asm add di,2 //position in output
_EmitOnePixel:
	asm mov byte ptr es:[di],al
	asm inc di
	asm jmp short _Advance
_TwoPixels:
	asm cmp dx, 127
	asm jg _TwoPixLarge
	asm cmp dx, -128
	asm jl _TwoPixLarge
	asm mov word ptr es:[di],044c7h
	asm add di,2
	asm mov byte ptr es:[di],dl
	asm inc di            // 8-bit position in output
	asm jmp _EmitTwoPixels
_TwoPixLarge:
	asm mov word ptr es:[di],084c7h
	asm add di,2
	asm mov word ptr es:[di],dx
	asm add di,2 // position in output
_EmitTwoPixels:
	asm mov word ptr es:[di],ax
	asm add di,2

_Advance:
	asm inc word ptr [outputx]
	asm mov ax, [scanx]
	asm add ax, 4
	asm cmp ax, [bwidth]
	asm jl _AdvanceDone
	asm mov dx, [outputy]
	asm add dx, [logical_width]
	asm mov cx, [scany]
	asm add cx, [bwidth]
	asm cmp cx, [input_size]
	asm jl _NoNewColumn
	asm inc word ptr [column]
	asm mov cx, [column]
	asm cmp cx, 4
	asm je _Exit			// Column 4: there is no column 4.
	asm xor cx, cx          // scany and outputy are 0 again for
	asm mov dx, cx          // the new column
_NoNewColumn:
	asm mov [outputy], dx
	asm mov [scany], cx
	asm mov word ptr [outputx], 0
	asm mov ax,[column]
_AdvanceDone:
	asm mov [scanx], ax
	asm jmp _MainLoop

_Exit:
	asm mov byte ptr es:[di],0cbh
	asm inc di
	asm mov ax,di
	asm sub ax,word ptr [output] // size of generated code
	asm mov code_size,ax

	asm pop ds
	asm pop di
	asm pop si
	
	return code_size;
}


byte *TGA=(byte *)0xB8000000L;

word c_offset = 0;
word c_offset1 = 0;
word c_code_size = 0;
word c_skipped_bytes = 0;
void write_skip(){
	//write: add di,skipped_bytes
	LT_sprite_data[c_offset1++] = 0x81;
	LT_sprite_data[c_offset1++] = 0xC7;
	LT_sprite_data[c_offset1++] = c_skipped_bytes&0x00FF;
	LT_sprite_data[c_offset1++] = c_skipped_bytes>>8;
	c_code_size+=4;
	c_skipped_bytes = 0;
}
void write_inc(byte inc){
	//write: add di,2
	LT_sprite_data[c_offset1++] = 0x81;
	LT_sprite_data[c_offset1++] = 0xC7;
	LT_sprite_data[c_offset1++] = inc;
	LT_sprite_data[c_offset1++] = 0x00;
	c_code_size+=4;
}
void write_word(word pixels){
	c_offset1-=3;c_code_size-=3;//Undo write pixel
	//write: mov ax,word
	LT_sprite_data[c_offset1++] = 0xB8;
	LT_sprite_data[c_offset1++] = pixels&0x00FF;
	LT_sprite_data[c_offset1++] = pixels>>8;
	//write: stosw (mov es:[di],ax) 
	LT_sprite_data[c_offset1++] = 0xAB;
	c_code_size+=4;
}
void write_pixel(byte pixel){
	//write: mov al,byte
	LT_sprite_data[c_offset1++] = 0xB0;
	LT_sprite_data[c_offset1++] = pixel;
	//write: stosb (mov byte ptr es:[di],al)
	LT_sprite_data[c_offset1++] = 0xAA;
	c_code_size+=3;
}

unsigned char tga_start_line_code[] = {0x81,0xE7,0xFF,0x1F,0x89,0xD8,0x25,0x03,0x00,0xB1,0x0D,0xD3,0xE0,0x01,0xC7};
unsigned char tga_end_line_code[] = {0x43,0x81,0xC7,0xF8,0x1F,0x89,0xD8,0x25,0x03,0x00,0x75,0x04,0x81,0xC7,0xA0,0x00};
word compile_tga_sprite(int width, unsigned char* source_data, word sprite_data_offset){
	int x,y;
	byte pbyte0 = 0;
	byte pbyte1 = 0;
	byte transparent = 0x0E;//Yellow is barely used
	//word of_debug = sprite_data_offset;
	word line_jump = 8192-(width/2);
	tga_end_line_code[3] = line_jump&0x00FF;
	tga_end_line_code[4] = line_jump>>8;
	c_offset = 0;
	c_offset1 = sprite_data_offset;
	c_code_size = 0;
	
	for (y = 0; y<width;y++){
		byte color_bytes = 0;
		c_skipped_bytes = 0;
		//Write start line code
		_memcpy(&LT_sprite_data[c_offset1],tga_start_line_code,15);
		c_offset1+=15;c_code_size+=15;
		
		//LINE/////
		for (x = 0; x<width/2;x++){
			byte pixels = 0;
			byte b0 = source_data[c_offset++];
			byte b1 = source_data[c_offset++];
			if (b0 != transparent) pixels = 1;if (b1 != transparent) pixels = 1;

			if (pixels) {
				color_bytes++;
				pbyte1 = pbyte0;
				pbyte0 = (b0<<4)+b1;
				if (c_skipped_bytes) write_skip();
				if (color_bytes == 2) {write_word((pbyte0<<8)+pbyte1);color_bytes = 0;}
				else        write_pixel(pbyte0);
			} else {c_skipped_bytes++;color_bytes = 0;}
		}
		////////
		
		if (c_skipped_bytes) write_skip();
		//Write end line code
		if (y<(width-1)){
			_memcpy(&LT_sprite_data[c_offset1],tga_end_line_code,15);
			c_offset1+=16;
			c_code_size+=16;
		}
	}
	//write far ret
	LT_sprite_data[c_offset1++] = 0xCB;
	c_code_size++;
	//_memcpy(&LT_sprite_data[offset1++],&code_size,2);
	
	return c_code_size;
}

char SPR[] = {
	0x81, 0xE7, 0xFF, 0x1F, //AND DI
	0x81, 0xC7, 0x00, 0x20,	//ADD DI 
	0x47, //INC DI
	0xB8, 0x00, 0x00,//MOV AX
	0xAB, //STOSW
	0x47, 
	0x81, 0xEF, 0xB4, 0x1F, //SUB DI
	
	0x81, 0xE7, 0xFF, 0x1F, 
	0xB8, 0x80, 0x1F, 
	0xAB, 
	0xB8, 0x14, 0x02, 
	0xAB, 
	0x81, 0xC7, 0xFC, 0x1F,
	0xB8, 0x0A, 0x9F, 
	0xAB, 
	0xB8, 0x15, 0x50, 
	0xAB, 


	0xCB

};

word compile_cga_sprite(int width, unsigned char* source_data, word sprite_data_offset){
	int x,y;
	//FILE *f = fopen("sprite.c","wb");
	byte pbyte0 = 0;
	byte pbyte1 = 0;
	//byte pbyte2 = 0;
	//byte pbyte3 = 0;
	byte transparent = 0x04;//color 4
	byte _2_lines = 0;
	//word of_debug = sprite_data_offset;
	//word line_jump = 8192-(width/4);
	//fwrite(SPR,32,1,f);
	//fprintf(f,"\n\n\n");
	c_offset = 0;
	c_offset1 = sprite_data_offset;
	c_code_size = 0;

	for (y = 0; y<width;y++){
		byte color_bytes = 0;
		c_skipped_bytes = 0;
		//Start 2 lines
		if (_2_lines == 0) {
			//and di,8191; Fix new offset
			LT_sprite_data[c_offset1++] = 0x81;
			LT_sprite_data[c_offset1++] = 0xE7;// and di
			LT_sprite_data[c_offset1++] = 0xFF;//
			LT_sprite_data[c_offset1++] = 0x1F;// 8191;
			//fprintf(f,"and di,8191\n");
			c_code_size+=4;
		}
	
		//LINE/////
		for (x = 0; x<width/4;x++){
			byte pixels = 0;
			byte b0 = source_data[c_offset++];
			byte b1 = source_data[c_offset++];
			byte b2 = source_data[c_offset++];
			byte b3 = source_data[c_offset++];
			if (b0 != transparent) pixels = 1;if (b2 != transparent) pixels = 1;

			if (pixels) {
				color_bytes++;
				pbyte1 = pbyte0;
				pbyte0 = (b0<<6)+(b1<<4)+(b2<<2)+b3;
				if (c_skipped_bytes) {write_skip();/*fprintf(f,"add di,%i\n",c_skipped_bytes);*/}
				if (color_bytes == 2) {write_word((pbyte0<<8)+pbyte1);color_bytes = 0;/*fprintf(f,"mov ax,word; stosw\n");*/}
				else        {write_pixel(pbyte0);/*fprintf(f,"mov al,byte; stosb\n");*/}
			} else {
				//if (color_bytes == 1){write_pixel(pbyte0);fprintf(f,"mov al,byte; stosb\n");}
				c_skipped_bytes++;
				color_bytes = 0;
			}
		}
		////////
		
		if (c_skipped_bytes) {write_skip();/*fprintf(f,"add di,%i\n",c_skipped_bytes);*/}
		//Write end line code
		if (y<(width-1)){
			_2_lines++;
			if (_2_lines == 1) {
				word b = 8192-(width/4);	// 8192 -bytes: next line
				LT_sprite_data[c_offset1++] = 0x81;
				LT_sprite_data[c_offset1++] = 0xC7;//add di
				LT_sprite_data[c_offset1++] = b&0x00FF;
				LT_sprite_data[c_offset1++] = b>>8;
				c_code_size+=4;
				//fprintf(f,"add di,%i\n",b);
			} if (_2_lines == 2) {
				word b = 8192-(80-(width/4));	//8192 -bytes -line: next bank line
				LT_sprite_data[c_offset1++] = 0x81;
				LT_sprite_data[c_offset1++] = 0xEF;//sub di
				LT_sprite_data[c_offset1++] = b&0x00FF;
				LT_sprite_data[c_offset1++] = b>>8;
				//fprintf(f,"sub di,%i\n",b);
				_2_lines = 0;
				c_code_size+=4; 
			}
		}
	}
	
	//write far ret
	LT_sprite_data[c_offset1++] = 0xCB;
	c_code_size++;
	//fprintf(f,"ret\n");
	//fclose(f);
	return c_code_size;
}

//load sprites with transparency (size = 8,16,32)
byte spr_line[8+64+8] = {0};
void LT_Load_Sprite(char *file, char *dat_string, int sprite_number, byte *animations){
	SPRITE *s = &sprite[sprite_number];
	long index,offset;
	word num_colors;
	word x,y;
	word i,j;
	word frame = 0;
	byte tileX;
	int tileY;
	byte size;

    int code_size;
	
	asm CLI
	LT_Draw_Text_Box(11,18,16,1,0,0,0,"LOADING: SPRITES");
	asm STI
	
	LT_Load_BMP(file,dat_string,2,sprite_number);

	//Get size of sprite according to its number in the sprite array
	size = LT_Sprite_Size_Table[sprite_number];
	if (size == 8) s->ega_size = &LT_Sprite_Size_Table_EGA_8[0];
	if (size == 16) s->ega_size = &LT_Sprite_Size_Table_EGA_16[0];
	if (size == 32) s->ega_size = &LT_Sprite_Size_Table_EGA_32[0];
	if (size == 64) s->ega_size = &LT_Sprite_Size_Table_EGA_64[0];

	index = 0; //use a chunk of temp allocated RAM to rearrange the sprite frames
	//Rearrange sprite frames one after another in temp memory
	for (tileY=(s->height-1);tileY>0;tileY-=size){
		for (tileX=0;tileX<s->width;tileX+=size){
			offset = (tileY*s->width)+tileX;
			if (LT_VIDEO_MODE==1){//VGA
				LT_tile_tempdata2[index] = size;
				LT_tile_tempdata2[index+1] = size;
				index+=2;
			}
			for(x=0;x<size;x++){
				_memcpy(&LT_tile_tempdata2[index],&LT_tile_tempdata[offset-(x*s->width)],size);
				index+=size;
			}
		}
	}
		
	s->nframes = (s->width/size) * (s->height/size);
	
	//Estimated size
	//fsize = (size * size * 7) / 2 + 25;
	s->code_size = 0;
	for (frame = 0; frame < s->nframes; frame++){
		if (LT_VIDEO_MODE == 1){
			dword check_size = 0;
			//Compile and store the compiled frame at LT_tile_tempdata
			code_size = Compile_Bitmap(LT_VRAM_Logical_Width, &LT_tile_tempdata2[(frame*2)+(frame*(size*size))],LT_tile_tempdata);
			//Check code size
			check_size = LT_sprite_data_offset + code_size;
			if (check_size > 0x0000FFFF) LT_Error("Not enough RAM to allocate frames $",file);
			if (!code_size) LT_Error("ERROR $",file);
			if (code_size > 28000) LT_Error("Sprite too complex $",file);
			//Store compiled sprite at final location
			memcpy(&LT_sprite_data[LT_sprite_data_offset],LT_tile_tempdata,code_size);
			s->frames[frame].compiled_code = &LT_sprite_data[LT_sprite_data_offset];
			LT_sprite_data_offset += code_size;
			s->code_size += code_size;
		} 
		if (LT_VIDEO_MODE == 0){
			byte *colors = (byte*) &s->tga_sprite_data_offset[0];
			byte linepos = 0;
			byte *pixels = &LT_tile_tempdata2[frame*size*size];
			byte bg_color = pixels[0];
			byte block = (size>>3)+1;
			if (bg_color) bg_color = 16;
			s->frames[frame].compiled_code = &LT_sprite_data[LT_sprite_data_offset];
			if(frame == 0) LT_memset(colors,0,32);//Scanline colors
			//read 8 shifted tiles
			//if (size ==32) LT_sprite_data_offset = 16*1024;
			for (i = 0; i < 8; i++){//Read copies
				//Read 16x16 tile
				offset = 0;
				for (y = 0; y < size; y++){//Read lines
					byte col = 0;
					if (LT_SPRITE_MODE) LT_memset(spr_line,16,64+8+8);
					else LT_memset(spr_line,bg_color,64+8+8);
					_memcpy(&spr_line[8],&pixels[offset],size);
					offset+=size;
					linepos = 8-i;//Shift tile
					for (j = 0; j < block; j++){//3
						byte cmask = 0x80;
						byte color = 0;
						byte mask = 0;
						for (x = 0; x < 8; x++){
							byte bit = spr_line[linepos++];//colors 1,2
							if (bit != 16) mask += cmask;
							if (bit != 0){
								color += cmask;
								if (bit < 16) col = bit;
							}
							cmask = cmask >> 1;
						}
						
						if (LT_SPRITE_MODE)LT_sprite_data[LT_sprite_data_offset++] = mask;
						LT_sprite_data[LT_sprite_data_offset++] = color;
					}
					if(!frame) {
						if(colors[(size-1-y)>>1] == 0) colors[(size-1-y)>>1] = col;
					}
				}
				if (LT_sprite_data_offset > 65536-2048) LT_Error("Not enough RAM to allocate frames $",file);
			}
		}
		if (LT_VIDEO_MODE == 3){
			code_size = compile_tga_sprite(size,&LT_tile_tempdata2[frame*size*size],LT_sprite_data_offset);
			//s->frames[frame].compiled_code = &LT_sprite_data[LT_sprite_data_offset];
			s->tga_sprite_data_offset[frame] = LT_sprite_data_offset;//leave space for sprites bkg
			LT_sprite_data_offset += code_size;
			s->code_size += code_size;
			if ((LT_sprite_data_offset) > 65536-8192) LT_Error("Not enough RAM to allocate frames $",file);
		}
		if (LT_VIDEO_MODE == 2){
			code_size = compile_cga_sprite(size,&LT_tile_tempdata2[frame*size*size],LT_sprite_data_offset);
			s->tga_sprite_data_offset[frame] = LT_sprite_data_offset;//leave space for sprites bkg
			LT_sprite_data_offset += code_size;
			s->code_size += code_size;
			if ((LT_sprite_data_offset) > 65536-8192) LT_Error("Not enough RAM to allocate frames $",file);
		}
	}
	
	//IINIT SPRITE
	//Load animations
	_memcpy(s->animations,animations,64);
	//Check animations so that game does not crash
	for (frame = 0; frame < 64;frame++){
		if (s->animations[frame] > s->nframes-1) s->animations[frame] = 0;
	}
	//s->bkg_data set in init function (sys.c)
	s->width = size;
	s->height = size;
	
	s->init = 0;
	s->frame = 0;
	s->baseframe = 65;
	s->ground = 0;
	s->jump = 1;
	s->jump_frame = 35;
	s->climb = 0;
	s->animate = 0;
	s->anim_speed = 0;
	s->speed = 4;
	s->anim_counter = 0;
	s->speed_x = 8*16;
	s->speed_y = 8*16;
	s->s_x = 0;
	s->s_y = 0;
	s->id_pos = 0;
	s->s_delete = 0;
	s->fixed_sprite_number = 0;
	s->mspeed_x = 1;
	s->mspeed_y = 1;
	s->size = s->height>>3;
	s->siz = s->height>>2;
	if (LT_VIDEO_MODE == 0) s->siz = s->size + 1;
	if (LT_VIDEO_MODE == 1) s->siz = (s->height>>2) + 1;
	if (LT_VIDEO_MODE == 2){
	}
	if (LT_VIDEO_MODE == 3){
		if (size == 8) {s->size = 12>>2;s->siz = 8>>2;}
		if (size == 16) {s->size = 20>>2;s->siz = 16>>2;}
		if (size == 32) {s->size = 36>>2;s->siz = 32>>2;}
	}
	s->next_scanline = LT_VRAM_Logical_Width - s->siz;
	s->get_item = 0;
	s->mode = 0;
	s->ai_stack = 0;
}

void LT_Clone_Sprite(int sprite_number_c,int sprite_number){
	SPRITE *c = &sprite[sprite_number_c];
	SPRITE *s = &sprite[sprite_number];
	int j;
	c->ega_size = s->ega_size;
	c->nframes = s->nframes;
	c->width = s->width;
	c->height = s->height;
	for(j=0;j<c->nframes;j++) {
		c->frames[j].compiled_code = s->frames[j].compiled_code;
		c->frames[j] = s->frames[j];
	}
	
	//sprite[sprite_number_c].bkg_data  //allocated in lt_sys
	_memcpy(c->animations,s->animations,32);
	c->init = 0;
	c->frame = 0;
	c->baseframe = 65;
	c->ground = 0;
	c->jump = 2;
	c->jump_frame = 0;
	c->climb = 0;
	c->animate = 0;
	c->anim_speed = 0;
	c->speed = s->speed;
	c->anim_counter = 0;
	c->mspeed_x = 1;
	c->mspeed_y = 1;
	c->speed_x = 8*16;
	c->speed_y = 8*26;
	c->s_x = 0;
	c->s_y = 0;
	c->id_pos = 0;
	c->s_delete = 0;
	c->fixed_sprite_number = 0;
	c->size = s->size;
	c->siz = s->siz;
	c->next_scanline = LT_VRAM_Logical_Width - c->siz;
	if (LT_VIDEO_MODE > 1) {
		int frame = 0;
		for (frame = 0; frame < s->nframes; frame++)
			c->tga_sprite_data_offset[frame] = s->tga_sprite_data_offset[frame];
	}
	if (LT_VIDEO_MODE == 0) {
		memcpy(c->tga_sprite_data_offset,s->tga_sprite_data_offset,32);
	}
	c->get_item = 0;
	c->mode = 0;
	c->ai_stack = 0;
}

void LT_Init_Sprite(int sprite_number,int x,int y){
	SPRITE *s = &sprite[sprite_number];
	LT_Sprite_Stack_Table[LT_Sprite_Stack] = sprite_number;
	s->pos_x = x;
	s->pos_y = y;
	s->ai_stack = LT_Sprite_Stack;
	s->init = 0;
	LT_Sprite_Stack++;
	LT_Player_Dir = 3;
	s->jump = 1;
	s->jump_frame = 35;
}

void LT_Add_Sprite(int sprite_number,word x, word y) {
	sprite[sprite_number].pos_x = x;
	sprite[sprite_number].pos_y = y;
}

void LT_Reset_Sprite_Stack(){
	int i;
	LT_Sprite_Stack = 0;
	for (i = 0; i<33; i++) LT_Sprite_Stack_Table[i] = 0;
	LT_AI_Sprite[0] = 0;
	LT_memset(sprite_id_table,0,19*256);
}

void LT_Set_Sprite_Animation(int sprite_number, byte anim){
	SPRITE *s = &sprite[sprite_number];
	if (s->baseframe != anim<<3){//If changed animation
		s->baseframe = anim<<3;
		s->anim_counter = anim<<3;
		s->animate = 1;
		if (s->baseframe > 64) s->baseframe = 0;
	}
}

void LT_Set_Sprite_Animation_Speed(int sprite_number, byte speed){
	SPRITE *s = &sprite[sprite_number];
	if(LT_VIDEO_MODE == 2) speed = speed>>1;//If CGA, sprites are updates at 30 fps, not 60
	s->speed = speed;
}

void LT_Sprite_Stop_Animation(int sprite_number){
	if (sprite[sprite_number].animate == 1){
		sprite[sprite_number].animate = 0; 
		sprite[sprite_number].baseframe = 65;
	}
}

byte CMask[] = {0x11,0x22,0x44,0x88};
void run_compiled_sprite(word XPos, word YPos, char *Sprite){
	asm{
		push si
		push ds
	
		mov ax, LT_VRAM_Logical_Width //width
		mul word ptr [YPos] // height in bytes
		mov si, [XPos]
		mov bx, si
		mov	cl,2
		shr	si,cl
		add si,ax
		add si, 128         // (Xpos / 4) + 128 ==> starting pos
		and bx, 3
		mov ah,byte ptr CMask[bx]
		mov dx, 03c4h
		mov al, 02h
		out dx, ax
		inc dx              //ready to send out other masks as bytes
		mov al, ah
	
		mov bx, 0a000h
		mov ds, bx          // We do this so the compiled shape won't need  segment overrides.
	
		call dword ptr [Sprite] //the business end of the routine
		
		pop ds
		pop si
	}
}

//colors are odd, but this runs very fast
void draw_ega_sprite(word XPos, word YPos, char *Sprite, word size, word offset, char *colors){
	//draw EGA sprite and destroy bkg
	char *bitmap = Sprite;
	word screen_offset = (YPos<<5)+(YPos<<3)+(YPos<<2)+(XPos>>3);
	byte *col = colors;
	
	size /= 2;
	offset+=0;
	asm{
		push ds
		push di
		push si
		
		lds	si,[bitmap]
		mov	ax,0A000h
		mov	es,ax						
		mov	di,screen_offset	//ES:DI destination vram
		
		mov	cx,size	//scanlines
		mov dx,0x03CE
		mov ax,0x0005	//write mode 0
		out dx,ax
		
		/*
		mov dx,0x03C4	//dx = indexregister
		mov ax,0x0F02	//write planes
		out dx,ax*/
		
		//Set logical op
		/*mov dx,03CEh
		mov ax,0803h	//AND op, no rotation
		out dx,ax*/
		
		//Prepare mask register to receive commands from al
		mov	dx,0x03CE	  
		mov al,0x08
		out	dx,al
		inc dx
	}
	
	asm cmp cx,4
	asm jz _drawsprit_8
	asm cmp cx,8
	asm jz _drawsprit_16
	asm cmp cx,16
	asm jnz _skip
	asm jmp _drawsprit_32
	_skip:
	asm jmp _end_sprite
	
	_drawsprit_8:
	asm{
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		mov	al,es:[di]	//Get 8 pixels latch from vram position
		lodsb			//al <- ds:[si]	//Get sprite mask; inc si     		
		out	dx,al		//Set mask
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		add di,44-2		//Next scanline
		
		
		mov	al,es:[di]
		lodsb		
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		add di,44-2		//Next scanline
		
		loop	_drawsprit_8
		
		jmp _end_sprite
	}
	
	_drawsprit_16:
	asm{
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		//Copy sprite
		mov	al,es:[di]	//Get 8 pixels latch from vram position
		lodsb			//al <- ds:[si]	//Get sprite mask; inc si
		out dx,al		//Set mask
		movsb			//combine mask with latch, and write sprite data, from ram DS:SI to vram ES:DI; inc si; inc di
		
		mov	al,es:[di]
		lodsb
		out dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out dx,al
		movsb
		
		add di,44-3				//Next scanline
		
		
		mov	al,es:[di]
		lodsb
		out dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out dx,al
		movsb
		
		add di,44-3				//Next scanline
		
		loop	_drawsprit_16
		
		jmp _end_sprite
	}
	
	_drawsprit_32:
	asm{
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		mov	al,es:[di]	//Get 8 pixels latch from vram position
		lodsb			//al <- ds:[si]	//Get sprite mask; inc si     		
		out	dx,al		//Set mask
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		add di,44-5				//Next scanline
		
		
		mov	al,es:[di]
		lodsb				
		out	dx,al	
		movsb		
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		mov	al,es:[di]
		lodsb
		out	dx,al
		movsb
		
		add di,44-5				//Next scanline
		
		dec cx
		jz 	_end_sprite
		jmp _drawsprit_32
	}
	
	_end_sprite:
	asm{
		pop si
		pop di
		pop ds
		
		
		//reset logical op
		/*mov dx,03CEh
		mov ax,0003h	//NONE
		out dx,ax*/
	}
}

void draw_ega_sprite_fast(word XPos, word YPos, char *Sprite, word size, char *colors){
	//draw EGA sprite and destroy bkg
	char *bitmap = Sprite;
	word screen_offset = (YPos<<5)+(YPos<<3)+(YPos<<2)+(XPos>>3);
	byte *col = colors;
	
	size /= 2;
	asm{
		push ds
		push di
		push si
		
		lds	si,[bitmap]
		mov	ax,0A000h
		mov	es,ax						
		mov	di,screen_offset	//ES:DI destination vram
		//Test 16x16 sprite
		mov	cx,size			//scanlines
		mov dx,0x03CE
		mov ax,0x0005		//write mode 0
		out dx,ax
		mov	dx,0x03CE	  //Set mask register
		mov al,0x08
		out	dx,al
		inc dx
	}
	
	asm cmp cx,4
	asm jz _drawsprit_8
	
	asm cmp cx,8
	asm jnz _skip0
	asm jmp _drawsprit_16
	
	_skip0:
	asm cmp cx,16
	asm jnz _skip1
	asm jmp _drawsprit_32
	
	_skip1:
	asm cmp cx,32
	asm jnz _skip2
	asm jmp _drawsprit_64
	
	_skip2:
	asm jmp _end_sprite
	
	_drawsprit_8:
	asm{
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		movsb; movsb	//Paste sprite data DS:SI to ES:DI; inc di
		add di,44-2		//Next scanline
		
		movsb; movsb			//Paste sprite data DS:SI to ES:DI; inc di
		add di,44-2				//Next scanline
		
		loop	_drawsprit_8
		
		jmp _end_sprite
	}
	
	_drawsprit_16:
	asm{
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		movsb
		movsb
		add di,44-3				//Next scanline
		
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		movsb
		movsb
		add di,44-3				//Next scanline
		
		loop	_drawsprit_16
		jmp _end_sprite
	}
	
	_drawsprit_32:
	asm{
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		movsb
		movsb
		movsb
		movsb

		add di,44-5				//Next scanline
		
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		movsb
		movsb
		movsb
		movsb

		add di,44-5				//Next scanline
		
		loop	_drawsprit_32
		jmp _end_sprite
	}
	
	_drawsprit_64:
	asm {
		//get color
		les	bx,col
		add	bx,cx
		mov ah,es:[bx-1]
		//Set/reset register
		dec dx
		xor al,al
		out dx,ax		//color + 00: set reset reg
		inc al; not ah
		out dx,ax		//color inv + 01: enable set reset
		//set regs for vram write
		mov al,0x08
		out	dx,al
		inc dx
		mov	ax,0A000h
		mov	es,ax
		
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		movsb
		movsb
		movsb
		movsb
		movsb
		movsb
		movsb
		movsb
		add di,44-9				//Next scanline
		
		movsb			//Paste sprite data DS:SI to ES:DI; inc di
		movsb
		movsb
		movsb
		movsb
		movsb
		movsb
		movsb
		movsb
		add di,44-9				//Next scanline
		
		loop	_drawsprit_64
	}
	
	/*asm mov ax,16
	_loop64:
	asm{
		mov cx,9
		rep movsb
		add di,44-9
		mov cx,9
		rep movsb
		add di,44-9
		mov cx,9
		rep movsb
		add di,44-9
		mov cx,9
		rep movsb
		add di,44-9
		
		dec ax
		jnz _loop64
	}*/
	
	_end_sprite:
	asm{
		pop si
		pop di
		pop ds
	}
}

void run_compiled_tga_sprite(word XPos, word YPos, char *Sprite){
	word y = (YPos>>2);
	word screen_offset = (y<<7)+(y<<5)+(XPos>>1);
	asm{
		push si
		push ds

		mov 	ax,0B800h
		mov 	es,ax						
		mov		di,screen_offset	//es:di destination vram				
	
		//mov bx, 0a000h
		//mov ds, bx          // We do this so the compiled shape won't need  segment overrides.
	
		mov		bx,YPos
		call dword ptr [Sprite] //the business end of the routine
		
		pop ds
		pop si
	}

}

void run_compiled_cga_sprite(word XPos, word YPos, char *Sprite){
	word s_offset;
	XPos = XPos>>2; YPos = YPos>>1;
	YPos = (YPos<<6) + (YPos<<4);
	s_offset = YPos+XPos;
	//Sprite = &SPR[0];
	asm{
		push si
		push ds
		
		
		mov 	ax,0B800h
		mov 	es,ax
		mov		di,s_offset			//es:di SCREEN
		
		mov		bx,YPos
		call dword ptr [Sprite]		//the business end of the routine
		/*
		
		and	di,8191; //fix offset
		add	di,8192-0//draw line (empty, jump line)
		inc	di; mov ax,0x0000; stosw; inc di; //Draw line
		sub di,8192-76;//jump line
		
		and		di,8191;//Fix offset
		mov ax,0x1F80; stosw; mov ax,0x0214; stosw;  //Draw line
		add	di,8192-4;		//jump bank-bytes
		mov ax,0x9F0A; stosw; mov ax,0x5015; stosw;  //Draw line
		sub di,8192-(80-4);	//jump bank-bytes-line
		
		and		di,8191;
		mov ax,0x5F22; stosw; mov ax,0x5415; stosw; add di,8192-4;
		mov ax,0x9F0A; stosw; mov ax,0x54C5; stosw; sub di,8192-76;
		
		and		di,8191;
		mov ax,0x5F22; stosw; mov ax,0x54A1; stosw; add di,8192-4;
		mov ax,0x9F0A; stosw; mov ax,0x00E8; stosw; sub di,8192-76;
		
		and		di,8191;
		mov ax,0x5F22; stosw; mov ax,0x80AA; stosw; add di,8192-4;
		mov ax,0x9F0A; stosw; mov ax,0xD8EA; stosw; sub di,8192-76;
		
		and		di,8191;
		mov ax,0x5F22; stosw; mov ax,0xA0AA; stosw; add di,8192-4;
		mov ax,0x9F0A; stosw; mov ax,0x88EA; stosw; sub di,8192-76;
		
		and		di,8191;
		mov ax,0x0080; stosw; mov ax,0x0200; stosw; add di,8192-4;
		mov ax,0x5401; stosw; mov ax,0x5005; stosw; sub di,8192-76;
		
		and		di,8191;
		mov ax,0x5505; stosw; mov ax,0x5415; stosw; add di,8192-4;
		mov ax,0x0000; stosw; mov ax,0x0000; stosw;
		*/
		
		pop ds
		pop si
		
	}
}


int LT_Sprite_Stack_Table_Pos0 = 0;
extern word pageflip[];
extern word gfx_draw_page;
//Draw restoring bkg, slower
extern unsigned char Enemies;
//If pos -2 (frame-2), then Clean bkg at pos-2 in draw page (for all sprites)
//If pos -1 (frame-1), pick bkg at pos -1 from the clean draw page.
//paste at current pos on draw page (if first paste, only do this)

void (*LT_Draw_Sprites)(void);
void (*LT_Draw_Sprites_Fast)(void);
void LT_Draw_Sprites_EGA_VGA(){
	int sprite_number;
	int page_y = pageflip[gfx_draw_page&1];
	
	LT_VGA_Enable_4Planes();

	//RESTORE SPRITE BKG's
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int lx = s->last_last_x;
		int ly = s->last_last_y+page_y;
		//ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X-2)&&(x < (SCR_X+306))){
			if (s->init == 2){
			word bkg_data = s->bkg_data;
			word screen_offset0;
			word size = s->size;
			word siz = s->siz;
			word next_scanline = s->next_scanline;
			//Paste at pos -2
			if (!LT_VIDEO_MODE)screen_offset0 = (ly<<5)+(ly<<3)+(ly<<2)+(lx>>sprite_size); 
			else screen_offset0 = (ly<<6)+(ly<<4)+(ly<<3)+(lx>>sprite_size); 
			///Clean pos 0 bkg
			asm{
				push ds;push di;push si;
				
				mov dx,SC_INDEX //dx = indexregister
				mov ax,0F02h	//INDEX = MASK MAP, 
				out dx,ax 		//write all the bitplanes.
				mov dx,GC_INDEX //dx = indexregister
				mov ax,008h		
				out dx,ax 		
				
				mov ax,0A000h; mov es,ax; mov di,screen_offset0; //es:di destination vram				
				mov	ds,ax; mov si,bkg_data	//ds:si source vram				
				mov 	ax,size
				mov		bx,next_scanline
				mov		dx,siz
			}
			bkg_scanline1:	
			asm{
				mov cx,dx; rep movsb; add di,bx; // copy bytes from ds:si to es:di
				mov cx,dx; rep movsb; add di,bx;
				mov cx,dx; rep movsb; add di,bx;
				mov cx,dx; rep movsb; add di,bx;
				mov cx,dx; rep movsb; add di,bx;
				mov cx,dx; rep movsb; add di,bx;
				mov cx,dx; rep movsb; add di,bx;
				mov cx,dx; rep movsb; add di,bx;
				dec 	ax
				jnz		bkg_scanline1
			}
			
			asm mov dx,GC_INDEX +1 //dx = indexregister
			asm mov ax,00ffh		
			asm out dx,ax 
			
			asm pop si;asm pop di;asm pop ds;
			}
		}
		if (s->s_delete == 2)s->init = 0;
	}
	LT_VGA_Return_4Planes();

	//GET NEW BKG's
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int lx = s->last_x;
		int ly = s->last_y+page_y;
		
		//ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X+4)&&(x < (SCR_X+304-4))){
			word bkg_data = s->bkg_data;
			word screen_offset1;
			word size = s->size;
			word siz = s->siz;
			word next_scanline = s->next_scanline;
			
			//GOT ITEM? REPLACE BKG BEFORE Getting the new one
			if (s->get_item == 1){
				LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
				s->get_item = 0;
			}
			
			//Get pos -1
			if (!LT_VIDEO_MODE)screen_offset1 = (ly<<5)+(ly<<3)+(ly<<2)+(lx>>sprite_size); 
			else screen_offset1 = (ly<<6)+(ly<<4)+(ly<<3)+(lx>>sprite_size); 
			
			if (s->init){
			//Copy bkg chunk to a reserved VRAM part, before destroying it
			asm{
				push ds
				push di
				push si
				
				mov dx,SC_INDEX //dx = indexregister
				mov ax,0F02h	//INDEX = MASK MAP, 
				out dx,ax 		//write all the bitplanes.
				mov dx,GC_INDEX //dx = indexregister
				mov ax,008h		
				out dx,ax 
				
				mov ax,0A000h; mov ds,ax; mov si,screen_offset1;	//ds:si source vram			
				mov es,ax; mov di,bkg_data	//es:di destination
				
				mov		ax,size				//Scanlines
				mov		bx,next_scanline
				mov		dx,siz
			}
			bkg_scanline2:	
			asm{
				mov cx,dx;rep movsb;add si,bx;// copy bytes from ds:si to es:di
				mov cx,dx;rep movsb;add si,bx;
				mov cx,dx;rep movsb;add si,bx;
				mov cx,dx;rep movsb;add si,bx;
				mov cx,dx;rep movsb;add si,bx;
				mov cx,dx;rep movsb;add si,bx;
				mov cx,dx;rep movsb;add si,bx;
				mov cx,dx;rep movsb;add si,bx;
				dec 	ax
				jnz		bkg_scanline2	
				
				mov dx,GC_INDEX +1 //dx = indexregister
				mov ax,00ffh		
				out dx,ax 
				
				pop si;pop di;pop ds
			}
			
			s->last_last_x = s->last_x;
			s->last_last_y = s->last_y;
			}
		}
	}
	if (LT_EGA_SPRITES_TRANSLUCENT){
		asm mov dx,03C4h	//dx = indexregister
		asm mov ax,0702h	//INDEX = MASK MAP, 
		asm out dx,ax 		//write planes 1, 2, 3.
	}
	//AT LEAST, DRAW SPRITES
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		int lx = s->last_x;
		int ly = s->last_y+page_y;
		word screen_offset1;
		
		if (!LT_VIDEO_MODE)screen_offset1 = (ly<<5)+(ly<<3)+(ly<<2)+(lx>>sprite_size); 
		else screen_offset1 = (ly<<6)+(ly<<4)+(ly<<3)+(lx>>sprite_size); 
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > SCR_X+4)&&(x < (SCR_X+304-4))){
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			//draw sprite and destroy bkg
			if (s->s_delete != 2){
				if (LT_VIDEO_MODE == 0){
					word panning = s->ega_size[x&7];
					draw_ega_sprite(x,y+page_y,s->frames[s->frame].compiled_code+panning,s->width,screen_offset1,(byte*)s->tga_sprite_data_offset);
				} 
				if (LT_VIDEO_MODE == 1){
					run_compiled_sprite(x,y+page_y,s->frames[s->frame].compiled_code);
				}
			} else s->s_delete = 0;
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
			if (s->init != 2) s->init++;
		} else {
			if (s->s_delete == 1) {
				s->s_delete = 2;
			}
		}
		if (sprite_number){//remove from stack
			if ((x < (SCR_X-40))||(x > SCR_X+360)){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				_memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
				sprite_id_table[s->id_pos] = 0;
				s->id_pos = 0;
			}
		}
		s->last_x = x;
		s->last_y = y;
	}

	if (LT_EGA_SPRITES_TRANSLUCENT){
		asm mov dx,03C4h	//dx = indexregister
		asm mov ax,0F02h	//INDEX = MASK MAP, 
		asm out dx,ax 		//write plane 1.
	}
}

//Draw without restoring bkg, use over solid color backgrounds
void LT_Draw_Sprites_Fast_EGA_VGA(){
	int sprite_number;

	if (LT_EGA_SPRITES_TRANSLUCENT){
		asm mov dx,03C4h	//dx = indexregister
		asm mov ax,0702h	//INDEX = MASK MAP, 
		asm out dx,ax 		//write plane 1.
	}

	//DRAW SPRITE
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y+pageflip[gfx_draw_page&1];
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x+8 > SCR_X)&&(x-8 < (SCR_X+320-s->width))&&(s->pos_y+8 >= SCR_Y)&&(s->pos_y-8 < (SCR_Y+200-s->width))){
			
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			
			//draw sprite and destroy bkg
			if (LT_VIDEO_MODE == 0){
				word panning = 0;
				if(s->width == 64) panning = s->ega_size[x&7];
				else panning = s->ega_size[x&7]>>1;//Sprite size is half, no mask used
				draw_ega_sprite_fast(x,y,s->frames[s->frame].compiled_code+panning,s->width,(byte*)s->tga_sprite_data_offset);
			}
			if (LT_VIDEO_MODE == 1){
				run_compiled_sprite(x,y,s->frames[s->frame].compiled_code);
			}
			//run_compiled_sprite2(x,y);
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		}
		else if (s->s_delete == 1) {
			s->s_delete = 0;
		}
		if (sprite_number){
			if ((x < SCR_X-40)||(x > SCR_X+360)){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				_memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
				sprite_id_table[s->id_pos] = 0;
				s->id_pos = 0;
			}
		}
		s->last_x = x;
		s->last_y = y;
	}
	
	if (LT_EGA_SPRITES_TRANSLUCENT){
		asm mov dx,03C4h	//dx = indexregister
		asm mov ax,0F02h	//INDEX = MASK MAP, 
		asm out dx,ax 		//write plane 1.
	}
}

extern int TGA_SCR_X;
extern int TGA_SCR_Y;
extern int TANDY_SCROLL_X;
extern int TANDY_SCROLL_Y;
void LT_Draw_Sprites_Tandy(){
	int sprite_number;
	
	LT_TGA_MapPage(tga_page+1);
	//RESTORE SPRITE BKG
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		int lx = s->last_last_x>>1;
		int ly = s->last_last_y>>2;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > TGA_SCR_X)&&(x < (TGA_SCR_X+304))&&(y>TGA_SCR_Y)&&(y<(TGA_SCR_Y+174))){
		if (s->init == 2){
		word boffset = tandy_sprite_bkg_table[LT_Sprite_Stack_Table[sprite_number]];
		byte *bkg_data = &tandy_bkg_data[boffset];
		word screen_offset0;
		word size = s->size;
		word siz = s->siz;
		word next_scanline = siz<<1;//s->next_scanline;
		
		//Paste at pos -2
		screen_offset0 = (ly<<7)+(ly<<5)+(lx); 
		///Clean pos 0 bkg
		asm{
			push ds
			push di
			push si
			
			mov 	ax,0B800h
			mov 	es,ax						
			mov		di,screen_offset0	//es:di destination vram				
			
			lds		si,bkg_data			//ds:si source ram				
			mov 	ax,size
			mov		bx,next_scanline
			mov		dx,siz
		}
		bkg_scanline1:	
		asm{
			and		di,8191
			mov	cx,dx; rep movsw; add di,8192; sub di,bx;
			mov	cx,dx; rep movsw; add di,8192; sub di,bx;
			mov	cx,dx; rep movsw; add di,8192; sub di,bx;
			
			mov		cx,dx
			rep 	movsw
			
			sub		di,(8192*3)-160
			sub		di,bx
			
			dec 	ax
			jnz		bkg_scanline1
		}
		
		asm pop si
		asm pop di
		asm pop ds
		}
		}
		if (s->s_delete == 2)s->init = 0;
	}
	
	//GET NEW BKG
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		int lx = s->last_x>>1;
		int ly = s->last_y>>2;
		
		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
		}
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > TGA_SCR_X)&&(x < (TGA_SCR_X+304))&&(y>TGA_SCR_Y)&&(y<(TGA_SCR_Y+174))){
			word boffset = tandy_sprite_bkg_table[LT_Sprite_Stack_Table[sprite_number]];
			byte *bkg_data = &tandy_bkg_data[boffset];
			word screen_offset1;
			word size = s->size;
			word siz = s->siz;
			word next_scanline = siz<<1;//s->next_scanline;

			//Get pos -1
			screen_offset1 = (ly<<7)+(ly<<5)+(lx); 
			if (s->init){
			//Copy bkg chunk to a reserved VRAM part, before destroying it
			asm{
				push ds
				push di
				push si
				
				mov 	ax,0B800h
				mov 	ds,ax						
				mov		si,screen_offset1	//ds:si source vram			
				
				les		di,bkg_data			//es:di destination
				
				mov		ax,size				//Scanlines
				mov		bx,next_scanline
				mov		dx,siz
			}
			bkg_scanline2:	
			asm{
				and		si,8191
				mov	cx,dx; rep movsw; add si,8192; sub	si,bx;
				mov	cx,dx; rep movsw; add si,8192; sub	si,bx;
				mov	cx,dx; rep movsw; add si,8192; sub	si,bx;
		
				mov		cx,dx
				rep 	movsw
				sub		si,(8192*3)-160
				sub		si,bx
				
				dec 	ax
				jnz		bkg_scanline2
				
				pop si
				pop di
				pop ds
			}
			
			s->last_last_x = s->last_x;
			s->last_last_y = s->last_y;
			}
		}
	}
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		
		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
		}
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > TGA_SCR_X+12)&&(x < (TGA_SCR_X+290))&&(y>TGA_SCR_Y+16)&&(y<(TGA_SCR_Y+168))){
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			//draw sprite and destroy bkg
			if (s->s_delete != 2){
				run_compiled_tga_sprite(x,y,&LT_sprite_data[s->tga_sprite_data_offset[s->frame]]);
			} else s->s_delete = 0;
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
			if (s->init != 2) s->init++;
		} else {
			if (s->s_delete == 1) {
				s->s_delete = 2;
			}
		}
		if (sprite_number){//remove from stack
			if ((x < (TGA_SCR_X-60))||(x > (TGA_SCR_X+370))){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				_memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
				sprite_id_table[s->id_pos] = 0;
				s->id_pos = 0;
				s->init = 0;
			}
		}
		s->last_x = x;
		s->last_y = y;
	}
	LT_TGA_MapPage(tga_page);
}

void LT_Draw_Sprites_Fast_Tandy(){
	int sprite_number;
	LT_TGA_MapPage(tga_page+1);
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x+8 > TGA_SCR_X)&&(x-8 < (TGA_SCR_X+320-s->width))&&(s->pos_y+8 >= TGA_SCR_Y)&&(s->pos_y-8 < (TGA_SCR_Y+200-s->width))){
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			//draw sprite and destroy bkg
			if (s->s_delete != 2){
				run_compiled_tga_sprite(x,y,&LT_sprite_data[s->tga_sprite_data_offset[s->frame]]);
			} else s->s_delete = 0;
			s->last_x = x;
			s->last_y = y;
			s->s_delete = 1;
		} else {
			if (s->s_delete == 1) {
				s->s_delete = 2;
			}
		}
		if (sprite_number){//remove from stack
			if ((x < (TGA_SCR_X-60))||(x > (TGA_SCR_X+370))){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				_memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
				sprite_id_table[s->id_pos] = 0;
				s->id_pos = 0;
				s->init = 0;
			}
		}
		s->last_x = x;
		s->last_y = y;
	}
}

void LT_Draw_Sprites_CGA(){
	int sprite_number;
	//RESTORE SPRITE BKG
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){	
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		int lx = s->last_x;
		int ly = s->last_y;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > TGA_SCR_X+8)&&(x < (TGA_SCR_X+312))&&(y>TGA_SCR_Y+8)&&(y<(TGA_SCR_Y+192))){
		if (s->init){
		word boffset = tandy_sprite_bkg_table[LT_Sprite_Stack_Table[sprite_number]];
		byte *bkg_data = &tandy_bkg_data[boffset];
		word screen_offset;
		word next_bank = 8192 - (80 - s->siz);
		word next_scanline = 8192 - s->siz;
		word size = s->size;
		
		//Clean BKG
		lx = lx>>2; ly = ly>>1;
		ly = (ly<<6) + (ly<<4);
		screen_offset = ly+lx; 
		asm{//Paste BKG
			push ax
			push ds
			push si
			
			mov 	ax,0B800h
			mov 	es,ax
			mov		di,screen_offset	//es:di SCREEN
			lds		si,[bkg_data]		//ds:si Sprite bkg
			
			mov		cx,size
			mov 	ax,8191
			mov 	bx,next_scanline
		}
		
		asm cmp cx,1
		asm jz paste_bkg_8
		
		asm cmp cx,2
		asm jnz _skip0
		asm jmp paste_bkg_16
		
		_skip0:
		asm cmp cx,4
		asm jnz _skip1
		asm jmp paste_bkg_32
		
		_skip1:
		asm jmp _end_sprite_CGA0
		
		paste_bkg_8:
		asm{
			mov cx,next_bank
			and	di,ax; movsw; add di,bx; movsw; sub di,cx;
			and	di,ax; movsw; add di,bx; movsw; sub di,cx;
			and	di,ax; movsw; add di,bx; movsw; sub di,cx;
			and	di,ax; movsw; add di,bx; movsw; sub di,cx;
			
			jmp	_end_sprite_CGA0
		}
		
		paste_bkg_16:
		asm{	
			mov cx,next_bank
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			and	di,ax; movsw; movsw; add di,bx; movsw; movsw; sub di,cx;
			
			jmp _end_sprite_CGA0
		}
		
		paste_bkg_32:
		asm{	
			mov cx,next_bank
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
			and	di,ax; movsw;movsw;movsw;movsw; add	di,bx; movsw;movsw;movsw;movsw; sub di,cx;
		}
		
		_end_sprite_CGA0:
		asm{
			pop es
			pop ds
			pop si
		}	
		
		}
		}

		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
		}
		if ((x > TGA_SCR_X+8)&&(x < (TGA_SCR_X+312))&&(y>TGA_SCR_Y+8)&&(y<(TGA_SCR_Y+192))){
			word boffset = tandy_sprite_bkg_table[LT_Sprite_Stack_Table[sprite_number]];
			byte *bkg_data = &tandy_bkg_data[boffset];
			word next_bank = 8192 - (80 - s->siz);
			word next_scanline = 8192 - s->siz;
			word size = s->size;
			word screen_offset;
			//Copy BKG
			x = x>>2; y = y>>1;
			y = (y<<6) + (y<<4);
			screen_offset = y+x; 
			//Copy bkg chunk to a reserved VRAM part, before destroying it
			asm{//Copy BKG
				push ax
				push ds
				push si
				
				mov 	ax,0B800h
				mov 	ds,ax
				mov		si,screen_offset			//ds:si SCREEN
				les		di,[bkg_data]		//es:di Sprite bkg
				
				mov		cx,size
				mov 	ax,8191
				mov		bx,next_scanline
			}
			asm cmp cx,1
			asm jz copy_bkg_8
			
			asm cmp cx,2
			asm jnz _skip2
			asm jmp copy_bkg_16
			
			_skip2:
			asm cmp cx,4
			asm jnz _skip3
			asm jmp copy_bkg_32
			
			_skip3:
			asm jmp _end_sprite_CGA1
			
			copy_bkg_8:
			asm{	
				mov cx,next_bank
				and	si,ax; movsw; add si,bx; movsw; sub si,cx;
				and	si,ax; movsw; add si,bx; movsw; sub si,cx;
				and	si,ax; movsw; add si,bx; movsw; sub si,cx;
				and	si,ax; movsw; add si,bx; movsw; sub si,cx;
				
				jmp	_end_sprite_CGA1
			}
			
			copy_bkg_16:
			asm{
				mov cx,next_bank
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				and	si,ax; movsw; movsw; add si,bx; movsw; movsw; sub si,cx;
				
				jmp _end_sprite_CGA1
			}
			
			copy_bkg_32:
			asm{
				mov cx,next_bank
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
				and	si,ax; movsw;movsw;movsw;movsw; add	si,bx; movsw;movsw;movsw;movsw; sub si,cx;
			}
			_end_sprite_CGA1:
			asm{
				pop es
				pop ds
				pop si
			}
		}
		
		//DRAW
		x = s->pos_x;
		y = s->pos_y;
		
		
		//GOT ITEM? REPLACE BKG BEFORE DRAWING NEXT SPRITE FRAME
		/*if (s->get_item == 1){
			LT_Edit_MapTile(s->tile_x,s->tile_y,s->ntile_item, s->col_item);
			s->get_item = 0;
		}*/
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x > TGA_SCR_X+16)&&(x < (TGA_SCR_X+303))&&(y>TGA_SCR_Y+16)&&(y<(TGA_SCR_Y+183))){
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			//draw sprite and destroy bkg
			run_compiled_cga_sprite(x,y,&LT_sprite_data[s->tga_sprite_data_offset[s->frame]]);
			s->last_x = x;
			s->last_y = y;
			s->init = 1;
		} else {
			s->init = 0;
		}
		if (sprite_number){//remove from stack
			if ((x < (TGA_SCR_X-60))||(x > (TGA_SCR_X+370))){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				_memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
				sprite_id_table[s->id_pos] = 0;
				s->id_pos = 0;
				s->init = 0;
			}
		}
		s->last_x = x;
		s->last_y = y;
	}
}

void LT_Draw_Sprites_Fast_CGA(){
	int sprite_number;
	for (sprite_number = 0; sprite_number < LT_Sprite_Stack; sprite_number++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[sprite_number]];
		int x = s->pos_x;
		int y = s->pos_y;
		//DRAW THE SPRITE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((x+8 > TGA_SCR_X)&&(x-8 < (TGA_SCR_X+320-s->width))&&(s->pos_y+8 >= TGA_SCR_Y)&&(s->pos_y-8 < (TGA_SCR_Y+200-s->width))){
			//animation
			if (s->animate == 1){
				s->frame = s->animations[s->anim_counter];
				if (s->anim_speed > s->speed){
					s->anim_speed = 0;
					s->anim_counter++;
					if (s->anim_counter == s->baseframe+8) s->anim_counter = s->baseframe;
				}
				s->anim_speed++;
			}
			//draw sprite and destroy bkg
			run_compiled_cga_sprite(x,y,&LT_sprite_data[s->tga_sprite_data_offset[s->frame]]);
		} 
		if (sprite_number){//remove from stack
			if ((x < (TGA_SCR_X-60))||(x > (TGA_SCR_X+370))){
				int spr = LT_Sprite_Stack_Table[sprite_number];
				LT_Active_AI_Sprites[s->fixed_sprite_number] = spr; 
				if (LT_Sprite_Stack>1) LT_Sprite_Stack--;
				_memcpy(&LT_Sprite_Stack_Table[sprite_number],&LT_Sprite_Stack_Table[sprite_number+1],8);
				sprite_id_table[s->id_pos] = 0;
				s->id_pos = 0;
				s->init = 0;
			}
		}
	}
}


void LT_Set_AI_Sprite(byte sprite_number, byte mode, word x, word y, int sx, int sy, word id_pos){
	//int i = 0;
	SPRITE *s = &sprite[sprite_number];
	s->pos_x = x<<4;
	s->pos_y = y<<4;
	s->mspeed_x = sx;
	s->mspeed_y = sy;
	if (s->mspeed_x!=0) {s->speed_x = 3; s->speed_y = 0;}
	if (s->mspeed_y!=0) {s->speed_x = 0; s->speed_y = 3;}
	s->mode = mode;
	s->ai_stack = LT_Sprite_Stack;
	s->id_pos = id_pos;
	s->init = 0;
}

void LT_Set_AI_Sprites(byte first_type, byte second_type, byte mode_0, byte mode_1){
	int i;
	LT_AI_Enabled = 1;
	LT_AI_Sprite[0] = first_type;
	LT_AI_Sprite[3] = second_type;
	for (i = 0; i < 3; i++) {
		LT_Clone_Sprite(first_type+i,first_type);
		sprite[first_type+i].fixed_sprite_number = i;
		sprite[first_type+i].mode = mode_0;
		LT_Active_AI_Sprites[i] = first_type+i;
	}
	if (first_type != 17){
		for (i = 0; i < 4; i++) {
			LT_Clone_Sprite(first_type+3+i,second_type);
			sprite[first_type+3+i].fixed_sprite_number = i+3;
			sprite[first_type+3+i].mode = mode_1;
			LT_Active_AI_Sprites[i+3] = first_type+3+i;
		}
	}
	
	//for (i = first_ai+1; i < number_ai; i++) LT_Clone_Sprite(i,first_ai);
}

void LT_move_player(int sprite_number){
	SPRITE *s = &sprite[sprite_number];
	byte slope_speed = 16; //A dirty way of making big slopes or long conveyor belts impossible to cross
	int tsize = 4;//16
	byte tilememsize = 0;
	byte col_x = 0;
	byte col_y = 0;
	int x,y,platform_y;
	byte size = s->width;
	byte siz = s->width -1;
	byte si = s->width>>1;
	/*
	#define SPR_TILP_NUM 0;	
	#define SPR_TILC_NUM 1;	
	#define SPR_COL_X	 2;
	#define SPR_COL_Y	 3;
	#define SPR_HIT		 4;
	#define SPR_JUMP	 5;
	#define SPR_ACT		 6;
	#define SPR_MOVE	 7;	//0 1 2 3 4 => Stop U D L R 
	#define SPR_GND		 8;
	*/

	int last_dir;
	if (TANDY_SCROLL_X || TANDY_SCROLL_Y) return;
	//GET TILE POS
	if (LT_VIDEO_MODE == 0) tilememsize = 5;
	if (LT_VIDEO_MODE == 1) tilememsize = 6;
	s->tile_x = (s->pos_x+si)>>tsize;
	s->tile_y = (s->pos_y+si)>>tsize;
	if (LT_VIDEO_MODE > 1) tile_number = (LT_map_data[(s->tile_y << 10) + (s->tile_x<<1)])>>2;
	else tile_number = (LT_map_data[(s->tile_y << 8) + s->tile_x]-TILE_VRAM)>>tilememsize;
	tilecol_number = LT_map_collision[(((s->pos_y+si)>>tsize) <<8) + ((s->pos_x+si)>>tsize)];
	tilecol_number_down = LT_map_collision[(((s->pos_y+size+1)>>tsize) <<8) + ((s->pos_x+si)>>tsize)];
	
	LT_Player_State[SPR_MOVE] = 0;
	LT_Player_State[SPR_GND] = 0;
	s->state = 0;
	//PREDEFINED GAME TYPES //77.824
	switch (LT_MODE){
	case 0:{//TOP
		LT_Player_State[SPR_GND] = s->ground = 1;
		if (LT_Keys[LT_UP]){	//UP
			LT_Player_State[SPR_MOVE] = 1;
			LT_Player_Dir = 0;
			col_y = 0;
			y = (s->pos_y+1)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-2)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+2)>>tsize)];
			if ((tile_number_VR == 1) || (tile_number_VR == 5)) col_y = tile_number_VR;
			if ((tile_number_VL == 1) || (tile_number_VL == 5)) col_y = tile_number_VL;
			if (col_y == 0) s->pos_y--;
		} else if (LT_Keys[LT_DOWN]){//DOWN
			LT_Player_State[SPR_MOVE] = 2;
			LT_Player_Dir = 1;
			col_y = 0;
			y = (s->pos_y+size-2)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-2)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+2)>>tsize)];
			if ((tile_number_VR == 1) || (tile_number_VR == 5)) col_y = tile_number_VR;
			if ((tile_number_VL == 1) || (tile_number_VL == 5)) col_y = tile_number_VL;
			if (col_y == 0) s->pos_y++;
		}
		
		if (LT_Keys[LT_LEFT]){	//LEFT
			LT_Player_State[SPR_MOVE] = 3;
			LT_Player_Dir = 2;
			s->state = 3;
			col_x = 0;
			x = (s->pos_x+1)>>tsize;
			tile_number_HU = LT_map_collision[(((s->pos_y+2)>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz-2)>>tsize) <<8) + x];	
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x--;
		} else if (LT_Keys[LT_RIGHT]){	//RIGHT
			LT_Player_State[SPR_MOVE] = 4;
			LT_Player_Dir = 3;
			s->state = 4;
			col_x = 0;
			x = (s->pos_x+size-2)>>tsize;
			tile_number_HU = LT_map_collision[(((s->pos_y+2)>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz-2)>>tsize) <<8) + x];
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x++;
		}
	break;}
	case 1:{//PLATFORM
		if ((s->ground == 1) && (LT_Jump_Release) && (LT_Keys[LT_JUMP])) {s->ground = 0; s->jump_frame = 0; s->jump = 1;}
		if (s->jump == 1){//JUMP
			col_y = 0;
			s->ground = 0;
			s->climb = 0;
			LT_Jump_Release = 0;
			if (s->jump_frame < 34){
				y = (s->pos_y-1)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-2)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+2)>>tsize)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1;
				if (col_y > 0) s->jump_frame = 40;
				//Short jumps
				if (!LT_Keys[LT_JUMP]){
					if (s->jump_frame ==  6) s->jump_frame = 24;
					if (s->jump_frame == 12) s->jump_frame = 24;
					if (s->jump_frame == 18) s->jump_frame = 24;
					LT_Jump_Release = 1;
				}
			} else {
				int platform_y = 1+(((s->pos_y+size)>>tsize)<<4);
				y = (s->pos_y+size)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-2)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+2)>>tsize)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1;  			
				if (tile_number_VR == 2) col_y = 1;  
				if (tile_number_VL == 2) col_y = 1;
				if (tile_number_VR == 3) col_y = 1;  
				if (tile_number_VL == 3) col_y = 1;
				if (tile_number_VR == 5) col_y = 5;  
				if (tile_number_VL == 5) col_y = 5;
				if ((col_y == 1)&&(s->pos_y+size > platform_y)) col_y = 0;
				if (col_y > 0) s->jump_frame = 68;
			}
			if (s->jump_frame < 67) {
				s->pos_y += LT_player_jump_pos[s->jump_frame];
				s->jump_frame++;
			} else {
				if (col_y == 0) {s->ground = 0; s->pos_y+=2;}
				if (col_y > 0) {
					s->ground = 1;
					if (s->pos_y&15)s->pos_y--;
					else {s->jump = 2; s->jump_frame = 0;}
				}
			}
		}
		
		if(LT_Keys[LT_UP]){//CLIMB UP
			if ((tilecol_number == 3) || (tilecol_number == 4) || (tilecol_number_down == 3)){
				LT_Player_Dir = 0;
				LT_Player_State[SPR_MOVE] = 1;
				s->ground = 1;
				s->jump = 2;
				s->climb = 1;
				col_y = 0;
				s->pos_x = ((s->pos_x+si)>>tsize)<<tsize;
				y = (s->pos_y-1)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-2)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+2)>>tsize)];
				if (tile_number_VR == 1) col_y = 1; 
				if (tile_number_VL == 1) col_y = 1; 
				if (tile_number_VR == 5) col_y = 5;  
				if (tile_number_VL == 5) col_y = 5;
				if (col_y == 0) s->pos_y--;
			}
		}else if(LT_Keys[LT_DOWN]){//CLIMB DOWN
			if ((tilecol_number == 3) || (tilecol_number == 4) || (tilecol_number_down == 3)){
				LT_Player_Dir = 1;
				LT_Player_State[SPR_MOVE] = 1;
				s->jump = 2;
				s->ground = 1;
				s->climb = 1;
				col_y = 0;
				s->pos_x = ((s->pos_x+si)>>tsize)<<tsize;
				y = (s->pos_y+size)>>tsize;
				tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-2)>>tsize)];
				tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+2)>>tsize)];
				if (tile_number_VR == 1) col_y = 1;
				if (tile_number_VL == 1) col_y = 1;
				if (tile_number_VR == 2) col_y = 1; 
				if (tile_number_VL == 2) col_y = 1;
				if (tile_number_VR == 5) col_y = 5;  
				if (tile_number_VL == 5) col_y = 5;
				if (col_y == 0) s->pos_y++;
				else s->climb = 0;
			}
		}
		if (s->jump == 2){
			if ((tilecol_number != 3) && (tilecol_number != 4)&& (tilecol_number_down != 3)){//DISABLE CLIMB
				s->climb = 0;
				s->jump = 1;
				s->jump_frame = 40;
				//LT_Player_Dir = 0;
				//LT_Player_State[SPR_MOVE] = 2;
			}
			//if (!LT_Keys[LT_JUMP]) LT_Jump_Release = 1;//THIS WAS OK but failed sometimes
		}
		if (!LT_Keys[LT_JUMP] && s->ground) LT_Jump_Release = 1;
		if (LT_Keys[LT_LEFT]){	//LEFT
			LT_Player_Dir = 2;
			if (s->ground) LT_Player_State[SPR_MOVE] = 3;
			if (s->climb) {LT_Player_State[SPR_MOVE] = 1; LT_Player_Dir = 0;}
			s->state = 3;
			col_x = 0;
			x = (s->pos_x+1)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];	
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x--;
		} else if (LT_Keys[LT_RIGHT]){	//RIGHT
			LT_Player_Dir = 3;
			if (s->ground) LT_Player_State[SPR_MOVE] = 4;
			if (s->climb) {LT_Player_State[SPR_MOVE] = 1; LT_Player_Dir = 0;}
			s->state = 4;
			col_x = 0;
			x = (s->pos_x+size-2)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if ((tile_number_HU == 1) || (tile_number_HU == 5)) col_x = tile_number_HU;
			if ((tile_number_HD == 1) || (tile_number_HD == 5)) col_x = tile_number_HD;
			if (col_x == 0) s->pos_x++;
		} 
		LT_Player_State[SPR_GND] = s->ground;
	
	break;}	
	case 2:{//TOP DOWN PHYSICS
		LT_Player_State[SPR_GND] = s->ground = 1;
		tile_number_VR = 0;
		tile_number_VL = 0;
		tile_number_HU = 0;
		tile_number_HD = 0;
		if (LT_Spr_speed_float == 4) LT_Spr_speed_float = 0;
		col_y = 0; col_x = 0;
		
		//UP
		if (LT_Keys[LT_UP]){ LT_Player_State[SPR_MOVE] = 1; if (s->speed_y>8) s->speed_y-=8;}
		//DOWN
		else if (LT_Keys[LT_DOWN]){ LT_Player_State[SPR_MOVE] = 2; if (s->speed_y < 8*32) s->speed_y+=8;}
		//LEFT
		if (LT_Keys[LT_LEFT]){ LT_Player_State[SPR_MOVE] = 3;s->state = 3; if (s->speed_x>8) s->speed_x-=8;}
		//RIGHT
		else if (LT_Keys[LT_RIGHT]){ LT_Player_State[SPR_MOVE] = 4;s->state = 4; if (s->speed_x <8*32) s->speed_x+=8;}
		
		//Tile physics
		if(LT_Spr_speed_float&1){
			if (LT_Slope_Timer < 8) slope_speed = 16;
			else slope_speed = 24;
			switch (tilecol_number){
				case 0://Flat tile, friction
					if (s->speed_y < 128) s->speed_y+=8;
					if (s->speed_y > 128) s->speed_y-=8;
					if (s->speed_x < 128) s->speed_x+=8;
					if (s->speed_x > 128) s->speed_x-=8;
					LT_Slope_Timer = 0;
				break;
				//breakable block
				case 5:  break;
				//UP
				case 6: if (s->speed_y>16) s->speed_y-=slope_speed; LT_Slope_Timer++; break;
				//DOWN
				case 7: if (s->speed_y<8*30) s->speed_y+=slope_speed; LT_Slope_Timer++; break;
				//LEFT
				case 8: if (s->speed_x>16) s->speed_x-=slope_speed; LT_Slope_Timer++; break;
				//RIGHT
				case 9: if (s->speed_x<8*30) s->speed_x+=slope_speed; LT_Slope_Timer++; break;
			} 
		}
		
		if (s->speed_x < 128){
			x = (s->pos_x)>>tsize;
			tile_number_HU = LT_map_collision[(((s->pos_y+3)>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz-3)>>tsize) <<8) + x];
			if ((tile_number_HU==1) || (tile_number_HD==1)) {s->speed_x = 8*21; col_x = 1;}
			if ((tile_number_HU==5) || (tile_number_HD==5)) {s->speed_x = 8*21; col_x = 5;}
		} else if (s->speed_x > 128){
			x = (s->pos_x+size)>>tsize;
			tile_number_HU = LT_map_collision[(((s->pos_y+3)>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz-3)>>tsize) <<8) + x];
			if ((tile_number_HU==1) || (tile_number_HD==1)) {s->speed_x = 8*11; col_x = 1;}
			if ((tile_number_HU==5) || (tile_number_HD==5)) {s->speed_x = 8*11; col_x = 5;}
		}
		if (s->speed_y < 128){
			y = (s->pos_y)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-3)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+3)>>tsize)];
			if ((tile_number_VR==1) || (tile_number_VL==1)) {s->speed_y = 8*21; col_y = 1;}
			if ((tile_number_VR==5) || (tile_number_VL==5)) {s->speed_y = 8*21; col_y = 5;}
		} else if (s->speed_y > 128){
			y = (s->pos_y+size)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz-3)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + ((s->pos_x+3)>>tsize)];
			if ((tile_number_VR==1) || (tile_number_VL==1)) {s->speed_y = 8*11; col_y = 1;}
			if ((tile_number_VR==5) || (tile_number_VL==5)) {s->speed_y = 8*11; col_y = 5;}
		}
		
		s->pos_x += (LT_Spr_speed[s->speed_x+LT_Spr_speed_float]);
		s->pos_y += (LT_Spr_speed[s->speed_y+LT_Spr_speed_float]);
		
		LT_Spr_speed_float++;
	break;}
	case 3:{//SIDESCROLL
		LT_Player_State[SPR_GND] = s->ground = 1;
		if (LT_Keys[LT_UP]){	//UP
			LT_Player_State[SPR_MOVE] = 1;
			col_y = 0;
			y = (s->pos_y-1)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (col_y == 0) s->pos_y--;
		}
		if (LT_Keys[LT_DOWN]){	//DOWN
			LT_Player_State[SPR_MOVE] = 2;
			col_y = 0;
			y = (s->pos_y+size)>>tsize;
			tile_number_VR = LT_map_collision[( y <<8) + ((s->pos_x+siz)>>tsize)];
			tile_number_VL = LT_map_collision[( y <<8) + (s->pos_x>>tsize)];
			if (tile_number_VR == 1) col_y = 1;
			if (tile_number_VL == 1) col_y = 1;
			if (tile_number_VR == 2) col_y = 1;
			if (tile_number_VL == 2) col_y = 1;
			if (col_y == 0) s->pos_y++;
		}
		if (LT_Keys[LT_LEFT]){	//LEFT
			LT_Player_State[SPR_MOVE] = 3;
			col_x = 0;
			x = (s->pos_x-1)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];	
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x-=2;
		}
		if (LT_Keys[LT_RIGHT]){	//RIGHT
			LT_Player_State[SPR_MOVE] = 4;
			col_x = 0;
			x = (s->pos_x+size)>>tsize;
			tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + x];
			tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + x];
			if (tile_number_HU == 1) col_x = 1;
			if (tile_number_HD == 1) col_x = 1;
			if (col_x == 0) s->pos_x++;
		}
		col_x = 0;
		tile_number_HU = LT_map_collision[((s->pos_y>>tsize) <<8) + ((s->pos_x+size)>>tsize)];
		tile_number_HD = LT_map_collision[(((s->pos_y+siz)>>tsize) <<8) + ((s->pos_x+size)>>tsize)];
		if (tile_number_HU == 1) col_x = 1;
		if (tile_number_HD == 1) col_x = 1;
		if (col_x == 0) s->pos_x++;
		if (s->pos_x < SCR_X+3) s->pos_x = SCR_X+3;
	break;}
	};
	
	if (s->jump_frame == 1) LT_Player_State[SPR_JUMP] = 1;
	else LT_Player_State[SPR_JUMP] = 0;
	LT_Player_State[SPR_TILP_NUM] = tile_number;
	LT_Player_State[SPR_TILC_NUM] = tilecol_number;
	LT_Player_State[SPR_COL_X] = col_x;
	LT_Player_State[SPR_COL_Y] = col_y;
	
	//Fixed animations
	if (LT_Player_State[SPR_GND]){
		switch (LT_Player_State[SPR_MOVE]){
			case LT_SPR_UP: LT_Set_Sprite_Animation(sprite_number,0); break;
			case LT_SPR_DOWN: LT_Set_Sprite_Animation(sprite_number,1); break;
			case LT_SPR_LEFT: LT_Set_Sprite_Animation(sprite_number,2); break;
			case LT_SPR_RIGHT: LT_Set_Sprite_Animation(sprite_number,3); break;
			case LT_SPR_STOP:
				if (LT_Player_Dir == 0) s->frame = s->animations[ 0];
				if (LT_Player_Dir == 1) {
					if (!s->climb) s->frame = s->animations[ 8];
					else s->frame = s->animations[ 0];
				}
				if (LT_Player_Dir == 2) s->frame = s->animations[16];
				if (LT_Player_Dir == 3) s->frame = s->animations[24];
				LT_Sprite_Stop_Animation(sprite_number);
			break;
		}
	} else {
		switch (LT_Player_Dir){
			case 2: LT_Set_Sprite_Animation(sprite_number,4); break;
			case 3: LT_Set_Sprite_Animation(sprite_number,5); break;
		}
	}
}

	
void LT_Update_AI_Sprites(){
	int i = 0;
	int motion = 0;
	int SCX = SCR_X;
	if (LT_AI_Enabled){
	if (LT_VIDEO_MODE > 1) SCX = TGA_SCR_X;
	//for (sprite_number = LT_Sprite_Stack; sprite_number > 1; sprite_number--){
	for (i = 1; i < LT_Sprite_Stack; i++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[i]];
		int px = s->pos_x;
		int py = s->pos_y;
		//CALCULATE ONLY IF IT IS INSIDE THE ACTIVE MAP
		if ((px > SCX)&&(px < (SCX+304))&&(py)&&(py < 308)){	
			byte col_x = 0;
			byte col_y = 0;
			int x,y;
			int sx = s->mspeed_x;
			int sy = s->mspeed_y;
			byte siz = s->width+1;
			byte si = s->width>>1;
			
			switch (s->mode){
			case 0:{ //WALKS ON TOP VIEW
				y = (py+si>>4);
				switch (sx){
					case -1:	//LEFT
					motion = 3;
					col_x = 0;
					x = (px-1)>>4;
					tile_number_HU = LT_map_collision[( y <<8) + x];	
					if (tile_number_HU == 1) col_x = 1;
					if (col_x == 1) {s->speed_x = 0; s->speed_y = 3; s->mspeed_x = 0;s->mspeed_y = -1;}
					break;
					case 1:		//RIGHT
					motion = 4;
					col_x = 0;
					x = (px+siz)>>4;
					tile_number_HU = LT_map_collision[( y <<8) + x];
					if (tile_number_HU == 1) col_x = 1;
					if (col_x == 1) {s->speed_x = 0; s->speed_y = 3; s->mspeed_x = 0; s->mspeed_y = 1;}
					break;
				}
				x = (px+si)>>4;
				switch (sy){
					case -1:	//UP
					motion = 1;
					col_y = 0;
					y = (py-1)>>4;
					tile_number_VR = LT_map_collision[( y <<8) + x];	
					if (tile_number_VR == 1) col_y = 1;
					if (col_y == 1) {s->speed_x = 3; s->speed_y = 0;s->mspeed_y = 0; s->mspeed_x = 1;}
					break;
					case 1:		//DOWN
					motion = 2;
					col_y = 0;
					y = (py+siz)>>4;
					tile_number_VR = LT_map_collision[( y <<8) + x];	
					if (tile_number_VR == 1) col_y = 1;
					if (col_y == 1) {s->speed_x = 3; s->speed_y = 0;s->mspeed_y = 0; s->mspeed_x = -1;}
					break;
				}
			break;}
			case 1:{ //ONLY WALKS ON PLATFORMS UNTILL IT REACHES EDGES OR SOLID TILES
				switch (sx){
					case -1:	//LEFT
					motion = 3;
					col_x = 0;
					x = (px-1)>>4;
					tile_number_HU = LT_map_collision[(((py+si)>>4) <<8) + x];
					tile_number_HD = LT_map_collision[(((py+siz)>>4) <<8) + x];	
					if (tile_number_HU == 1) col_x = 1;
					if (tile_number_HD == 10) col_x = 1;  //Platform edge
					if (col_x == 1) s->mspeed_x *= -1;
					break;
					case 1:		//RIGHT
					motion = 4;
					col_x = 0;
					x = (px+siz)>>4;
					tile_number_HU = LT_map_collision[(((py+si>>4)) <<8) + x];
					tile_number_HD = LT_map_collision[(((py+siz)>>4) <<8) + x];
					if (tile_number_HU == 1) col_x = 1;
					if (tile_number_HD == 10) col_x = 1; //Platform edge
					if (col_x == 1) s->mspeed_x *= -1;
					break;
				}
			break;}
			case 3:{ //Only flies to the left
				s->speed_x = 1;
				s->mspeed_x = -1;
			break;}
			}
			col_x = 0;
			col_y = 0;
			
			switch (motion){
				case 1: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],0); break;
				case 2: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],1); break;
				case 3: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],2); break;
				case 4: LT_Set_Sprite_Animation(LT_Sprite_Stack_Table[i],3); break;
			}
			//SPEED CONTROL TO MAKE ENEMIES TO MOVE SLOWER THAN +=1
			if (s->speed_x != 0){
				if (s->s_x == s->speed_x) {
					s->s_x = 0;
					s->pos_x += s->mspeed_x;
				}
				s->s_x ++;
			}
			if (s->speed_y != 0){
				if (s->s_y == s->speed_y) {
					s->s_y = 0;
					s->pos_y += s->mspeed_y;
				}
				s->s_y ++;
			}
		}
	}
	}
}

int LT_Player_Col_Enemy(){
	unsigned char i = 0;
	int col = 0;
	int ex = 0;
	int ey = 0;
	SPRITE *p = &sprite[LT_Sprite_Stack_Table[0]];
	int px = p->pos_x;
	int py = p->pos_y;
	for (i = 1; i < LT_Sprite_Stack; i++){
		SPRITE *s = &sprite[LT_Sprite_Stack_Table[i]];
		ex = s->pos_x;
		ey = s->pos_y;
		if ((_abs(px-ex)<LT_ENEMY_DIST_X)&&(_abs(py-ey)<LT_ENEMY_DIST_Y)) col = 1;
	}
	return col;
}

void LT_Unload_Sprites(){
	int i = 0;
	for (i=0;i<20;i++){
		SPRITE *s = &sprite[i];
		s->init = 0;
		s->code_size = 0;
	}
	LT_AI_Enabled = 0;
	if (LT_VIDEO_MODE==1)LT_sprite_data_offset = 16*1024; //just after loading animation
	if (LT_VIDEO_MODE==0)LT_sprite_data_offset = 512;
	if (LT_VIDEO_MODE >1){
		LT_sprite_data_offset = 2048;
		LT_memset(&LT_sprite_data[LT_sprite_data_offset],0,62*1024);
	}
	//LT_Delete_Sprite(sprite_number);
	//for (i=0;i<s->nframes;i++){
	//	farfree(s->frames[i].compiled_code);
	//	s->frames[i].compiled_code = NULL;
	//}
}	
