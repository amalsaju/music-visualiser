#include <iostream>
#include <string>
#include <math.h>
#include <algorithm>
#include <complex>
#include <vector>
#include <cmath>
#include <filesystem>

#include "raylib.h"
// #define RAYGUI_IMPLEMENTATION
// #include "raygui.h"
#include "resource_dir.h" // utility header for SearchAndSetResourceDir
#include "reasings.h"	  // raylib easing library

#include "tinyfiledialogs.h"

namespace fs = std::filesystem;

// Colours
#define GUNMETAL_DARK (Color){0, 0, 0, 255}		// Gun Metal color dark
#define GUNMETAL_LIGHT (Color){40, 56, 69, 255} // Gun Metal color light

#define GUNMETAL_LIGHT (Color){40, 56, 69, 255} // Gun Metal color light

#define DEBUG 1

using Complex = std::complex<double>;
using CArray = std::vector<Complex>;

enum PLAY_BUTTONS
{
	FREWIND,
	FR_FILL,
	PLAY,
	PLAY_FILL,
	FFORWARD,
	FF_FILL,
	PAUSE,
	PAUSE_FILL
};

const double pi = acos(-1);
CArray data(256);

// FFT code taken from
// https://www.w3computing.com/articles/how-to-implement-a-fast-fourier-transform-fft-in-cpp/
void fft()
{
	const size_t N = data.size();
	if (N <= 1)
		return;

	// Bit-reversed addressing permutation
	size_t k = 0;
	for (size_t i = 1; i < N; ++i)
	{
		size_t bit = N >> 1;
		while (k & bit)
		{
			k ^= bit;
			bit >>= 1;
		}
		k ^= bit;

		if (i < k)
		{
			swap(data[i], data[k]);
		}
	}

	// Iterative FFT
	for (size_t len = 2; len <= N; len <<= 1)
	{
		double angle = -2 * pi / len;
		Complex wlen(cos(angle), sin(angle));
		for (size_t i = 0; i < N; i += len)
		{
			Complex w(1);
			for (size_t j = 0; j < len / 2; ++j)
			{
				Complex u = data[i + j];
				Complex v = data[i + j + len / 2] * w;
				data[i + j] = u + v;
				data[i + j + len / 2] = u - v;
				w *= wlen;
			}
		}
	}
}

Music music;

std::vector<std::string> musicFileNames{};
int musicIndex = 0;

void OpenFile()
{

	char const *lFilterPatterns[2] = {"*.mp3", "*.MP3"};

	char *fileNames;

	// Seperator for multiple files is |
	fileNames = tinyfd_openFileDialog(
		"Open Music File",
		"",
		2,
		lFilterPatterns,
		"text files",
		1);

	if (!fileNames)
	{
		std::cout << "File not found!" << std::endl;

		tinyfd_messageBox(
			"Error",
			"Open file name is NULL",
			"ok",
			"error",
			0);
	}
	else
	{
		std::string data = fileNames;

		std::stringstream ss(data);

		char delimiter = '|';

		std::string t;

		musicFileNames.clear();
		musicIndex = 0;

		while (getline(ss, t, delimiter))
		{
			// std::cout << "File Name: " << t << " : " << fs::is_directory(t) << std::endl;
			if (!fs::is_directory(t))
			{
				musicFileNames.push_back(t); // the file order is the order in which they appear in the folder ( I guess alphabetical ?)
				std::cout << "File selected is: " << t << std::endl;
			}
		}
	}
}

void ProcessAudio(void *buffer, unsigned int frames)
{
	float *samplesBuffer = (float *)buffer;

	for (int i = 0; i < 256; ++i)
	{
		data[i] = samplesBuffer[i];
	}
}

void HandleKeyboardPress(int key)
{
	switch (key)
	{
	case KEY_SPACE:
		if (IsMusicStreamPlaying(music))
		{
			PauseMusicStream(music);
		}
		else
		{
			ResumeMusicStream(music);
		}
		break;
	case KEY_RIGHT:
		SeekMusicStream(music, GetMusicTimePlayed(music) + 1.0f);
		break;
	case KEY_LEFT:
		SeekMusicStream(music, GetMusicTimePlayed(music) - 1.0f);
		break;
	case KEY_O:
		OpenFile();
		if (musicFileNames[0] != "NULL" || !musicFileNames.empty())
		{
			if (IsMusicStreamPlaying(music))
			{
				PauseMusicStream(music);
				StopMusicStream(music);
				music = LoadMusicStream(musicFileNames[musicIndex].c_str());
				AttachAudioStreamProcessor(music.stream, ProcessAudio);
				PlayMusicStream(music);
			}
		}
		break;
	default:
		break;
	}
}

bool IsHovering(Rectangle collisionArea)
{
	Vector2 mousePoint = GetMousePosition();

	if (CheckCollisionPointRec(mousePoint, {collisionArea.x + 10, collisionArea.y + 10, collisionArea.width - 10, collisionArea.height - 10}))
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string GetMusicName(std::string path)
{
	fs::path pathObj(path);
	std::string filename = pathObj.filename().string();
	return filename.substr(0, filename.size() - 4);
}

int main()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	SetTargetFPS(60);

	const int SCREEN_WIDTH = 1280;
	const int SCREEN_HEIGHT = 800;

	// Create the window and OpenGL context
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Music Visualiser");

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	Font robotoFont = LoadFontEx("fonts/Roboto.ttf", 32, 0, 250);
	if (IsFontValid(robotoFont))
	{
		printf("Font is valid!");
	}
	else
	{
		printf("Font is not valid!");
	}
	// Font robotoFont= LoadFontEx("resources/fonts/Roboto.ttf", 32, 0, 250);

	bool showMessageBox = false;

	float timePlayed = 0.0f;
	float interpolationSpeed = 0.01f;

	int playlistMarginLeft = 40;

	int timeLinePosX = 100;
	int timeLinePosY = 670;
	int timeLineLength = SCREEN_WIDTH - 280 - playlistMarginLeft - timeLinePosX;
	int timeLineHeight = 5;

	Texture2D buttonSpriteSheet = LoadTexture("spritesheet.png");
	int spriteCount = 8;
	int spriteWidth = buttonSpriteSheet.width / spriteCount;
	int spriteHeight = buttonSpriteSheet.height;

	Rectangle spriteSourceRet = {192, 0, (float)spriteWidth, (float)spriteHeight};
	Rectangle playBtnDestRec = {timeLineLength / 2.0f + timeLinePosX, timeLinePosY, spriteWidth, spriteHeight};
	Rectangle frBtnDestRec = {playBtnDestRec.x - 96, playBtnDestRec.y, spriteWidth, spriteHeight};
	Rectangle ffBtnDestRec = {playBtnDestRec.x + 96, playBtnDestRec.y, spriteWidth, spriteHeight};
	Vector2 origin = {0, 0};

	struct WhiteSpeck
	{
		Rectangle rec;
		Color color;
		Vector2 moveDirection;
		float moveSpeed;
	};

	WhiteSpeck whiteSpecks[100];

	for (int i = 0; i < 100; i++)
	{
		whiteSpecks[i].rec.height = GetRandomValue(1, 4);
		whiteSpecks[i].rec.width = whiteSpecks[i].rec.height;
		whiteSpecks[i].rec.x = GetRandomValue(1, SCREEN_WIDTH);
		whiteSpecks[i].rec.y = GetRandomValue(1, SCREEN_HEIGHT);
		whiteSpecks[i].moveDirection = Vector2{(float)GetRandomValue(1, 9) / 10, (float)GetRandomValue(1, 9) / 10};
		whiteSpecks[i].color = (Color){GetRandomValue(200, 255), GetRandomValue(200, 255), GetRandomValue(200, 255), 255};
		whiteSpecks[i].moveSpeed = GetRandomValue(10, 50);
	}

	Vector2 mousePoint;

	bool isBarGraph = true;

	InitAudioDevice();

	if (!DEBUG)
	{
		OpenFile();
		if (musicFileNames[musicIndex] != "NULL")
		{
			music = LoadMusicStream(musicFileNames[musicIndex].c_str());
			AttachAudioStreamProcessor(music.stream, ProcessAudio);
			PlayMusicStream(music);
		}
		else
		{
			tinyfd_messageBox(
				"Error",
				"The file could not be opened, try another one",
				"ok",
				"error",
				0);
		}
	}
	else
	{
		musicFileNames.clear();
		musicIndex = 0;
		musicFileNames.push_back("visualisation_music.mp3");
		music = LoadMusicStream(musicFileNames[musicIndex].c_str());
		AttachAudioStreamProcessor(music.stream, ProcessAudio);
		PlayMusicStream(music);
	}

	int frameCounter = 0;
	int noOfRectangles = 250;

	// Rectangle for drawing
	std::vector<Rectangle> rec(noOfRectangles);

	for (int i = 0; i < noOfRectangles; ++i)
	{
		rec[i] = {0, 0, (float)timeLineLength / (float)noOfRectangles, 0};
	}

	fft();
	std::vector<float> heightValues(256);
	for (int i = 0; i < 256; ++i)
	{
		heightValues[i] = std::abs(data[i]);
	}

	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// change song after current one is finished
		if ((int)GetMusicTimePlayed(music) == (int)GetMusicTimeLength(music))
		{
			musicIndex++;

			if (musicIndex >= musicFileNames.size())
			{
				musicIndex = 0;
			}

			music = LoadMusicStream(musicFileNames[musicIndex].c_str());
			AttachAudioStreamProcessor(music.stream, ProcessAudio);
			PlayMusicStream(music);
		}

		HandleKeyboardPress(GetKeyPressed());

		UpdateMusicStream(music);
		// Reducing the number of times we update the audio frequency to show a better visualiser

		mousePoint = GetMousePosition();
		// std::cout << "Mouse Point : " << mousePoint.x << " ," << mousePoint.y << std::endl;

		Rectangle playCollisionArea = {610, 700, 30, 40};
		// check for playbutton
		if (CheckCollisionPointRec(mousePoint, playCollisionArea))
		{
			// std::cout << "Rectangle points: " << playCollisionArea.x << " ," << playCollisionArea.y << " ," << playCollisionArea.width << " ," << playCollisionArea.height << std::endl;

			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				HandleKeyboardPress(KEY_SPACE);
			}
		}

		if (!IsMusicStreamPlaying(music))
		{
			// Todo: give a small padding around the buttons
			Rectangle pauseCollisionArea = {600, 690, 50, 60};
		}

		Rectangle fastRewindCollisionArea = {500, 700, 60, 40};
		Rectangle fastForwardCollisionArea = {690, 700, 50, 40};

		if (CheckCollisionPointRec(mousePoint, fastRewindCollisionArea))
		{

			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				HandleKeyboardPress(KEY_LEFT);
			}
		}

		if (CheckCollisionPointRec(mousePoint, fastForwardCollisionArea))
		{

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				HandleKeyboardPress(KEY_RIGHT);
			}
		}

		// drawing
		BeginDrawing();

		// if (GuiButton((Rectangle){24, 24, 120, 30}, "#191#Show Message"))
		// 	showMessageBox = true;

		// if (showMessageBox)
		// {
		// 	int result = GuiMessageBox((Rectangle){85, 70, 250, 100},
		// 							   "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

		// 	if (result >= 0)
		// 		showMessageBox = false;
		// }

		if (IsMusicStreamPlaying(music))
		{

			if (frameCounter > 9)
			{
				fft();
				// std::cout << " Entered this function" << std::endl;
				for (int i = 0; i < 256; ++i)
				{
					heightValues[i] = data[i].real();
				}
				frameCounter = 0;
			}
			for (int i = 0; i < 100; i++)
			{
				whiteSpecks[i].rec.x += whiteSpecks[i].moveDirection.x * whiteSpecks[i].moveSpeed * GetFrameTime();
				if (whiteSpecks[i].rec.x > 1000)
				{
					whiteSpecks[i].rec.x = 0;
				}
				//  DrawCircle(whiteSpecks[i].rec.x, whiteSpecks[i].rec.y, whiteSpecks[i].rec.width, whiteSpecks[i].color);
				DrawRectangle(whiteSpecks[i].rec.x, whiteSpecks[i].rec.y, whiteSpecks[i].rec.width, whiteSpecks[i].rec.height, whiteSpecks[i].color);
			}
			frameCounter++;

			for (int i = 0; i < noOfRectangles; ++i)
			{
				float value = 0;

				value = std::min((int)(heightValues[i] * 50), 200);
				value = value > 0 && value < 10 ? value * 2.0f : value;
				value = std::max((int)value, 5);

				//(current time/frame, initial value, variation of value, time duration)
				rec[i].height = EaseQuadOut((float)frameCounter, 0, value * 1.5, 6);
				rec[i].height = std::clamp((int)rec[i].height, 0, 300);
				rec[i].x = 100 + i * rec[i].width;
				if (isBarGraph)
				{

					rec[i].y = 550 - rec[i].height;
				}
				else
				{

					rec[i].y = SCREEN_HEIGHT / 2 - rec[i].height / 2;
				}

				Vector2 barOrigin = {0.0f, 0.0f};
				DrawRectanglePro(rec[i], barOrigin, 0.0f, (Color){255, 71, i * (250 / noOfRectangles), 255});
			}

			// Draw the remaining music file names
			DrawRectangle(SCREEN_WIDTH - 250, 0, 250, SCREEN_HEIGHT, GUNMETAL_LIGHT);
			DrawTextEx(robotoFont, "Playlist", {SCREEN_WIDTH - 220, 20}, robotoFont.baseSize, 2, YELLOW);

			for (int i = 0; i < musicFileNames.size(); ++i)
			{
				// Draw everything in the list, highlight the current one
				if (IsHovering({SCREEN_WIDTH - 230, (float)100 + (100 * i) - 10, 200, 40}))
				{
					DrawRectangle(SCREEN_WIDTH - 230, (float)100 + (100 * i) - 10, 200, 40, WHITE);
					if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
					{
						if (IsMusicStreamPlaying(music))
						{
							PauseMusicStream(music);
							StopMusicStream(music);
							music = LoadMusicStream(musicFileNames[i].c_str());
							musicIndex = i;
							AttachAudioStreamProcessor(music.stream, ProcessAudio);
							PlayMusicStream(music);
						}
					}
				}
				// DrawPlaylistItem(robotoFont, GetMusicName(musicFileNames[i]), {SCREEN_WIDTH - 220, (float)100 + (100 * i)});
				// DrawTextEx(robotoFont, GetMusicName((musicFileNames[i])).c_str(), {SCREEN_WIDTH - 220, (float)100 + (100 * i)}, robotoFont.baseSize / 1.5, 1, musicIndex == i ? YELLOW : ORANGE);
			}

			if (IsHovering({55, 50, 150, 40}))
			{
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					isBarGraph = !isBarGraph;
				}
				// cyan background
				DrawRectangle(55, 50, 170, 40, (Color){67, 255, 175, 255});

				std::string text = "";
				if (isBarGraph)
				{
					text = "Box visuals";
				}
				else
				{
					text = "Graph visuals";
				}
				// Dark Blue text
				DrawText(text.c_str(), 65, 60, 20, (Color){38, 42, 51, 255});
			}
			else
			{
				// Dark blue background
				DrawRectangle(55, 50, 170, 40, (Color){38, 42, 51, 255});
				std::string text = "";
				if (isBarGraph)
				{
					text = "Box visuals";
				}
				else
				{
					text = "Graph visuals";
				}
				DrawText(text.c_str(), 65, 60, 20, (Color){229, 247, 239, 255});
			}

			// Draw music details and timeline
			DrawText(GetMusicName(musicFileNames[musicIndex]).c_str(), 70, 640, 20, ORANGE);

			timePlayed = GetMusicTimePlayed(music) / GetMusicTimeLength(music);

			if (timePlayed > 1.0f)
				timePlayed = 1.0f;

			DrawRectangle(timeLinePosX, timeLinePosY, timeLineLength, timeLineHeight, GUNMETAL_LIGHT);
			DrawRectangle(timeLinePosX, timeLinePosY, (int)(timePlayed * timeLineLength), timeLineHeight, (Color){255, 71, (timePlayed * 28 * 10) / 2, 255});
			DrawRectangleLines(timeLinePosX, timeLinePosY, timeLineLength, timeLineHeight, GUNMETAL_LIGHT);

			// Draw media control buttons
			if (IsMusicStreamPlaying(music))
			{
				DrawTexturePro(buttonSpriteSheet, {(float)(IsHovering(playBtnDestRec) ? PLAY_FILL : PLAY) * 96, 0, spriteSourceRet.width, spriteSourceRet.height}, playBtnDestRec, origin, 0, WHITE);
			}
			else
			{
				DrawTexturePro(buttonSpriteSheet, {(float)(IsHovering(playBtnDestRec) ? PAUSE_FILL : PAUSE) * 96, 0, spriteSourceRet.width, spriteSourceRet.height}, playBtnDestRec, origin, 0, WHITE);
			}

			DrawTexturePro(buttonSpriteSheet, {(float)(IsHovering(frBtnDestRec) ? FR_FILL : FREWIND) * 96, 0, spriteSourceRet.width, spriteSourceRet.height}, frBtnDestRec, origin, 0, WHITE);
			DrawTexturePro(buttonSpriteSheet, {(float)(IsHovering(ffBtnDestRec) ? FF_FILL : FFORWARD) * 96, 0, spriteSourceRet.width, spriteSourceRet.height}, ffBtnDestRec, origin, 0, WHITE);

			DrawFPS(1200, 10);
			// Setup the back buffer for drawing (clear color and depth buffers)
			ClearBackground(GUNMETAL_DARK);
		}
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	UnloadFont(robotoFont);
	UnloadMusicStream(music);

	UnloadTexture(buttonSpriteSheet);

	CloseAudioDevice();

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
