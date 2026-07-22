#include <stdio.h>
#include "m_argv.h"
#include "doomgeneric.h"

pixel_t* DG_ScreenBuffer = NULL;

void M_FindResponseFile(void);
void D_DoomMain(void);

void doomgeneric_Create(int argc, char **argv)
{
    myargc = argc;
    myargv = argv;

    M_FindResponseFile();

    DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

    /* NÃO chama DG_Init aqui — será chamado por I_InitGraphics dentro de D_DoomLoop */

    D_DoomMain();
}
