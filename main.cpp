/*
  BOTLOOP
  Copyright (C) 2020 Bernhard Schelling

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <ZL_Application.h>
#include <ZL_Display.h>
#include <ZL_Surface.h>
#include <ZL_Audio.h>
#include <ZL_Font.h>
#include <ZL_Input.h>
#include <ZL_SynthImc.h>
#include <vector>
using namespace std;

static ZL_Font fntMain, fntBig;
static ZL_Surface srfBot, srfTiles;
static ZL_Sound sndSelect, sndRun, sndReturn, sndClear, sndStage, sndMove, sndBump;
static ZL_SynthImcTrack imcMusic;

enum ECommand { CMD_NONE, CMD_FORWARD, CMD_REVERSE, CMD_TURNLEFT, CMD_TURNRIGHT, CMD_COUNT };
enum EBotState { BOT_PROGRAMMING, BOT_RUNNING, BOT_CLEARED };
enum ETiles
{
	TILE_FLAG = 8,
	TILE_WALL = 12,
	TILE_FLOOR = 13,
	TILE_SHADOW = 15,
	TILE_CMD_NONE = 3,
	TILE_CMD_FORWARD = 6,
	TILE_CMD_REVERSE = 7,
	TILE_CMD_TURNLEFT = 10,
	TILE_CMD_TURNRIGHT = 11,
};
enum EGameState
{
	GAME_BOOT,
	GAME_TITLE,
	GAME_STAGEFADEIN,
	GAME_STAGENAME,
	GAME_PLAY,
	GAME_CLEARSTAGE,
	GAME_CLEARALL,
	GAME_STAGEFADEOUT,
};
static EGameState GameState;
static float GameStateTime;
static ZL_String StageName;

static int tileCommands[CMD_COUNT] = { (int)TILE_CMD_NONE, (int)TILE_CMD_FORWARD, (int)TILE_CMD_REVERSE, (int)TILE_CMD_TURNLEFT, (int)TILE_CMD_TURNRIGHT };

static ZL_Color BackGradient[4], GradientColors[] = { ZLRGBX(0x051e3e), ZLRGBX(0x251e3e), ZLRGBX(0x451e3e), ZLRGBX(0x651e3e), ZLRGBX(0x851e3e) };

static const char* Boards[] =
{
"1" // Board 1
"#######"
"### ###"
"##   ##"
"#G   L#"
"##   ##"
"### ###"
"#######",

"3" // Board 2
"#####"
"#G  #"
"### #"
"#R  #"
"#####",

"2" // Board 3
"#####"
"## G#"
"#R  #"
"#  ##"
"#####",

"3" // Board 4
"######"
"##G ##"
"#    #"
"#    #"
"## D##"
"######",

"4" // Board 5
"#########"
"#R ###  #"
"##  #  ##"
"###   ###"
"###   ###"
"##  #  ##"
"#  ###  #"
"# #####G#"
"#########",

"5" // Board 6
"#########"
"#       #"
"#G### ###"
"#       #"
"# # #####"
"#   #R  #"
"# ### ###"
"#       #"
"#########",

"5" // Board 7
"##############"
"## # # # # # #"
"#      #     #"
"###### ##### #"
"#  #     #   #"
"## #     ### #"
"#D           #"
"## #     # # #"
"## #       # #"
"## # ##### # #"
"#            #"
"## ##### ###G#"
"#      #   # #"
"##############",

"5" // Board 8
"###########"
"# #    G  #"
"# # ### ###"
"#   #     #"
"# ###     #"
"# #       #"
"#####     #"
"#   #     #"
"# #L### ###"
"# #       #"
"###########",
	
"6" // Board 9
"##########"
"#      # #"
"#  #####G#"
"#      # #"
"##     # #"
"##       #"
"#R     ###"
"##       #"
"#### ### #"
"##########",

"6" // Board 10
"##############"
"####D# # #####"
"#            #"
"#### ### #####"
"#    ###     #"
"#### ### #####"
"#            #"
"#### ### #####"
"#            #"
"#### # # #####"
"#    # #     #"
"#### # # #####"
"#    # #    G#"
"##############",

"7" // Bonus 1
"#############"
"#  G        #"
"# # # ### ###"
"#           #"
"# ####### # #"
"#   #       #"
"# # #     # #"
"# #       # #"
"# # #  U  # #"
"#         # #"
"# # ### ### #"
"# #         #"
"#############",

"8" // Bonus 2
"################"
"#######G########"
"##### # ########"
"#####   ########"
"##### # ########"
"#         ######"
"#   # # ########"
"#           ####"
"# # # #     ####"
"#     #     ####"
"# # ###     ####"
"# # #       ####"
"# # ### # ######"
"#           ####"
"# # # L## # ####"
"################",
};

static const char* Board;
static int BoardIdx, BoardSize;
enum EBoard { BOARD_LAST_NORMAL = 9, BOARD_LAST_BONUS = 11 };
#define TILE(X,Y) Board[(BoardSize - 1 - Y) * BoardSize + X]

static void DrawTextBordered(const ZL_Vector& p, const char* txt, float scale = 1, const ZL_Color& colfill = ZLWHITE, const ZL_Color& colborder = ZLBLACK, int border = 2, ZL_Origin::Type origin = ZL_Origin::Center)
{
	for (int i = 0; i < 9; i++) if (i != 4) fntMain.Draw(p.x+(border*((i%3)-1)), p.y+8+(border*((i/3)-1)), txt, scale, scale, colborder, origin);
	fntMain.Draw(p.x  , p.y+8  , txt, scale, scale, colfill, origin);
}

static void SetState(EGameState state)
{
	GameState = state;
	GameStateTime = 0.0f;
	imcMusic.SetSongVolume(state == GAME_TITLE ? 100 : 60);
}

static struct SBot
{
	int StartPosX, StartPosY, StartDir, GoalX, GoalY;
	int PosX, PosY, Dir, NextPosX, NextPosY, NextDir;
	bool NextBonk, SpeedUp;
	float MoveDelta;
	float Animation;
	ECommand Commands[10];
	EBotState State;
	int CommandCount, CommandIndex;

	void RunCommand()
	{
		if (!NextBonk)
		{
			PosX = NextPosX;
			PosY = NextPosY;
			Dir = NextDir;
		}
		if (PosX == GoalX && PosY == GoalY)
		{
			return;
		}
		CommandIndex = (CommandIndex + 1) % CommandCount;
		int fwdDir = (((Dir%4)+4)%4), fwdX = (fwdDir == 0 ? 1 : (fwdDir == 2 ? -1 : 0)), fwdY = (fwdDir == 1 ? 1 : (fwdDir == 3 ? -1 : 0));
		switch (Commands[CommandIndex])
		{
			case CMD_NONE:      NextPosX = PosX;        NextPosY = PosY;        NextDir = Dir;     break;
			case CMD_FORWARD:   NextPosX = PosX + fwdX; NextPosY = PosY + fwdY; NextDir = Dir;     break;
			case CMD_REVERSE:   NextPosX = PosX - fwdX; NextPosY = PosY - fwdY; NextDir = Dir;     break;
			case CMD_TURNLEFT:  NextPosX = PosX;        NextPosY = PosY;        NextDir = Dir + 1; break;
			case CMD_TURNRIGHT: NextPosX = PosX;        NextPosY = PosY;        NextDir = Dir - 1; break;
			default:;
		}
		
		NextBonk = (NextPosX < 0 || NextPosX >= BoardSize || NextPosY < 0 || NextPosY >= BoardSize);
		if (!NextBonk) NextBonk = (TILE(NextPosX, NextPosY) == '#');
	}

	void Program()
	{
		NextPosX = PosX = StartPosX;
		NextPosY = PosY = StartPosY;
		NextDir  = Dir  = StartDir;
		CommandIndex = 0;
		MoveDelta = 0;
		State = BOT_PROGRAMMING;
	}

	void Run()
	{
		CommandIndex = CommandCount - 1;
		RunCommand();
		State = BOT_RUNNING;
	}

	void SetCommand(ECommand cmd)
	{
		Commands[CommandIndex] = cmd;
		CommandIndex = ((CommandIndex + 1) % CommandCount);
		sndSelect.Play();
	}

	void Update()
	{
		if (State != BOT_RUNNING) return;
		float speed = (SpeedUp ? 20.f : 5.f);
		float elapsed = ZLELAPSEDF(speed);
		MoveDelta += elapsed;
		if (MoveDelta > (Commands[CommandIndex] != CMD_NONE ? 1.f : .3f))
		{
			MoveDelta = 0;
			RunCommand();
			if (PosX == GoalX && PosY == GoalY)
			{
				State = BOT_CLEARED;
				SetState(GAME_CLEARSTAGE);
				sndClear.Play();
			}
			else if (Commands[CommandIndex] != CMD_NONE)
			{
				(NextBonk ? sndBump : sndMove).Play();
			}
		}
		if (PosX != GoalX || PosY != GoalY)
		{
			if (NextBonk && MoveDelta > .5f) elapsed = -elapsed;
			if (Commands[CommandIndex] == CMD_FORWARD) Animation += elapsed;
			if (Commands[CommandIndex] == CMD_REVERSE) Animation -= elapsed;
		}
	}

	void Draw()
	{
		float d = (NextBonk && MoveDelta > .5f ? 1.f - MoveDelta : MoveDelta);
		d = ZL_Easing::InOutSine(d);
		float x = ZL_Math::Lerp(s(PosX), s(NextPosX), d), y = ZL_Math::Lerp(s(PosY), s(NextPosY), d), a = ZL_Math::Lerp(s(Dir), s(NextDir), d);
		int tile = ((((int)(Animation*10)%3)+3)%3);
		srfBot.SetTilesetIndex(tile);
		srfBot.Draw(x+.55f, y+.45f, a * PIHALF, ZLLUMA(0, .5));
		srfBot.Draw(x+.5f, y+.5f, a * PIHALF);
	}
} Bot;

#if defined(ZILLALOG)
static void MakeBoard()
{
	int MAPW = 1+2*RAND_INT_RANGE(3,9), MAPH = MAPW;
	static std::vector<char> buf;
	buf.resize(1+MAPW*MAPH+1);
	char* Map = &buf[1];
	Map[MAPW*MAPH] = '\0';
	memset(Map, '#', MAPW*MAPH);

	int playerX = RAND_INT_RANGE(2, MAPW-3);
	int playerY = RAND_INT_RANGE(2, MAPH-3);
	Map[playerX*MAPW+playerY] = ' ';

	for (char empty = 0; empty < 2; empty++)
	{
		int currentx = MAPW/2|1, currenty = MAPH/2|1;
		for (int y = currenty - 2; y <= currenty + 2; y++)
			for (int x = currentx - 2; x <= currentx + 2; x++)
				Map[y*MAPW+x] = empty;
 
		REGENERATE:
		for (int i = 0; i != 100; i++)
		{
			int oldx = currentx, oldy = currenty;
			switch (RAND_INT_MAX(3))
			{
				case 0: if (currentx < MAPW-2) currentx += 2; break;
				case 1: if (currenty < MAPH-2) currenty += 2; break;
				case 2: if (currentx >      2) currentx -= 2; break;
				case 3: if (currenty >      2) currenty -= 2; break;
			}
			if (Map[currenty*MAPW+currentx] == empty) continue;
			Map[currenty*MAPW+currentx] = empty;
			Map[((currenty + oldy) / 2)*MAPW+((currentx + oldx) / 2)] = empty;
		}
 
		//check if all cells are visited
		for (int y = 1; y != MAPH; y += 2)
			for (int x = 1; x != MAPW; x += 2)
				if (Map[y*MAPW+x] > ' ') goto REGENERATE;
	}

	for (int i = 0; i != MAPW*MAPH; i++)
		if (Map[i] < ' ') Map[i] = ' ';

	Board = Map;
	BoardSize = MAPW;

	Bot.StartPosX = playerX;
	Bot.StartPosY = playerY;
	Bot.StartDir = RAND_INT_RANGE(0, 3);

	#define SETTILE(X,Y) Map[(BoardSize - 1 - Y) * BoardSize + X]
	SETTILE(playerX, playerY) = "RULD"[Bot.StartDir];

	for (int WantMinRange = MAPH; ; WantMinRange--)
	{
		for (int retry = 0; retry != 10000; retry++)
		{
			Bot.Program();
			for (ECommand& c : Bot.Commands) c = (ECommand)RAND_INT_RANGE(CMD_NONE+1, CMD_COUNT-1);
			Bot.Run();

			for (int step = 0; step != 1000; step++)
			{
				Bot.RunCommand();
				if (
					(((Bot.PosX != Bot.StartPosX) && (Bot.PosY != Bot.StartPosY)) || WantMinRange<=1) &&
					ZLV(playerX,playerY).GetDistance(ZLV(Bot.PosX,Bot.PosY)) >= WantMinRange)
				{
					SETTILE(Bot.PosX, Bot.PosY) = 'G';
					printf("\n\"%c\"", '0' + Bot.CommandCount);
					for (int i = 0; i != MAPH; i++)
					{
						printf("\n\"%.*s\"", MAPW, Map+i*MAPW);
					}
					printf(",\n\n\n");

					Bot.GoalX = Bot.PosX;
					Bot.GoalY = Bot.PosY;
					Bot.Program();
					return;
				}
			}
		}
	}
}

static void Bruteforce(int* out_retries = NULL, int* out_steps = NULL, int* out_commands = NULL)
{
	for (int retry = 1; retry != 100000; retry++)
	{
		Bot.Program();
		for (ECommand& c : Bot.Commands) c = (ECommand)RAND_INT_RANGE(CMD_NONE, CMD_COUNT-1);
		Bot.Run();

		for (int step = 1; step <= 1000; step++)
		{
			Bot.RunCommand();
			if (Bot.PosX == Bot.GoalX && Bot.PosY == Bot.GoalY)
			{
				if (!out_retries) printf("Solved after %d tries - takes %d steps\n", retry, step);
				Bot.Program();
				if (out_retries) *out_retries = retry;
				if (out_steps) *out_steps = step;
				if (out_commands) { *out_commands = 0; for (int i = 0; i != Bot.CommandCount; i++) if (Bot.Commands[i] != CMD_NONE) (*out_commands)++; }
				return;
			}
		}
	}
}

static void BruteStats()
{
	int n, totalRetries = 0, minRetries = 10000000, maxRetries = 0, minSteps = 10000000, maxSteps = 0, minCommands = 10000000, maxCommands = 0;
	for (n = 0; n != 10; n++)
	{
		int retries, steps, commands;
		Bruteforce(&retries, &steps, &commands);
		totalRetries += retries;
		if (retries < minRetries) minRetries = retries;
		if (retries > maxRetries) maxRetries = retries;
		if (steps < minSteps) minSteps = steps;
		if (steps > maxSteps) maxSteps = steps;
		if (commands < minCommands) minCommands = commands;
		if (commands > maxCommands) maxCommands = commands;
	}
	printf("Avg Retries: %d - Retries: %d ~ %d - Steps: %d ~ %d - Commands: %d ~ %d\n", totalRetries/n, minRetries, maxRetries, minSteps, maxSteps, minCommands, maxCommands);
}
#endif

static void SetBoard(int idx)
{
	Board = Boards[idx]+1;
	BoardSize = (int)(ssqrt((float)strlen(Board))+.4f);
	BoardIdx = idx;

	Bot.CommandCount = Boards[idx][0] - '0';
	memset(Bot.Commands, 0, sizeof(Bot.Commands));
	Bot.SpeedUp = false;
	for (int y = 0; y != BoardSize; y++)
	{
		for (int x = 0; x != BoardSize; x++)
		{
			switch (TILE(x, y))
			{
				case 'R': Bot.StartPosX = x; Bot.StartPosY = y; Bot.StartDir = 0; break;
				case 'U': Bot.StartPosX = x; Bot.StartPosY = y; Bot.StartDir = 1; break;
				case 'L': Bot.StartPosX = x; Bot.StartPosY = y; Bot.StartDir = 2; break;
				case 'D': Bot.StartPosX = x; Bot.StartPosY = y; Bot.StartDir = 3; break;
				case 'G': Bot.GoalX = x, Bot.GoalY = y; break;
			}
		}
	}
	Bot.Program();
	StageName = ZL_String::format("Stage %d", idx + 1);
	if (idx > BOARD_LAST_NORMAL) StageName = ZL_String::format("Bonus Stage %d", idx - BOARD_LAST_NORMAL);

	BackGradient[0] = RAND_ARRAYELEMENT(GradientColors);
	BackGradient[1] = RAND_ARRAYELEMENT(GradientColors);
	BackGradient[2] = RAND_ARRAYELEMENT(GradientColors);
	BackGradient[3] = RAND_ARRAYELEMENT(GradientColors);
}

static void Load()
{
	fntMain = ZL_Font("Data/typomoderno.ttf.zip", 25.f);
	fntBig = ZL_Font("Data/typomoderno.ttf.zip", 150.f).SetCharSpacing(15);
	srfTiles = ZL_Surface("Data/gfx.png").SetTilesetClipping(4, 4);
	srfBot = srfTiles.Clone().SetOrigin(ZL_Origin::Center).SetScale(1/64.f, 1/64.f);


	extern TImcSongData imcDataIMCSELECT; sndSelect = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCSELECT);
	extern TImcSongData imcDataIMCRUN;    sndRun    = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCRUN);
	extern TImcSongData imcDataIMCRETURN; sndReturn = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCRETURN);
	extern TImcSongData imcDataIMCCLEAR;  sndClear  = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCCLEAR);
	extern TImcSongData imcDataIMCSTAGE;  sndStage  = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCSTAGE);
	extern TImcSongData imcDataIMCMOVE;   sndMove   = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCMOVE);
	extern TImcSongData imcDataIMCBUMP;   sndBump   = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCBUMP);
	extern TImcSongData imcDataIMCMUSIC;  imcMusic  = ZL_SynthImcTrack(&imcDataIMCMUSIC);

	SetBoard(0);
	imcMusic.Play();
}

static void Draw()
{
	if (ZL_Input::Down(ZLK_ESCAPE))
	{
		ZL_Application::Quit(0);
		return;
	}
	if (GameState >= GAME_STAGEFADEIN && GameState <= GAME_PLAY)
	{
		if (Bot.State == BOT_PROGRAMMING)
		{
			#if defined(ZILLALOG)
			if (ZL_Input::Down(ZLK_F1))  { Bot.CommandCount =  1; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F2))  { Bot.CommandCount =  2; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F3))  { Bot.CommandCount =  3; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F4))  { Bot.CommandCount =  4; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F5))  { Bot.CommandCount =  5; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F6))  { Bot.CommandCount =  6; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F7))  { Bot.CommandCount =  7; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F8))  { Bot.CommandCount =  8; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F9))  { Bot.CommandCount =  9; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F10)) { Bot.CommandCount = 10; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F11)) { Bot.CommandCount = 11; MakeBoard(); }
			if (ZL_Input::Down(ZLK_F12)) { Bot.CommandCount = 12; MakeBoard(); }
			if (ZL_Input::Down(ZLK_1)) { SetBoard(0); }
			if (ZL_Input::Down(ZLK_2)) { SetBoard(1); }
			if (ZL_Input::Down(ZLK_3)) { SetBoard(2); }
			if (ZL_Input::Down(ZLK_4)) { SetBoard(3); }
			if (ZL_Input::Down(ZLK_5)) { SetBoard(4); }
			if (ZL_Input::Down(ZLK_6)) { SetBoard(5); }
			if (ZL_Input::Down(ZLK_7)) { SetBoard(6); }
			if (ZL_Input::Down(ZLK_8)) { SetBoard(7); }
			if (ZL_Input::Down(ZLK_9)) { SetBoard(8); }
			if (ZL_Input::Down(ZLK_0)) { SetBoard(9); }
			if (ZL_Input::Down(ZLK_F)) Bruteforce();
			if (ZL_Input::Down(ZLK_S)) BruteStats();
			#endif

			if (ZL_Input::Down(ZLK_UP)     || ZL_Input::Down(ZLK_W)    ) Bot.SetCommand(CMD_FORWARD);
			if (ZL_Input::Down(ZLK_DOWN)   || ZL_Input::Down(ZLK_S)    ) Bot.SetCommand(CMD_REVERSE);
			if (ZL_Input::Down(ZLK_LEFT)   || ZL_Input::Down(ZLK_A)    ) Bot.SetCommand(CMD_TURNLEFT);
			if (ZL_Input::Down(ZLK_RIGHT)  || ZL_Input::Down(ZLK_D)    ) Bot.SetCommand(CMD_TURNRIGHT);
			if (ZL_Input::Down(ZLK_DELETE) || ZL_Input::Down(ZLK_SPACE)) Bot.SetCommand(CMD_NONE);

			if (ZL_Input::Down(ZLK_RETURN)) { Bot.Run(); sndRun.Play(); }
		}
		else if (Bot.State == BOT_RUNNING)
		{
			if (ZL_Input::Down(ZLK_RETURN)) { Bot.Program(); sndReturn.Play(); }
		}
	}

	if (ZL_Input::Down(ZLK_LSHIFT) || ZL_Input::Down(ZLK_RSHIFT)) Bot.SpeedUp = true;
	if (ZL_Input::Up(ZLK_LSHIFT) || ZL_Input::Up(ZLK_RSHIFT)) Bot.SpeedUp = false;

	Bot.Update();

	ZL_Display::FillGradient(0, 0, ZLWIDTH, ZLHEIGHT, BackGradient[0], BackGradient[1], BackGradient[2], BackGradient[3]);

	float boardSize = MIN(ZLFROMH(100), ZLFROMW(20));
	ZL_Rectf boardRect = ZL_Rectf::FromCenter(ZLHALFW, ZLHALFH + 40, boardSize/2, boardSize/2);
	ZL_Rectf screenRect = ZL_Rectf(0, 0, ZLWIDTH, ZLHEIGHT);

	ZL_Display::FillRect(boardRect+3, ZLBLACK);

	float x0 = BoardSize * ((       0-boardRect.left) / boardRect.Width());
	float x1 = BoardSize * (( ZLWIDTH-boardRect.left) / boardRect.Width());
	float y0 = BoardSize * ((       0-boardRect.low)  / boardRect.Height());
	float y1 = BoardSize * ((ZLHEIGHT-boardRect.low)  / boardRect.Height());
	ZL_Display::PushOrtho(x0, x1, y0, y1);

	srfTiles.BatchRenderBegin();
	srfTiles.SetTilesetIndex(TILE_FLOOR).DrawTo(0, 0, s(BoardSize), s(BoardSize));
	for (int y = 0; y != BoardSize; y++)
		for (int x = 0; x != BoardSize; x++)
			srfTiles.SetTilesetIndex(TILE_FLOOR).DrawTo(s(x), s(y), s(x+1), s(y+1));

	for (int y = 0; y != BoardSize; y++)
		for (int x = 0; x != BoardSize; x++)
			if (TILE(x, y) == '#')
				srfTiles.SetTilesetIndex(TILE_SHADOW).DrawTo(s(x)-.05f, s(y)-.05f, s(x)+1.05f, s(y)+1.05f);

	for (int y = 0; y != BoardSize; y++)
	{
		for (int x = 0; x != BoardSize; x++)
		{
			float fx = s(x), fy = s(y);
			switch (TILE(x, y))
			{
				case '#': srfTiles.SetTilesetIndex(TILE_WALL).DrawTo(fx, fy, fx+1, fy+1); break;
				case 'G': srfTiles.SetTilesetIndex(TILE_FLAG).DrawTo(fx, fy, fx+1, fy+1); break;
			}
		}
	}
	srfTiles.BatchRenderEnd();

	//for (int i = 0; i <= BoardSize; i++)
	//	ZL_Display::FillWideLine(0, s(i), s(BoardSize), s(i), .005f, ZLWHITE),
	//	ZL_Display::FillWideLine(s(i), 0, s(i), s(BoardSize), .005f, ZLWHITE);

	Bot.Draw();

	ZL_Display::PopOrtho();

	for (int i = 0; i != Bot.CommandCount; i++)
	{
		float x = ZLHALFW + (i - (Bot.CommandCount * .5f)) * 75;
		ZL_Rectf commandBox(x, 15, x + 65, 15+65);
		if (i == Bot.CommandIndex)
			ZL_Display::FillRect(commandBox+3, ZL_Color::White);
		srfTiles.SetTilesetIndex(tileCommands[Bot.Commands[i]]).DrawTo(commandBox);
		ZL_Display::DrawRect(commandBox, ZLWHITE);
		if (GameState >= GAME_STAGEFADEIN && GameState <= GAME_PLAY && Bot.State == BOT_PROGRAMMING && ZL_Input::Clicked(commandBox))
		{
			Bot.CommandIndex = i;
		}
	}

	float panelLeft = ZLHALFW + (0 - (Bot.CommandCount * .5f)) * 75;
	fntMain.Draw(panelLeft - 10,15+65*.5f, (Bot.State == BOT_PROGRAMMING ? "Programming" : "Running"), ZL_Origin::CenterRight);

	for (int i = 0; i != CMD_COUNT; i++)
	{
		float y = ZLHALFH + 100 + (i - (CMD_COUNT * .5f)) * 75;
		ZL_Rectf commandBox(boardRect.right + 15, y, boardRect.right + 15 + 65, y+65);
		srfTiles.SetTilesetIndex(tileCommands[i]).DrawTo(commandBox);
		ZL_Display::DrawRect(commandBox, ZLWHITE);
		if (GameState >= GAME_STAGEFADEIN && GameState <= GAME_PLAY && Bot.State == BOT_PROGRAMMING && ZL_Input::Clicked(commandBox))
		{
			Bot.SetCommand((ECommand)i);
		}
	}

	{
		float y = ZLHALFH + 100 - (CMD_COUNT * .5f) * 75 - 1 * 45;
		ZL_Rectf commandBox(boardRect.right + 15, y, boardRect.right + 15 + 65, y+35);
		ZL_Display::DrawRect(commandBox, ZLWHITE, ZLBLACK);
		fntMain.Draw(commandBox.Center()+ZLV(0,9), (Bot.State == BOT_PROGRAMMING ? "START" : "STOP"), .70f, ZL_Origin::Center);
		fntMain.Draw(commandBox.Center()-ZLV(0,9), "PROGRAM", .70f, ZL_Origin::Center);
		if (GameState >= GAME_STAGEFADEIN && GameState <= GAME_PLAY && Bot.State != BOT_CLEARED && ZL_Input::Clicked(commandBox))
		{
			if (Bot.State == BOT_RUNNING) { Bot.Program(); sndReturn.Play(); }
			else { Bot.Run(); sndRun.Play(); }
		}
	}

	{
		float y = ZLHALFH + 100 - (CMD_COUNT * .5f) * 75 - 2 * 45;
		ZL_Rectf commandBox(boardRect.right + 15, y, boardRect.right + 15 + 65, y+35);
		ZL_Display::DrawRect(commandBox, ZLWHITE, ZLBLACK);
		fntMain.Draw(commandBox.Center()+ZLV(0,9), (Bot.SpeedUp ? "HIGH"  : "REGULAR"), .70f, ZL_Origin::Center);
		fntMain.Draw(commandBox.Center()-ZLV(0,9), (Bot.SpeedUp ? "SPEED" : "SPEED"), .70f, ZL_Origin::Center);
		if (GameState >= GAME_STAGEFADEIN && GameState <= GAME_PLAY && ZL_Input::Clicked(commandBox))
		{
			Bot.SpeedUp ^= true;
		}
	}

	ZL_Display::PushMatrix();
	ZL_Display::Translate(boardRect.left - 20.f, boardRect.MidY());
	ZL_Display::Rotate(PIHALF);
	DrawTextBordered(ZLV(0, 0), StageName);
	ZL_Display::PopMatrix();

	if (GameState == GAME_PLAY) { }
	else if (GameState == GAME_BOOT)
	{
		SetState(GAME_TITLE);
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, 1));
	}
	else if (GameState == GAME_TITLE)
	{
		GameStateTime += ZLELAPSED;
		float a = 1 - ZL_Math::Clamp01(GameStateTime * .8f);
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, .7f+a*.3f));
		ZL_Display::PushMatrix();
		ZL_Display::Translate(ZL_Display::Center());
		ZL_Display::Rotate(ZLTICKS*.001f);
		for (int i = 0; i < 100; i++)
		{
			float x = (5000 - ((ZLTICKS + i * 50) % 5000)) / 5000.f;
			ZL_Display::Rotate(.1f + (ZLTICKS * .00001f));
			srfBot.Draw(ZLV(x * ZLHALFW*1.7f, 0), x * 20.f, x * 4.f, x * 4.f, ZLLUMA(.5,.5));
		}
		ZL_Display::PopMatrix();
		ZL_Display::PushMatrix();
		ZL_Display::Translate(ZLV(ZLHALFW, ZLHALFH+100));
		for (int i = 0; i < 10; i++)
		{
			ZL_Display::PushMatrix();
			ZL_Display::Rotate(RAND_RANGE(-.1f, .1f));
			fntBig.Draw(0, 0, "BOTLOOP", 1.35f, 1.35f, ZLLUMA(0, .3f), ZL_Origin::Center);
			ZL_Display::PopMatrix();
		}
		ZL_Display::PopMatrix();
		fntBig.Draw(ZLV(ZLHALFW, ZLHALFH+100), "BOTLOOP", 1.25f, 1.25f, ZLHSV(smod(ZLTICKS*.001f,1),.2f,1), ZL_Origin::Center);

		DrawTextBordered(ZLV(ZLHALFW, ZLHALFH-100), "Click on the command panel on the right side of the screen to program the bot");
		DrawTextBordered(ZLV(ZLHALFW, ZLHALFH-140), "Alternatively you can use the arrow keys/space/enter");
		DrawTextBordered(ZLV(ZLHALFW, ZLHALFH-190), "Click or press any key to start!");
		DrawTextBordered(ZLV(ZLHALFW, ZLHALFH-250), "[ALT+ENTER] Toggle Fullscreen", .75f);
		DrawTextBordered(ZL_Vector(18, 12), "(C) 2020 Bernhard Schelling", 1, ZLRGBA(1,.9,.5,.5), ZLBLACK, 2, ZL_Origin::BottomLeft);
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, a));
		if (ZL_Input::Down(ZLK_ESCAPE))
			ZL_Application::Quit(0);
		else if (a <= .5f && (ZL_Input::KeyDownCount() || ZL_Input::Clicked()))
		{
			SetState(GAME_STAGEFADEIN);
			sndStage.Play();
		}
	}
	else if (GameState == GAME_STAGEFADEIN)
	{
		GameStateTime += ZLELAPSED;
		float a = 1 - ZL_Math::Clamp01(GameStateTime * 5.f);
		if (BoardIdx == 0) a *= .7f;
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, a));
		if (a <= .01f) SetState(GAME_STAGENAME);
	}
	else if (GameState == GAME_STAGENAME)
	{
		GameStateTime += ZLELAPSED;
		float a = 1 - ZL_Math::Clamp01(GameStateTime * .5f);
		if (a < .5f) a = ZL_Easing::OutCubic(a*2)*.5f;
		else         a = ZL_Easing::InCubic((a-.5f)*2)*.5f+.5f;
		DrawTextBordered(ZLV(-100.f+a*(ZLWIDTH+200.f), ZLHALFH), StageName, 2);
		if (a <= .01f) SetState(GAME_PLAY);
	}
	else if (GameState == GAME_CLEARSTAGE)
	{
		GameStateTime += ZLELAPSED;
		float a = ZL_Math::Clamp01(GameStateTime * .5f);
		if (a < .5f) a = ZL_Easing::OutCubic(a*2)*.5f;
		else         a = ZL_Easing::InCubic((a-.5f)*2)*.5f+.5f;
		DrawTextBordered(ZLV(-100.f+a*(ZLWIDTH+200.f), ZLHALFH), "SUCCESS!", 2);
		if (a >= .99f) SetState(BoardIdx == BOARD_LAST_NORMAL || BoardIdx == BOARD_LAST_BONUS ? GAME_CLEARALL : GAME_STAGEFADEOUT);
	}
	else if (GameState == GAME_CLEARALL)
	{
		GameStateTime += ZLELAPSED;
		float a = 1 - ZL_Easing::OutCubic(ZL_Math::Clamp01(GameStateTime * .5f));
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, .5f-a*.5f));
		DrawTextBordered(ZLV(ZLHALFW+a*(ZLHALFW+200.f), ZLFROMH(180)), (BoardIdx == BOARD_LAST_NORMAL ? "ALL STAGES CLEARED!" : "CONGRATULATION!!"), 2);
		DrawTextBordered(ZLV(ZLHALFW+a*(ZLHALFW+500.f), ZLFROMH(250)), "THANK YOU FOR PLAYING", 2);
		DrawTextBordered(ZLV(ZLHALFW+a*(ZLHALFW+900.f),          250), (BoardIdx == BOARD_LAST_NORMAL ? "Press any key for two bonus stage!" : "Press any key to restart"), 2);
		if (a <= .01f && (ZL_Input::KeyDownCount() || ZL_Input::Clicked()))
		{
			if (BoardIdx != BOARD_LAST_NORMAL) SetBoard(0);
			SetState(BoardIdx == BOARD_LAST_NORMAL ? GAME_STAGEFADEOUT : GAME_TITLE);
		}
	}
	else if (GameState == GAME_STAGEFADEOUT)
	{
		GameStateTime += ZLELAPSED;
		float a = ZL_Math::Clamp01(GameStateTime * 5.f);
		if (BoardIdx == BOARD_LAST_NORMAL) a = .5f + a * .5f;
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, a));
		if (a >= .99f)
		{
			SetBoard(BoardIdx + 1);
			SetState(GAME_STAGEFADEIN);
			sndStage.Play();
		}
	}
}

static struct sBotloop : public ZL_Application
{
	sBotloop() : ZL_Application(60) { }

	virtual void Load(int argc, char *argv[])
	{
		if (!ZL_Application::LoadReleaseDesktopDataBundle()) return;
		if (!ZL_Display::Init("BOTLOOP", 1280, 720, ZL_DISPLAY_ALLOWRESIZEHORIZONTAL)) return;
		ZL_Display::ClearFill(ZL_Color::White);
		ZL_Display::SetAA(true);
		ZL_Audio::Init();
		ZL_Input::Init();
		::Load();
	}

	virtual void AfterFrame()
	{
		::Draw();
	}
} Botloop;

// ------------------------ AUDIO ------------------------------------------------
static const unsigned int IMCSELECT_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCSELECT_PatternData[] = {
	0x70, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCSELECT_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCSELECT_EnvList[] = {
	{ 0, 256, 65, 8, 16, 4, true, 255, },
	{ 0, 256, 370, 8, 12, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCSELECT_EnvCounterList[] = {
	{ 0, 0, 256 }, { 1, 0, 256 }, { -1, -1, 256 },
};
static const TImcSongOscillator IMCSELECT_OscillatorList[] = {
	{ 9, 66, IMCSONGOSCTYPE_SQUARE, 0, -1, 126, 1, 2 },
	{ 7, 66, IMCSONGOSCTYPE_SAW, 0, 0, 242, 2, 2 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static unsigned char IMCSELECT_ChannelVol[8] = { 51, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCSELECT_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCSELECT_ChannelStopNote[8] = { false, false, false, false, false, false, false, false };
TImcSongData imcDataIMCSELECT = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2594, /*ENVLISTSIZE*/ 2, /*ENVCOUNTERLISTSIZE*/ 3, /*OSCLISTSIZE*/ 9, /*EFFECTLISTSIZE*/ 0, /*VOL*/ 62,
	IMCSELECT_OrderTable, IMCSELECT_PatternData, IMCSELECT_PatternLookupTable, IMCSELECT_EnvList, IMCSELECT_EnvCounterList, IMCSELECT_OscillatorList, NULL,
	IMCSELECT_ChannelVol, IMCSELECT_ChannelEnvCounter, IMCSELECT_ChannelStopNote };


static const unsigned int IMCRUN_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCRUN_PatternData[] = {
	0x60, 0x60, 0x62, 0x64, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCRUN_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCRUN_EnvList[] = {
	{ 0, 256, 65, 8, 16, 4, true, 255, },
	{ 0, 256, 370, 8, 12, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCRUN_EnvCounterList[] = {
	{ 0, 0, 256 }, { 1, 0, 256 }, { -1, -1, 256 },
};
static const TImcSongOscillator IMCRUN_OscillatorList[] = {
	{ 9, 66, IMCSONGOSCTYPE_SQUARE, 0, -1, 126, 1, 2 },
	{ 7, 66, IMCSONGOSCTYPE_SAW, 0, 0, 242, 2, 2 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static unsigned char IMCRUN_ChannelVol[8] = { 51, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCRUN_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCRUN_ChannelStopNote[8] = { false, false, false, false, false, false, false, false };
TImcSongData imcDataIMCRUN = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2594, /*ENVLISTSIZE*/ 2, /*ENVCOUNTERLISTSIZE*/ 3, /*OSCLISTSIZE*/ 9, /*EFFECTLISTSIZE*/ 0, /*VOL*/ 62,
	IMCRUN_OrderTable, IMCRUN_PatternData, IMCRUN_PatternLookupTable, IMCRUN_EnvList, IMCRUN_EnvCounterList, IMCRUN_OscillatorList, NULL,
	IMCRUN_ChannelVol, IMCRUN_ChannelEnvCounter, IMCRUN_ChannelStopNote };


static const unsigned int IMCRETURN_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCRETURN_PatternData[] = {
	0x47, 0x47, 0x45, 0x44, 0x42, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCRETURN_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCRETURN_EnvList[] = {
	{ 0, 256, 65, 8, 16, 4, true, 255, },
	{ 0, 256, 370, 8, 12, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCRETURN_EnvCounterList[] = {
	{ 0, 0, 256 }, { 1, 0, 256 }, { -1, -1, 256 },
};
static const TImcSongOscillator IMCRETURN_OscillatorList[] = {
	{ 9, 66, IMCSONGOSCTYPE_SQUARE, 0, -1, 126, 1, 2 },
	{ 7, 66, IMCSONGOSCTYPE_SAW, 0, 0, 242, 2, 2 },
};
static unsigned char IMCRETURN_ChannelVol[8] = { 51, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCRETURN_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCRETURN_ChannelStopNote[8] = { false, false, false, false, false, false, false, false };
TImcSongData imcDataIMCRETURN = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2594, /*ENVLISTSIZE*/ 2, /*ENVCOUNTERLISTSIZE*/ 3, /*OSCLISTSIZE*/ 2, /*EFFECTLISTSIZE*/ 0, /*VOL*/ 100,
	IMCRETURN_OrderTable, IMCRETURN_PatternData, IMCRETURN_PatternLookupTable, IMCRETURN_EnvList, IMCRETURN_EnvCounterList, IMCRETURN_OscillatorList, NULL,
	IMCRETURN_ChannelVol, IMCRETURN_ChannelEnvCounter, IMCRETURN_ChannelStopNote };


static const unsigned int IMCCLEAR_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCCLEAR_PatternData[] = {
	0x50, 0x50, 0x54, 0x57, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCCLEAR_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCCLEAR_EnvList[] = {
	{ 0, 256, 272, 25, 31, 255, true, 255, },
	{ 0, 256, 152, 8, 16, 255, true, 255, },
	{ 0, 256, 173, 8, 16, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCCLEAR_EnvCounterList[] = {
	{ 0, 0, 2 }, { -1, -1, 256 }, { 1, 0, 256 }, { 2, 0, 256 },
	{ 2, 0, 256 },
};
static const TImcSongOscillator IMCCLEAR_OscillatorList[] = {
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 100, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 66, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 24, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 88, 2, 1 },
	{ 10, 0, IMCSONGOSCTYPE_SQUARE, 0, -1, 62, 3, 1 },
	{ 9, 0, IMCSONGOSCTYPE_SQUARE, 0, -1, 34, 4, 1 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, 1, 36, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_NOISE, 0, 3, 14, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static const TImcSongEffect IMCCLEAR_EffectList[] = {
	{ 226, 173, 1, 0, IMCSONGEFFECTTYPE_RESONANCE, 1, 1 },
	{ 204, 0, 1, 0, IMCSONGEFFECTTYPE_LOWPASS, 1, 0 },
	{ 10795, 655, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 1 },
	{ 51, 0, 15876, 0, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
};
static unsigned char IMCCLEAR_ChannelVol[8] = { 97, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCCLEAR_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCCLEAR_ChannelStopNote[8] = { true, false, false, false, false, false, false, false };
TImcSongData imcDataIMCCLEAR = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 5292, /*ENVLISTSIZE*/ 3, /*ENVCOUNTERLISTSIZE*/ 5, /*OSCLISTSIZE*/ 15, /*EFFECTLISTSIZE*/ 4, /*VOL*/ 100,
	IMCCLEAR_OrderTable, IMCCLEAR_PatternData, IMCCLEAR_PatternLookupTable, IMCCLEAR_EnvList, IMCCLEAR_EnvCounterList, IMCCLEAR_OscillatorList, IMCCLEAR_EffectList,
	IMCCLEAR_ChannelVol, IMCCLEAR_ChannelEnvCounter, IMCCLEAR_ChannelStopNote };


static const unsigned int IMCSTAGE_OrderTable[] = {
	0x010000100,
};
static const unsigned char IMCSTAGE_PatternData[] = {
	0x50, 0x54, 0x50, 0x52, 0x54, 0x50, 0x54, 0x57, 255, 0, 0, 0, 0, 0, 0, 0,
	0x39, 0, 0x39, 0, 0x39, 0, 0x39, 0, 0x39, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCSTAGE_PatternLookupTable[] = { 0, 0, 0, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCSTAGE_EnvList[] = {
	{ 0, 256, 97, 8, 16, 0, true, 255, },
	{ 0, 256, 379, 8, 15, 255, true, 255, },
	{ 0, 256, 523, 8, 16, 255, true, 255, },
	{ 32, 256, 196, 8, 16, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCSTAGE_EnvCounterList[] = {
	{ 0, 2, 256 }, { -1, -1, 256 }, { 1, 2, 254 }, { 2, 7, 256 },
	{ 3, 7, 256 },
};
static const TImcSongOscillator IMCSTAGE_OscillatorList[] = {
	{ 8, 0, IMCSONGOSCTYPE_SAW, 2, -1, 168, 1, 1 },
	{ 10, 0, IMCSONGOSCTYPE_SAW, 2, -1, 96, 1, 1 },
	{ 7, 127, IMCSONGOSCTYPE_SAW, 2, -1, 158, 2, 1 },
	{ 6, 0, IMCSONGOSCTYPE_SINE, 2, 0, 16, 1, 1 },
	{ 6, 0, IMCSONGOSCTYPE_SINE, 2, 1, 50, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, 2, 100, 1, 1 },
	{ 8, 0, IMCSONGOSCTYPE_NOISE, 7, -1, 127, 1, 4 },
};
static const TImcSongEffect IMCSTAGE_EffectList[] = {
	{ 154, 49, 1, 2, IMCSONGEFFECTTYPE_RESONANCE, 1, 1 },
	{ 122, 0, 6615, 7, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 255, 156, 1, 7, IMCSONGEFFECTTYPE_RESONANCE, 1, 1 },
	{ 227, 0, 1, 7, IMCSONGEFFECTTYPE_HIGHPASS, 1, 0 },
};
static unsigned char IMCSTAGE_ChannelVol[8] = { 71, 84, 163, 104, 100, 140, 194, 212 };
static const unsigned char IMCSTAGE_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 3 };
static const bool IMCSTAGE_ChannelStopNote[8] = { true, true, false, true, false, true, true, true };
TImcSongData imcDataIMCSTAGE = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 6615, /*ENVLISTSIZE*/ 4, /*ENVCOUNTERLISTSIZE*/ 5, /*OSCLISTSIZE*/ 7, /*EFFECTLISTSIZE*/ 4, /*VOL*/ 49,
	IMCSTAGE_OrderTable, IMCSTAGE_PatternData, IMCSTAGE_PatternLookupTable, IMCSTAGE_EnvList, IMCSTAGE_EnvCounterList, IMCSTAGE_OscillatorList, IMCSTAGE_EffectList,
	IMCSTAGE_ChannelVol, IMCSTAGE_ChannelEnvCounter, IMCSTAGE_ChannelStopNote };


static const unsigned int IMCBUMP_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCBUMP_PatternData[] = {
	0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCBUMP_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCBUMP_EnvList[] = {
	{ 0, 256, 3, 7, 255, 255, true, 255, },
	{ 0, 256, 184, 23, 255, 255, true, 255, },
	{ 0, 256, 8, 8, 16, 255, true, 255, },
	{ 0, 256, 64, 8, 16, 255, true, 255, },
	{ 0, 256, 64, 5, 19, 255, true, 255, },
	{ 0, 256, 64, 8, 255, 255, true, 255, },
	{ 0, 256, 173, 1, 23, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCBUMP_EnvCounterList[] = {
	{ -1, -1, 256 }, { 0, 0, 254 }, { 1, 0, 2 }, { 2, 0, 256 },
	{ 1, 0, 2 }, { 3, 0, 256 }, { 4, 0, 238 }, { 5, 0, 256 },
	{ 6, 0, 158 },
};
static const TImcSongOscillator IMCBUMP_OscillatorList[] = {
	{ 6, 31, IMCSONGOSCTYPE_SAW, 0, -1, 178, 0, 1 },
	{ 7, 36, IMCSONGOSCTYPE_SAW, 0, -1, 60, 0, 3 },
	{ 6, 66, IMCSONGOSCTYPE_SINE, 0, -1, 148, 5, 6 },
	{ 7, 0, IMCSONGOSCTYPE_NOISE, 0, 0, 48, 2, 0 },
	{ 7, 0, IMCSONGOSCTYPE_NOISE, 0, 1, 136, 4, 0 },
	{ 7, 31, IMCSONGOSCTYPE_SINE, 0, 2, 104, 7, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },
};
static const TImcSongEffect IMCBUMP_EffectList[] = {
	{ 236, 134, 1, 0, IMCSONGEFFECTTYPE_RESONANCE, 8, 0 },
};
static unsigned char IMCBUMP_ChannelVol[8] = { 171, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCBUMP_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCBUMP_ChannelStopNote[8] = { true, false, false, false, false, false, false, false };
TImcSongData imcDataIMCBUMP = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 5512, /*ENVLISTSIZE*/ 7, /*ENVCOUNTERLISTSIZE*/ 9, /*OSCLISTSIZE*/ 13, /*EFFECTLISTSIZE*/ 1, /*VOL*/ 30,
	IMCBUMP_OrderTable, IMCBUMP_PatternData, IMCBUMP_PatternLookupTable, IMCBUMP_EnvList, IMCBUMP_EnvCounterList, IMCBUMP_OscillatorList, IMCBUMP_EffectList,
	IMCBUMP_ChannelVol, IMCBUMP_ChannelEnvCounter, IMCBUMP_ChannelStopNote };


static const unsigned int IMCMOVE_OrderTable[] = {
	0x000000001,
};
static const unsigned char IMCMOVE_PatternData[] = {
	0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const unsigned char IMCMOVE_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCMOVE_EnvList[] = {
	{ 0, 150, 144, 32, 24, 255, true, 255, },
	{ 128, 256, 1, 8, 16, 255, true, 255, },
	{ 128, 256, 174, 8, 16, 255, true, 255, },
	{ 0, 256, 697, 8, 16, 16, true, 255, },
};
static TImcSongEnvelopeCounter IMCMOVE_EnvCounterList[] = {
	{ 0, 0, 75 }, { -1, -1, 256 }, { 1, 0, 256 }, { 2, 0, 256 },
	{ 3, 0, 256 },
};
static const TImcSongOscillator IMCMOVE_OscillatorList[] = {
	{ 5, 48, IMCSONGOSCTYPE_SINE, 0, -1, 160, 1, 2 },
	{ 8, 0, IMCSONGOSCTYPE_NOISE, 0, -1, 26, 4, 1 },
	{ 5, 15, IMCSONGOSCTYPE_NOISE, 0, 0, 22, 1, 3 },
};
static const TImcSongEffect IMCMOVE_EffectList[] = {
	{ 96, 0, 1, 0, IMCSONGEFFECTTYPE_LOWPASS, 1, 0 },
	{ 12573, 664, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 1 },
};
static unsigned char IMCMOVE_ChannelVol[8] = { 84, 84, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCMOVE_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCMOVE_ChannelStopNote[8] = { true, true, false, false, false, false, false, false };
TImcSongData imcDataIMCMOVE = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 6615, /*ENVLISTSIZE*/ 4, /*ENVCOUNTERLISTSIZE*/ 5, /*OSCLISTSIZE*/ 3, /*EFFECTLISTSIZE*/ 2, /*VOL*/ 100,
	IMCMOVE_OrderTable, IMCMOVE_PatternData, IMCMOVE_PatternLookupTable, IMCMOVE_EnvList, IMCMOVE_EnvCounterList, IMCMOVE_OscillatorList, IMCMOVE_EffectList,
	IMCMOVE_ChannelVol, IMCMOVE_ChannelEnvCounter, IMCMOVE_ChannelStopNote };


static const unsigned int IMCMUSIC_OrderTable[] = {
	0x000000001, 0x000000001, 0x010000001, 0x010000001, 0x010000002, 0x010000001, 0x010000001,
};
static const unsigned char IMCMUSIC_PatternData[] = {
	0x50, 0, 0x52, 0, 0x54, 0, 0x55, 0, 0x50, 0, 0x52, 0, 0x57, 0, 0x55, 0,
	0x60, 0x60, 0x5B, 0x59, 0, 0, 0, 0, 0x5B, 0x5B, 0x59, 0x57, 0, 0, 0, 0,
	0x50, 0, 0, 0, 0x62, 0, 0, 0, 0x64, 0, 0, 0, 0x50, 0, 0, 0,
};
static const unsigned char IMCMUSIC_PatternLookupTable[] = { 0, 2, 2, 2, 2, 2, 2, 2, };
static const TImcSongEnvelope IMCMUSIC_EnvList[] = {
	{ 0, 256, 87, 8, 16, 255, true, 255, },
	{ 196, 256, 29, 8, 16, 255, true, 255, },
	{ 0, 256, 173, 8, 16, 255, true, 255, },
	{ 196, 256, 31, 8, 16, 255, true, 255, },
	{ 0, 128, 1046, 8, 16, 255, true, 255, },
	{ 0, 256, 379, 8, 16, 255, true, 255, },
	{ 32, 256, 196, 8, 16, 255, true, 255, },
};
static TImcSongEnvelopeCounter IMCMUSIC_EnvCounterList[] = {
	{ -1, -1, 256 }, { 0, 0, 256 }, { 1, 0, 256 }, { 2, 0, 256 },
	{ 3, 0, 256 }, { 0, 0, 256 }, { 4, 0, 128 }, { 5, 7, 256 },
	{ 6, 7, 256 },
};
static const TImcSongOscillator IMCMUSIC_OscillatorList[] = {
	{ 5, 0, IMCSONGOSCTYPE_SINE, 0, -1, 98, 1, 2 },
	{ 6, 0, IMCSONGOSCTYPE_SINE, 0, -1, 98, 3, 4 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 0, 5, 6 },
	{ 8, 0, IMCSONGOSCTYPE_NOISE, 7, -1, 127, 0, 8 },
};
static const TImcSongEffect IMCMUSIC_EffectList[] = {
	{ 6350, 913, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 0 },
	{ 44, 0, 49611, 0, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 128, 0, 16537, 7, IMCSONGEFFECTTYPE_DELAY, 0, 0 },
	{ 255, 110, 1, 7, IMCSONGEFFECTTYPE_RESONANCE, 0, 0 },
	{ 227, 0, 1, 7, IMCSONGEFFECTTYPE_HIGHPASS, 0, 0 },
};
static unsigned char IMCMUSIC_ChannelVol[8] = { 225, 100, 100, 100, 100, 100, 100, 192 };
static const unsigned char IMCMUSIC_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 7 };
static const bool IMCMUSIC_ChannelStopNote[8] = { true, false, false, false, false, false, false, true };
TImcSongData imcDataIMCMUSIC = {
	/*LEN*/ 0x7, /*ROWLENSAMPLES*/ 16537, /*ENVLISTSIZE*/ 7, /*ENVCOUNTERLISTSIZE*/ 9, /*OSCLISTSIZE*/ 4, /*EFFECTLISTSIZE*/ 5, /*VOL*/ 100,
	IMCMUSIC_OrderTable, IMCMUSIC_PatternData, IMCMUSIC_PatternLookupTable, IMCMUSIC_EnvList, IMCMUSIC_EnvCounterList, IMCMUSIC_OscillatorList, IMCMUSIC_EffectList,
	IMCMUSIC_ChannelVol, IMCMUSIC_ChannelEnvCounter, IMCMUSIC_ChannelStopNote };
