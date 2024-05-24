#include <stdarg.h>
#include <stdio.h>



#include "doomdef.h"
#include "doomtype.h"
#include "d_main.h"
#include "d_event.h"

#include "global_data.h"

#include "tables.h"

#include "i_system_esps3.h"





//**************************************************************************************


//*******************************************************************************
//VBlank handler.
//*******************************************************************************

void VBlankCallback()
{

}


void I_InitScreen_e32()
{

}

//**************************************************************************************

void I_BlitScreenBmp_e32()
{

}

//**************************************************************************************

void I_StartWServEvents_e32()
{

}


//**************************************************************************************

void I_ClearWindow_e32()
{

}

//**************************************************************************************

//unsigned short __dbuf[320*240];
extern unsigned short * __sprite_data;
unsigned short* I_GetBackBuffer()
{
    // CPSTUB

    return __sprite_data;
}

//**************************************************************************************

unsigned short* I_GetFrontBuffer()
{
    // CPSTUB
    return __sprite_data;
}

//**************************************************************************************

void I_CreateWindow_e32()
{



}

//**************************************************************************************

void I_CreateBackBuffer_e32()
{
    I_CreateWindow_e32();
}

//**************************************************************************************
extern void __update_sprite();

void I_FinishUpdate_e32(const byte* srcBuffer, const byte* pallete, const unsigned int width, const unsigned int height)
{
    __update_sprite();
}

//**************************************************************************************
void __set_sprite_pallete(unsigned int i, unsigned char r, unsigned char g, unsigned char b);

void I_SetPallete_e32(const byte* pallete)
{

    for(int i = 0; i< 256; i++)
    {
        unsigned int r = *pallete++;
        unsigned int g = *pallete++;
        unsigned int b = *pallete++;

        __set_sprite_pallete(i,r,g,b);
    }

}

//**************************************************************************************

int I_GetVideoWidth_e32()
{
    return 120;
}

//**************************************************************************************

int I_GetVideoHeight_e32()
{
    return 160;
}



//**************************************************************************************
extern unsigned char __get_event();

void I_ProcessKeyEvents()
{
    unsigned char evt_cp = __get_event();

    if(evt_cp != 0) {
        event_t ev;

        if(evt_cp & 0x80) {
            //btn down
            ev.type = ev_keydown;
        } else {
            //btn up
            ev.type = ev_keyup;
        }
        ev.data1 = evt_cp & 0x7f;
        D_PostEvent(&ev);
    }

}

//**************************************************************************************

#define MAX_MESSAGE_SIZE 1024

void I_Error (const char *error, ...)
{
    printf("I_Error: %s\n", error);
}

//**************************************************************************************

void I_Quit_e32()
{

}

//**************************************************************************************


