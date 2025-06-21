#include <nds.h>
#include <filesystem.h>
#include <fat.h>

#include <cstdio>
#include <cstdlib>

#include "oamMan.h"
#include "gameMode.h"
#include "point.h"
#include "gui.h"
#include "buttonMan.h"
#include "vblankBatcher.h"
#include "hblankBatcher.h"
#include "tcm.h"

#include "globals.h"
#include "consts.h"

#include "testMode.h"

extern std::unique_ptr<GameMode> g_mode;
extern std::unique_ptr<GameMode> g_nextMode;

void onVBlank();
void setupGFX();

DTCM_BSS volatile unsigned long int currentFrame = 0;
DTCM_BSS volatile unsigned long int lagFrames = 0;
DTCM_BSS volatile unsigned long int slowVblankInterrupts = 0;
DTCM_BSS volatile bool readyForVblank = false;

unsigned long int vblankOverruns = 0;

Point touchPoint(0,0);

TileReservationToken consoleReservation;

bool fatInitSuccess;

int main(void) {
	defaultExceptionHandler();
	
	fatInitSuccess = fatInitDefault();

	if(!nitroFSInit(nullptr)) {
		puts("nitroFSInit failed.");
		return 1;
	}

	irqEnable(IRQ_VBLANK);
	irqSet(IRQ_VBLANK, onVBlank);

	irqSet(IRQ_HBLANK, HBlankBatcher::ISR);

	consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, subBg1MapBase, subBg1TileBase, false, true);
	consoleReservation = TileManSubBg1.reserve(128);
	
	setupGFX();
	
	//TODO: custom init
	g_mode = std::make_unique<TestMode>();
	g_mode->Load();

	for(;;) {
		scanKeys();

		buttonMan.Update();

		if(keysCurrent() & KEY_TOUCH) {
			touchPosition data;
			touchRead(&data);

			touchPoint.x = data.px;
			touchPoint.y = data.py;
		}
		HBlankBatcher::getCurrentWriter().clear();
		vblankBatcher.Clear();

		if(g_nextMode) {
			REG_DISPCNT |= DISPLAY_SCREEN_OFF;
			g_mode->Unload();
			g_mode = std::move(g_nextMode);
			g_mode->Load();
			REG_DISPCNT &= ~DISPLAY_SCREEN_OFF;
		}
		
		g_mode->Update();
		
		mainOamMan.updateOAM();
		subOamMan.updateOAM();

		readyForVblank = true;

		swiWaitForVBlank();
		{
			unsigned long int startFrame = currentFrame;
			bool beganInVblank = (REG_DISPSTAT & DISP_IN_VBLANK);
			vblankBatcher.Execute();

			HBlankBatcher::swapBatchers();

			if(beganInVblank && (startFrame < currentFrame || (REG_DISPSTAT & DISP_IN_VBLANK) != DISP_IN_VBLANK)) {
				++vblankOverruns;
			}
		}
	}
}

void onVBlank() {
	bool irqLate = (REG_DISPSTAT & DISP_IN_VBLANK) != DISP_IN_VBLANK;
	if(irqLate) {
		slowVblankInterrupts = slowVblankInterrupts + 1;
	}
	currentFrame = currentFrame + 1;

	if(!irqLate && !readyForVblank) {
		lagFrames = lagFrames + 1;
		return;
	}
	readyForVblank = false;
}

void setupGFX() {
	lcdMainOnBottom();
	powerOn(PM_BACKLIGHT_BOTTOM | PM_BACKLIGHT_TOP);
	powerOn(POWER_ALL_2D);
	
	vramSetBankA(VRAM_A_MAIN_BG_0x06020000);
	vramSetBankB(VRAM_B_MAIN_SPRITE_0x06400000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	vramSetBankF(VRAM_F_MAIN_BG);
	
	REG_DISPCNT = DISPLAY_SPR_1D_LAYOUT | MODE_1_2D | DISPLAY_BG0_ACTIVE | DISPLAY_CHAR_BASE(tileBaseMainBig);
	
	REG_BG0CNT = BG_32x32 | BG_PRIORITY_3 | BG_COLOR_16 | BG_TILE_BASE(bg0TileBase) | BG_MAP_BASE(bg0MapBase);
	REG_BG1CNT = BG_32x32 | BG_PRIORITY_2 | BG_COLOR_16 | BG_TILE_BASE(bg1TileBase) | BG_MAP_BASE(bg1MapBase);
	REG_BG2CNT = BG_32x32 | BG_PRIORITY_0 | BG_COLOR_16 | BG_TILE_BASE(bg2TileBase) | BG_MAP_BASE(bg2MapBase);
	REG_BG3CNT = BG_RS_16x16 | BG_PRIORITY_1 | BG_COLOR_16 | BG_TILE_BASE(bg3TileBase) | BG_MAP_BASE(bg3MapBase);
	
	REG_DISPCNT_SUB = DISPLAY_SPR_1D_LAYOUT | MODE_3_2D | DISPLAY_BG1_ACTIVE;
	
	REG_BG0CNT_SUB = BG_32x32 | BG_PRIORITY_3 | BG_COLOR_16 | BG_TILE_BASE(subBg0TileBase) | BG_MAP_BASE(subBg0MapBase);
	REG_BG1CNT_SUB = BG_32x32 | BG_PRIORITY_2 | BG_COLOR_16 | BG_TILE_BASE(subBg1TileBase) | BG_MAP_BASE(subBg1MapBase);

	TileManMainBg0.setBpp(4);
}