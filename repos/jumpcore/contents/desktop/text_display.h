/*
 *  text_display.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 6/21/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#ifndef _TEXT_DISPLAY_H
#define _TEXT_DISPLAY_H

void text_init();

float textWidth(string str);
float textHeight(string str = string());
float descenderHeight(string str = string());
float centerOff(string str);
void drawText(string text, double x, double y, double rot = 0, bool xcenter = true, bool ycenter = true);

#endif