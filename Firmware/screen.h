#ifndef __SCREEN_H
#define __SCREEN_H

#include "structs.h"

#define PLUS_MINUS   1
#define BRIGHTNESS_1 2
#define BRIGHTNESS_2 3
#define BRIGHTNESS_3 4
#define BRIGHTNESS_4 5
#define BRIGHTNESS_5 6
#define BRIGHTNESS_6 7
#define BRIGHTNESS_DISPLAY_LOCATION 16
#define MAX_BRIGHTNESS BRIGHTNESS_6
#define MIN_BRIGHTNESS BRIGHTNESS_1
#define POS_BRIGHTNESS 15
#define BRIGHTNESS_STEPS 6

#define SCREEN_ITEM_SIZE sizeof(ScreenUpdate)
#define SCREEN_QUEUE_SIZE 20

void ScreenClear(ScreenUpdate);
void ScreenIncreaseBrightness(ScreenUpdate);
void ScreenDecreaseBrightness(ScreenUpdate);
void ScreenTaskInit();
void GetScreen(char*);
void ScreenTop(char*, char*);
void ScreenBot(char*, char*);
int GetBrightness();
void ScreenOff();
void ScreenRestore();

#endif
