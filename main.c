#include <stdio.h>                      // for sprintf()
#include <stdbool.h>                    // for data type bool
#include <plib.h>                       // Peripheral Library
#include <time.h>
#include <adc10.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL	// Oscillator selection
#pragma config POSCMOD      = XT	// Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2	// PLL input divider
#pragma config FPLLMUL      = MUL_20	// PLL multiplier
#pragma config FPLLODIV     = DIV_1	// PLL output divider
#pragma config FPBDIV       = DIV_8	// Peripheral bus clock divider
#pragma config FSOSCEN      = OFF	// Secondary oscillator enable

// Function prototypes
void Initialize();
bool getInput1();
bool getInput2();
void OledStringCursor(char *buf, int x, int y);
void Timer2Init();
void Timer4Init();
void debounce();
int selectNote(bool diff);
int ReadRegister(SpiChannel chn, int reg);
void WriteRegister(SpiChannel chn, int reg, int data);
bool getSPILeft(int Yval);
bool getSPIRight(int Yval);
int Random();

bool tap;

void __ISR(_EXTERNAL_1_VECTOR, IPL7AUTO) _External1Handler(void) {
    tap = true;
    INTClearFlag(INT_INT1);
}



void Timer2Init() 
{     
   OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 62499);
   return; 
}//Opens a 100ms timer

void Timer4Init() 
{     
   OpenTimer4(T4_ON | T4_IDLE_CON | T4_SOURCE_INT | T4_PS_1_16 | T4_GATE_OFF, 31249);     
   return; 
}////Opens a 50ms timer

int main()
{
    char buf[17];
    enum states {Init, DiffSel1, DiffSel2, Game};
    enum states Menu = Init;
    SpiChannel chn = SPI_CHANNEL3;
    Initialize();
    TMR2 = 0x0;
    Timer2Init();
    //initializes all the variables needed outside of the while loop    
    
    while(1)
    {
        switch (Menu)
        {
            bool hard;
            case Init:
                OledClearBuffer();
                OledSetCursor(0, 0);
                hard = false;
                
                OledPutString("Drum Hero");
                
                TMR4 = 0x0;
                Timer4Init();
                int timer4_past = ReadTimer4();
                int displayed = 0;
                while (displayed < 50)
                {
                    if(displayed > 20)
                    {
                        OledSetCursor(0, 2);
                        OledPutString("Josh Rehm");
                        OledSetCursor(0, 3);
                        OledPutString("ECE 2534");
                    }
                    int timer4_cur = ReadTimer4();
                    if (timer4_past > timer4_cur)
                    {
                        displayed++;
                    }
                    timer4_past = timer4_cur;
                }
                
                Menu = DiffSel1;
                break;
            case DiffSel1:
                OledClearBuffer();
                sprintf(buf, "Difficulty Level", 0);
                OledStringCursor(buf, 0, 0);
                sprintf(buf, "-> EASY (20)", 0);
                OledStringCursor(buf, 0, 2);
                sprintf(buf, "   HARD (30)", 0);
                OledStringCursor(buf, 0, 3);
                bool moved = false;
                while(!moved)
                {
                
                    if(getInput2())
                    {
                        hard = false;
                        moved = true;
                        Menu = Game;
                        debounce();

                    }
                    else if(getInput1())
                    {
                        moved = true;
                        Menu = DiffSel2;
                        debounce();
                    }
                }
                
                break;
                
            case DiffSel2:
                OledClearBuffer();
                sprintf(buf, "Difficulty Level", 0);
                OledStringCursor(buf, 0, 0);
                sprintf(buf, "   EASY (20)", 0);
                OledStringCursor(buf, 0, 2);
                sprintf(buf, "-> HARD (30)", 0);
                OledStringCursor(buf, 0, 3);
                moved = false;
                while(!moved)
                {
                
                    if(getInput2())
                    {
                        hard = true;
                        moved = true;
                        Menu = Game;
                        debounce();

                    }
                    else if(getInput1())
                    {
                        moved = true;
                        Menu = DiffSel1;
                        debounce();
                    }
                }
                
                break;
                
            case Game:
                OledClearBuffer();
                OledUpdate();
                int score;
                
                int hits = selectNote(hard);
                if(hard && hits)
                    score = hits + 10;
                else score = hits;
                    
                
                OledClearBuffer();
                OledSetCursor(0, 0);
                sprintf(buf, "Score: %d", score);
                OledPutString(buf);
                OledSetCursor(0, 2);
                sprintf(buf, "->  Menu", 0);
                OledPutString(buf);
                
                moved = false;
                while(!moved)
                {
                    
                    if(getInput2())
                    {
                        moved = true;
                        Menu = DiffSel1;

                    }
                }
                
                break;
        }
    }
}


/////////////////////////////////////////////////////////////////
// Function:    getInput
// Description: Perform a nonblocking check to see if BTN1 has been pressed
// Inputs:      None
// Returns:     TRUE if 0-to-1 transition of BTN1 is detected;
//                otherwise return FALSE
//
bool getInput1()
{
    enum Button1Position {UP, DOWN}; // Possible states of BTN1
    
    static enum Button1Position button1CurrentPosition = UP;  // BTN1 current state
    static enum Button1Position button1PreviousPosition = UP; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    button1PreviousPosition = button1CurrentPosition;
    // Read BTN1
    if(PORTG & 0x40)                                
    {
        button1CurrentPosition = DOWN;
    } else
    {
        button1CurrentPosition = UP;
    } 
    if((button1CurrentPosition == DOWN) && (button1PreviousPosition == UP))
    {
        TMR4 = 0x0;
        Timer4Init();
        int timer4_past = ReadTimer4();
        bool debouncing = true;
        while (debouncing)
        {
            int timer4_cur = ReadTimer4();
            if (timer4_past > timer4_cur)
            {
                debouncing = false;
            }
            timer4_past = timer4_cur;
        }
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}
bool getInput2()
{
    enum Button2Position {UP, DOWN}; // Possible states of BTN1
    
    static enum Button2Position button2CurrentPosition = UP;  // BTN1 current state
    static enum Button2Position button2PreviousPosition = UP; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    button2PreviousPosition = button2CurrentPosition;
    // Read BTN1
    if(PORTG & 0x80)                                
    {
        button2CurrentPosition = DOWN;
    } else
    {
        button2CurrentPosition = UP;
    } 
    if((button2CurrentPosition == DOWN) && (button2PreviousPosition == UP))
    {
        TMR4 = 0x0;
        Timer4Init();
        int timer4_past = ReadTimer4();
        bool debouncing = true;
        while (debouncing)
        {
            int timer4_cur = ReadTimer4();
            if (timer4_past > timer4_cur)
            {
                debouncing = false;
            }
            timer4_past = timer4_cur;
        }
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}

//Debounces for 50ms
void debounce()
{
    TMR4 = 0x0;
    Timer4Init();
    int timer4_past = ReadTimer4();
    bool debouncing = true;
    while (debouncing)
    {
        int timer4_cur = ReadTimer4();
        if (timer4_past > timer4_cur)
        {
            debouncing = false;
        }
        timer4_past = timer4_cur;
    }
}

/////////////////////////////////////////////////////////////////
// Function:     Initialize
// Description:  Initialize the system
// Inputs:       None
// Return value: None
//
void Initialize()
{
   // Initialize GPIO for all LEDs
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGSET = 0x80;     // For BTN2: configure PortG bit for input
   TRISGCLR = 0xf000;   // For LEDs 1-4: configure PortG pins for output
   ODCGCLR  = 0xf000;   // For LEDs 1-4: configure as normal output (not open drain)
   LATGCLR = 0xf000;

   // Initialize Timer1 and OLED
   DelayInit();
   OledInit();
   
   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
   
   SpiChannel chn = SPI_CHANNEL3;
    
   SpiChnOpen(chn, SPI_OPEN_MSTEN | SPI_OPEN_MSSEN | SPI_OPEN_SMP_END | SPI_OPEN_CKP_HIGH | SPI_OPEN_ENHBUF | SPI_OPEN_MODE8, 10);
    
   //WriteRegister(chn, 0x2E, 0x00);
   WriteRegister(chn, 0x31, 0x01);
   WriteRegister(chn, 0x2D, 0x08);
   WriteRegister(chn, 0x1D, 0x30);
   WriteRegister(chn, 0x21, 0x10);
   WriteRegister(chn, 0x22, 0x10);
   WriteRegister(chn, 0x23, 0x40);
   WriteRegister(chn, 0x2A, 0x01);
   WriteRegister(chn, 0x2F, 0x20);
   WriteRegister(chn, 0x2E, 0x20);
   
   
   
   INTSetVectorPriority(INT_EXTERNAL_1_VECTOR, INT_PRIORITY_LEVEL_7);
   mPORTESetPinsDigitalIn(BIT_8);
   ConfigINT1(EXT_INT_ENABLE | FALLING_EDGE_INT | EXT_INT_PRI_7);
   INTEnableInterrupts();
      
   return;
}

//Places a string, but not super helpful due to the OledUpdate() function
void OledStringCursor(char *buf, int x, int y){
    OledSetCursor(x, y);
    OledPutString(buf);
    OledUpdate();
}

void OledMakeL(int Lx,int Ly)
{
//    OledMoveTo(Lx, Ly);
//    OledLineTo(Lx - 7, Ly);
//    OledLineTo(Lx - 7, Ly + 7);
//    OledLineTo(Lx, Ly + 7);
//    OledLineTo(Lx, Ly);
    OledMoveTo(Lx - 5, Ly);
    OledLineTo(Lx - 5, Ly + 6);
    OledLineTo(Lx - 1, Ly + 6);
}
void OledMakeR(int Rx, int Ry)
{
//    OledMoveTo(Rx, Ry);
//    OledLineTo(Rx - 7, Ry);
//    OledLineTo(Rx - 7, Ry + 7);
//    OledLineTo(Rx, Ry + 7);
//    OledLineTo(Rx, Ry);
    OledMoveTo(Rx - 6, Ry);
    OledLineTo(Rx - 2, Ry);
    OledLineTo(Rx - 2, Ry + 2);
    OledMoveTo(Rx - 2, Ry + 4);
    OledDrawPixel();
    OledMoveTo(Rx - 2, Ry + 5);
    OledDrawPixel();
    OledMoveTo(Rx - 3, Ry + 3);
    OledDrawPixel();
    OledMoveTo(Rx - 4, Ry + 3);
    OledDrawPixel();
    OledMoveTo(Rx - 6, Ry);
    OledLineTo(Rx - 6, Ry + 6);
}
void OledMakeT(int Tapx, int Tapy)
{
//    OledMoveTo(Tapx, Tapy);
//    OledLineTo(Tapx - 7, Tapy);
//    OledLineTo(Tapx - 7, Tapy + 7);
//    OledLineTo(Tapx, Tapy + 7);
//    OledLineTo(Tapx, Tapy);
    OledMoveTo(Tapx - 7, Tapy);
    OledLineTo(Tapx, Tapy);
    OledMoveTo(Tapx - 4, Tapy);
    OledLineTo(Tapx - 4, Tapy + 7);
}

int selectNote(bool diff)
{
    int numNotes;
    if(diff)
        numNotes = 30;
    else numNotes = 20;
    int values[numNotes];
    int randsel[numNotes];
    int i;
    TMR4 = 0x0;
    Timer4Init();    
    SpiChannel chn = SPI_CHANNEL3;
    for(i = 0; i < numNotes; i++)
        values[i] = 128 + (100 * i);
    for(i = 0; i < numNotes; i++)
        randsel[i] = (Random() % 3);
    int Ly = 8;
    int Ry = 16;
    int Tapy = 24;
    bool confirm = false;
    int hits = 15;
    int divis = 0;
    
    
    
    while(values[numNotes - 1] >= 7)
    {
        tap = false;
        OledClearBuffer();
        OledMoveTo(7,8);
        OledLineTo(7, 31);
        OledMoveTo(20,8);
        OledLineTo(20, 31);
        for(i = 0; i < numNotes; i++)
        {
            values[i]--;
            if(values[i] < 128 && values[i] > 7)
            {
                if(randsel[i] == 0)
                    OledMakeL(values[i], Ly);
                else if(randsel[i] == 1)
                    OledMakeR(values[i], Ry);
                else
                    OledMakeT(values[i], Tapy);
                
                if(values[i] <= 27 && values[i] > 7)
                    confirm = true;
                else
                    confirm = false;
            }
            if(confirm)
            {
                int j;
                int Yavg = 0;
                for(j = 0; j < 10; j++)
                {
                    int y1 = ReadRegister(chn, 0x34);
                    int y2 = ReadRegister(chn, 0x35);
                    int Y = y2 << 8 | y1;
                    Yavg += Y;
                }
                Yavg /= 10;
                ReadRegister(chn, 0x30);
                
                if(randsel[i] == 0)
                {
                    if(getSPILeft(Yavg))
                    {
                        hits++;
                        divis = 0;
                        values[i] = 0;
                    }
                    else 
                    {
                        divis++;
                        if(divis / 20 == 1)
                        {
                            hits--;
                            divis = 0;
                        }
                    }
                }
                else if(randsel[i] == 1)
                {
                    if(getSPIRight(Yavg))
                    {
                        hits++;
                        divis = 0;
                        values[i] = 0;
                    }
                    else 
                    {
                        divis++;
                        if(divis / 20 == 1)
                        {
                            hits--;
                            divis = 0;
                        }
                    }
                }
                else if(randsel[i] == 2)
                {
                    if(tap)
                    {
                        hits++;
                        values[i] = 0;
                        divis = 0;
                        INTClearFlag(INT_INT1);
                    }
                        
                    else
                    {
                        divis++;
                        if(divis / 20 == 1)
                        {
                            hits--;
                            divis = 0;
                        }
                    }
                }
            }
        }
        
        if(hits == 0)
        {
            LATGCLR = 0xF000;
            return hits;
        }
            
        else if(hits < 6)
        {
            LATGCLR = 0xE000;
            LATGSET = 0x1000;
        }
        else if(hits < 12)
        {
            LATGCLR = 0xC000;
            LATGSET = 0x3000;
        }
        else if(hits < 20)
        {
            LATGCLR = 0x8000;
            LATGSET = 0x7000;
        }
        else LATGSET = 0xF000;
        OledUpdate();
    }
    return hits;
}


int ReadRegister(SpiChannel chn, int reg)
{
    SpiChnPutC(chn, reg | 0x80);
    SpiChnPutC(chn, 0x80);
    SpiChnGetC(chn);
    return SpiChnGetC(chn);
}

void WriteRegister(SpiChannel chn, int reg, int data)
{
    SpiChnPutC(chn, reg);
    SpiChnPutC(chn, data);
    SpiChnGetC(chn);
    SpiChnGetC(chn);
}

bool getSPILeft(int Yval)
{
    enum SPIYPosition {UP, DOWN}; // Possible states of BTN1
    
    static enum SPIYPosition SPIYCurrentPosition = UP;  // BTN1 current state
    static enum SPIYPosition SPIYPreviousPosition = UP; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    SPIYPreviousPosition = SPIYCurrentPosition;
    // Read BTN1
    if(Yval > 0x30 && Yval < 0x100)                                
    {
        SPIYCurrentPosition = DOWN;
    } 
    else if(Yval < 0x30 || Yval > 0xFFD0)
    {
        SPIYCurrentPosition = UP;
    }
    if((SPIYCurrentPosition == DOWN) && (SPIYPreviousPosition == UP))
    {
        TMR4 = 0x0;
        Timer4Init();
        int timer4_past = ReadTimer4();
        bool debouncing = true;
        while (debouncing)
        {
            int timer4_cur = ReadTimer4();
            if (timer4_past > timer4_cur)
            {
                debouncing = false;
            }
            timer4_past = timer4_cur;
        }
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}

bool getSPIRight(int Yval)
{
    enum SPIYPosition {UP, DOWN}; // Possible states of BTN1
    
    static enum SPIYPosition SPIYCurrentPosition = UP;  // BTN1 current state
    static enum SPIYPosition SPIYPreviousPosition = UP; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    SPIYPreviousPosition = SPIYCurrentPosition;
    // Read BTN1
    if(Yval < 0xFFD0 && Yval > 0x100)                                
    {
        SPIYCurrentPosition = DOWN;
    } 
    else if(Yval < 0x30 || Yval > 0xFFD0)
    {
        SPIYCurrentPosition = UP;
    }
    if((SPIYCurrentPosition == DOWN) && (SPIYPreviousPosition == UP))
    {
        TMR4 = 0x0;
        Timer4Init();
        int timer4_past = ReadTimer4();
        bool debouncing = true;
        while (debouncing)
        {
            int timer4_cur = ReadTimer4();
            if (timer4_past > timer4_cur)
            {
                debouncing = false;
            }
            timer4_past = timer4_cur;
        }
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}

int Random()
{
    SpiChannel chn = SPI_CHANNEL3;
    int t = ReadTimer4();
    int r = ReadRegister(chn, 0x32) + t;
    r = ((r * 7621) + 1) % 32768;
    return r;
}
