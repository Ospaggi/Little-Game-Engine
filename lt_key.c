#include "lt__eng.h"


//KEYBOARD
volatile unsigned char LT_Keys[256]; // volatile: changed in interrupt
volatile unsigned char keyhit = 0;

void interrupt (*LT_old_key_handler)(void);
void interrupt (far * LT_getvect(byte intr))();
void LT_setvect(byte intr, void interrupt (far *func)());
void LT_memset(void *ptr, byte val, word number);

//So easy, and so difficult to find samples of this...
void interrupt Key_Handler(void) {
    unsigned char sc;

    asm cli
    asm in  al, 0x60 //READ KEYBOARD PORT
    asm mov sc, al   //Store in keyhit
    asm in  al, 0x61 //READ SYSTEM CONTROL PORT
    asm mov bl, al
    asm or  al, 0x80
    asm out 0x61, al //DISABLE KEYBOARD (send "received" command (bit 7))
    asm mov al, bl
    asm out 0x61, al //RE ENABLE KEYBOARD (original state)
    asm mov al, 0x20 //Send 0x20 to 0x20 port (end of interrupt)
    asm out 0x20, al
    asm sti

    keyhit = sc;
	//0x80 => BREAK/RELEASE KEY CODE
    if (sc & 0x80) {
        sc &= 0x7F;
        LT_Keys[sc] = 0; //KEY_RELEASED
    } else {
        LT_Keys[sc] = 1; //KEY_PRESSED
    }
}

void LT_destroy_key_handler(){
    LT_old_key_handler = LT_getvect(9); // save old handler
    LT_setvect(9, Key_Handler);
    LT_memset(LT_Keys, 0, 256);
}

void LT_reset_key_handler(){
	LT_setvect(9,LT_old_key_handler);
}

byte LT_kbhit(){
	asm mov ax,0x0100
	asm int 0x16
	asm jnz	_skip	//if zf == 0
	return 0;		//Buffer empty
	_skip:
	return 1;		//key in buffer
}

byte LT_Wait_kbhit(){
	byte val = 0;
	asm mov ax,0x0000
	asm int 0x16
	asm mov val,al
	return val;
}

void Clearkb(){
	LT_memset(LT_Keys, 0, 256);
}

//JOYSTICK
#define JPORT 0x0201
volatile unsigned char LT_JOYSTICK = 0;
volatile unsigned char LT_JOY0_B,LT_JOY1_B;
volatile unsigned char LT_JOY0_A,LT_JOY1_A;
volatile unsigned short LT_JOY0_CX,LT_JOY1_CX;
volatile unsigned short LT_JOY0_CY,LT_JOY1_CY;
volatile unsigned short LT_JOY0_CENTER_X,LT_JOY1_CENTER_X = 0;
volatile unsigned short LT_JOY0_DEAD_X0,LT_JOY1_DEAD_X0;
volatile unsigned short LT_JOY0_DEAD_X1,LT_JOY1_DEAD_X1;
volatile unsigned short LT_JOY0_CENTER_Y,LT_JOY1_CENTER_Y = 0;
volatile unsigned short LT_JOY0_DEAD_Y0,LT_JOY1_DEAD_Y0;
volatile unsigned short LT_JOY0_DEAD_Y1,LT_JOY1_DEAD_Y1;

void read_joy(){//76.676
	LT_JOY0_CX = 0; LT_JOY0_CY = 0; 
	LT_JOY0_B = 1; LT_JOY0_A = 1;
	
	asm cli	
	///Read position without bios
	//Write dummy byte to joystick port, this makes al X Y bytes 1
	asm mov dx,JPORT;
	asm out dx,al
	//count until bit 1 returns to 0 (roughly proportional to X position)
    _joystick_x:
		asm inc LT_JOY0_CX
		asm in al,dx            //Get joystick port value
		asm and al,0x01
		asm jne _joystick_x
	_done_x:
	asm out dx,al
	//count until bit 1 returns to 0 (roughly proportional to Y position)
    _joystick_y:
		asm inc LT_JOY0_CY
		asm in al,dx
		asm and al,0x02
		asm jne _joystick_y
	_done_y:
	
	//READ BUTTONS
	asm mov dx,JPORT;
	asm in al,dx
	asm mov bl,al;
	asm and al,0x10
	asm mov LT_JOY0_B,al
	asm and bl,0x20
	asm mov LT_JOY0_A,bl
	
	asm sti
	
	//READ POSITION (BIOS FUNCTION)
	/*asm mov ah,0x84
	asm mov dx,1	//read
	asm int 0x15
	asm mov LT_JOY0_CX,AX
	asm mov LT_JOY0_CY,BX
	//asm mov LT_JOY1_CX,CX
	//asm mov LT_JOY1_CY,DX*/
}

void LT_Calibrate_JoyStick(){
	int max_x = 0; int min_x = 0;
	int max_y = 0; int min_y = 0;
	_printf("\n\rmove joystick in all directions, then center it and press a button.\n\rPress keyboard to cancel.$");
	read_joy();
	LT_JOY0_CENTER_X = LT_JOY0_CX;
	LT_JOY0_CENTER_Y = LT_JOY0_CY;
	while (1){ 
		read_joy();
		if (LT_JOY0_CX < min_x) min_x = LT_JOY0_CX;
		if (LT_JOY0_CX > max_x) max_x = LT_JOY0_CX;
		if (LT_JOY0_CY < min_y) min_y = LT_JOY0_CY;
		if (LT_JOY0_CY > max_y) max_y = LT_JOY0_CY;
		if (!LT_JOY0_B || !LT_JOY0_A) {LT_JOYSTICK = 1;break;}
		if (LT_kbhit()) break; 
	}
	LT_JOY0_DEAD_X0 = LT_JOY0_CENTER_X - ((LT_JOY0_CENTER_X-min_x)>>2);
	LT_JOY0_DEAD_X1 = LT_JOY0_CENTER_X + ((max_x-LT_JOY0_CENTER_X)>>2);
	LT_JOY0_DEAD_Y0 = LT_JOY0_CENTER_Y - ((LT_JOY0_CENTER_Y-min_y)>>2);
	LT_JOY0_DEAD_Y1 = LT_JOY0_CENTER_Y + ((max_y-LT_JOY0_CENTER_Y)>>2);
	LT_sleep(1);
}

void LT_Read_Joystick(void) {
    if (LT_JOYSTICK == 1) {
        LT_Keys[LT_ACTION] = LT_Keys[LT_JUMP] = 0;
        LT_Keys[LT_UP] = LT_Keys[LT_DOWN] = 0;
        LT_Keys[LT_LEFT] = LT_Keys[LT_RIGHT] = 0;
		//Read ports
        read_joy();
		//Update controls
        if (!LT_JOY0_B) LT_Keys[LT_ACTION] = 1;
        if (!LT_JOY0_A) LT_Keys[LT_JUMP]   = 1;
        if (LT_JOY0_CY < LT_JOY0_DEAD_Y0) LT_Keys[LT_UP]    = 1;
        if (LT_JOY0_CY > LT_JOY0_DEAD_Y1) LT_Keys[LT_DOWN]  = 1;
        if (LT_JOY0_CX < LT_JOY0_DEAD_X0) LT_Keys[LT_LEFT]  = 1;
        if (LT_JOY0_CX > LT_JOY0_DEAD_X1) LT_Keys[LT_RIGHT] = 1;
    }
}