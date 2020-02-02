// This file often does nothing. If it does anything it's incorporated into the end of main.h

#if DOING_VIDEOS

void wgo();

vector<eventHappened> video;
bool videoing = false;
string videoname;

void videoStart(string filename) {
	video.clear();
	video.reserve(80*60*1);
	videoname = filename;
	videoing = true;
}
void videoFrame() {
	if (videoing) {
		video.push_back(eventHappened());
		
		int size = video.size();
		int fps = FPS;
		if (0 == size % fps) {
			int s = size / fps;
			ERR("So far %d : %d\n", s / 60, s % 60);
		}
	}
}
void videoEvent(SDL_Event &e) {
	if (videoing)
		video.push_back(eventHappened(e));
}
void videoEnd() {
	videoing = false;
}

void videoSave() {
	slick.w = 1;
	slick.reset();
	
	char optfile[FILENAMESIZE] = "events.obj";
	ofstream f;
	unsigned int temp;
	f.open(optfile, ios_base::out | ios_base::binary | ios_base::trunc);
	
	if (f.fail()) {
		ERR("WAIT NO COULDN'T SAVE\n");
		return;
	}
	
	temp = videoname.size(); f.write((char *)&temp, sizeof(temp));
	f.write(videoname.c_str(), temp);

	temp = video.size(); f.write((char *)&temp, sizeof(temp));
	for(int c = 0; c < temp; c++) {
		f.write((char *)&(video[c]), sizeof(eventHappened));
	}
	
	slick.w = 4;
	slick.reset();
}

void videoLoad() {
	slick.w = 1;
	slick.reset();
	
	unsigned int temp;

	char optfile[FILENAMESIZE] = "events.obj";
	ifstream f;
	f.open(optfile, ios_base::in | ios_base::binary);
	if (f.fail()) {
		ERR("WAIT NO COULDN'T LOAD\n");
		return;
	}

	char filename[FILENAMESIZE+1];

	f.read((char *)&temp, sizeof(temp));
	f.read(filename, temp);
	filename[temp] = '\0';
	videoname = filename;

	f.read((char *)&temp, sizeof(temp));
	video.clear();
	for(int c = 0; c < temp; c++) {
		eventHappened e;
		f.read((char *)&e, sizeof(eventHappened));
		video.push_back(e);
	}
	
	slick.w = 4;
	slick.reset();
}

std::vector<unsigned char> frameimage;

void writeOneFrame(ofstream &f) {
	screenshotimage.resize(surfacew * surfaceh * 4);
	frameimage.resize(surfacew*surfaceh*4);
	glReadPixels(0, 0, surfacew, surfaceh, GL_RGBA, GL_UNSIGNED_BYTE, &screenshotimage[0]);
	
	uint32_t *data = (uint32_t *)&screenshotimage[0];
	unsigned char *outdata = (unsigned char *)&frameimage[0];
	
	for(unsigned y = 0; y < surfaceh; y++)
	for(unsigned x = 0; x < surfacew; x++)
	{
		unsigned y2 = surfaceh-y-1;
		unsigned int temp;
		temp = data[y2 * surfacew + x];
		*outdata = temp >> 24;
		outdata++;
		*outdata = temp >> 16;
		outdata++;
		*outdata = temp >> 8;
		outdata++;
	}
	
	f.write((char *)&frameimage[0], surfacew * surfaceh * 3);
}

#define LC 2
void displayCaption(ofstream &f) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	goOrtho();
	
	char *lines[LC] = {"Download at", "http://runhello.com/"};

	glColor3f(1.0, 1.0, 1.0);
	
	initFont(100);
	
	glEnable( GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ftex);
	for(int c = 0; c < LC; c++) {
		float leftMargin = -centerOff(lines[c]);

		glPushMatrix();
		glTranslatef(leftMargin, floatHeight*1.5 - floatHeight*3*c, 0);
		floating->Render(lines[c]);
		glPopMatrix();
	}
	glDisable( GL_TEXTURE_2D); 

	SDL_GL_SwapBuffers();		

	initFont(18);

	for(int c = 0; c < 80 * 4; c++) {
		writeOneFrame(f);
	}
}

// Play back with mplayer -demuxer rawvideo -rawvideo fps=80:w=640:h=480:format=rgb24 video.raw
// reencode with ./mencoder -demuxer rawvideo -rawvideo fps=80:w=640:h=480:format=rgb24 video.raw -ovc lavc -lavcopts vcodec=ffvhuff -o outfile.avi
void videoPlayback(bool tofile) {
	char optfile[FILENAMESIZE] = "/Users/mcc/video.raw";
	ofstream f;
	f.open(optfile, ios_base::out | ios_base::binary | ios_base::trunc);
	
	if (tofile)
		displayCaption(f);
	
	wantClearUi = true;
	serviceInterface();
	clearEverything();
	loadGame(videoname.c_str());
	wgo();
	for(int c = 0; c < video.size(); c++) {
		if (video[c].isEvent) {
			SDL_Event &event = video[c].event;
			
			if (event.type == SDL_KEYDOWN && KeyboardControl::focus != NULL) {
				KeyboardControl::focus->key(event.key.keysym.unicode, event.key.keysym.sym);
			} else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
	//			ERR("KEY EVENT TYPE %d KEY %d\n", (int)event.type, (int)event.key.keysym.sym); break;

				int control = -1;
				for(int c = 0; c < NUMCONTROLS; c++) {
					if (controlType[c] == ct_key && controlKeys[c] == event.key.keysym.sym)
						control = c;
				}

				if (control >= 0) {
					if ( event.type == SDL_KEYDOWN ) {
						moonBuggy_keydown(control);
					}
					if ( event.type == SDL_KEYUP ) {
						moonBuggy_keyup(control);
					}
				} else if (/*edit_mode && edit_mode != EPlayground &&*/ event.type == SDL_KEYDOWN) {
					switch ( event.key.keysym.sym ) {
						case SDLK_F5:
							if (edit_mode == EWalls) {
								drawHelp = !drawHelp;
								slick.w = drawHelp ? 1 : 4;
								slick.reset();
							}
							break;
						case SDLK_F6:
							if (edit_mode == EWalls) {
								drawPalette = !drawPalette;
								slick.w = drawPalette ? 1 : 4;
								slick.reset();
							}
							break;
						case SDLK_F7:
							if (edit_mode == EWalls) {
								drawGrid = !drawGrid;
								slick.w = drawGrid ? 1 : 4;
								slick.reset();
							}
							break;
						case SDLK_F8:
							pause(!paused);
							slick.w = !paused ? 1 : 4;
							slick.reset();
							if (edit_mode == EWalls) {
								justLoadedCleanup();
								reentryEdit();
							}
							break;
						case SDLK_F3: {
							drawMph = !drawMph;
							break;
						}
					}
				}
			}

		} else {
			display();
				
			if (tofile)
				writeOneFrame(f);
		}
	}
	
	if (tofile)
		displayCaption(f);
}
#endif
