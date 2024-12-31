//#include <stdio.h>
//#include <unistd.h>

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include "theoraplayer.h"
//#include "explorer.h"
C3D_RenderTarget *top;
//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------
	int fileMax;
	int cursor = 0;
	int from = 0;

	bool startedPlaying = false;

	romfsInit();
	ndspInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);
	hidSetRepeatParameters(25, 5);

	//osSetSpeedupEnable(true); //Sets the new3ds faster mode or something, I wouldn't know, my 3ds is 14 years old.


	// Create screens
	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	ndspSetCallback(TP_audioCallback, NULL);

	while(aptMainLoop()){
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kDownRepeat = hidKeysDownRepeat();

		if (kDown & KEY_START)
			break;

		//Starts playing the file, sets the file
		if (!THEORA_isplaying && startedPlaying == false) {
			TP_changeFile("sdmc:/videos/test.ogg");
		}
		else{
			startedPlaying = true;
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(top, C2D_Color32(20, 29, 31, 255));
			C2D_SceneBegin(top);


			if (THEORA_isplaying && THEORA_HasVideo(&THEORA_vidCtx))
				frameDrawAtCentered(&THEORA_frame, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0.5f, THEORA_scaleframe, THEORA_scaleframe);


		C3D_FrameEnd(0);

		if (!THEORA_isplaying && startedPlaying)
		{
			TP_exitThread(); //Stops playing the video at the end, otherwise it just loops through
		}
	}

	TP_exitThread();

	osSetSpeedupEnable(false);

	gfxExit();
	ndspExit();
	romfsExit();
	return 0;
}
