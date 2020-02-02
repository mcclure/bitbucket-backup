/*
 *  interface.cpp
 *  Jumpman
 *
 *  Created by Andi McClure on 4/27/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
	
#include "chipmunk.h"
#include "lodepng.h"
#include "sound.h"
#include "internalfile.h"
#include "tinyxml.h"
#include "color.h"
#include "jumpman.h"
#include "kludge.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>

#ifdef WINDOWS

// Why on earth is this necessary?
#define htonl not_htonl

inline long not_htonl (long i) { // Mingw has failed me but windows only ever has one endianness anyway.
	long r;
    char *p = (char *)&r;
	char *c = (char *)&i;
	    p[0] = c[3];
        p[1] = c[2];
        p[2] = c[1];
        p[3] = c[0];
	return *((long *)p);
}
#endif

void recreate_surface(bool in_a_window);

// I do this with a -D flag in the project settings now
// #define SELF_EDIT 0

/* --- SOME GUI STUFF. --- */

ContainerBase *container[5] = {0,0,0,0,0};
bool wantClearUi = false;
WantNewScreen want = WNothing, onEsc = WNothing, onWin = WMainMenu, onSave = WNothing;
TextControl *scratchTextControl = NULL;
bool paused = false, completelyHalted = false;
bool editorPanning = false;

ScrollContainer *currentlyScrolling = NULL;
int lastlevelscroll = 0;
int currentleveloffset = 0;
bool beenUsingFullPalette = false;

CheckGroup paletteGroup;

KeyboardControl *setName, *setMessage;
CheckControl *setCamera, *setRepeat, *setFlag;
WheelControl *setAfter, *setZoom, *setLayers;
ControlBase *paletteDefault = NULL;

CheckControl *setOptions[5];

bool haveWonGame = false;

string editDisplayName;

string lastLoadedFilename;
void wload(string filename, bool suppressMenu = false);
void depopulateControlsScreen();
#define FULL_REPROT 0
#if FULL_REPROT
WheelControl *setReprot[4];
CheckControl *setRepref;
#else
WheelControl *setReprot;
#define NUMLOOPS 20
const int loops[NUMLOOPS] = {0, 1313, 2222, 3131, 4444, 5577, 6666, 7755, 202, 404, 606, 2020, 2424, 2626, 4040, 4242, 4646, 6060, 6262, 6464};
#endif

// For no particular reason, all state related to the "color" pane lives inside of the RGB/HSV convert button.
class IndividualColor : public WheelControl {public:
	IndividualColor(double _text = 0, double _lo = 0, double _hi = 0, double _by = 1)
		: WheelControl(_text,_lo,_hi,_by) {}
	void feedback();
	virtual void wheel(int dir) { WheelControl::wheel(dir); feedback(); }
	virtual void loseFocus() {
		WheelControl::loseFocus();
		feedback();
	}
};
class ColorCenter : public ControlBase {
public:
	bool hsv;
	ControlBase *labels[3];
	WheelControl *colors[3];
	void setHsv(bool _hsv) {
		hsv = _hsv;
		if (hsv) {
			labels[0]->text = "Hue:";
			labels[1]->text = "Saturation:";
			labels[2]->text = "Value:";
		} else {
			labels[0]->text = "Red:";
			labels[1]->text = "Green:";
			labels[2]->text = "Blue:";
		}
		if (hsv) {
			RGBtoHSV(colors[0]->is, colors[1]->is, colors[2]->is, &colors[0]->is, &colors[1]->is, &colors[2]->is);
			if (!isnormal(colors[0]->is)) colors[0]->is = 0; // Sometimes hue comes out meaningless
			colors[0]->hi = 360; colors[0]->by = 10;
		} else {
			HSVtoRGB(&colors[0]->is, &colors[1]->is, &colors[2]->is, colors[0]->is, colors[1]->is, colors[2]->is);
			colors[0]->hi = 1.0; colors[0]->by = 0.05;
		}
		for(int c = 0; c < 3; c++)
			colors[c]->changedIs();
	}
	ColorCenter(string _text) : ControlBase(_text, true) {
		for(int c = 0; c < 3; c++) {
			labels[c] = new ControlBase("");
			colors[c] = new IndividualColor(0, 0, 1, 0.05);
		}
		setHsv(false);
	}
	void feedback() {
		if (hsv) {
			HSVtoRGB(&level[0].r[editLayer], &level[0].g[editLayer], &level[0].b[editLayer], colors[0]->is, colors[1]->is, colors[2]->is);
		} else {
			level[0].r[editLayer] = colors[0]->is; level[0].g[editLayer] = colors[1]->is; level[0].b[editLayer] = colors[2]->is;
		}
//		ERR("Okay uh %s: %lf %lf %lf -> %lf %lf %lf\n", hsv?"HSV":"RGB", colors[0]->is, colors[1]->is, colors[2]->is, level[0].r, level[0].g, level[0].b);
	}
	virtual void click() {
		sjump.reset();
		setHsv(!hsv);
		feedback(); // This should do nothing, unless something's broken
	}
};
ColorCenter *colorCenter = NULL;
void IndividualColor::feedback() {
	if (colorCenter)
		colorCenter->feedback();
}

void LoadFromColorCenter() {
	colorCenter->colors[0]->is = level[0].r[editLayer]; colorCenter->colors[0]->changedIs();
	colorCenter->colors[1]->is = level[0].g[editLayer]; colorCenter->colors[1]->changedIs();
	colorCenter->colors[2]->is = level[0].b[editLayer]; colorCenter->colors[2]->changedIs();
}
void SaveToColorCenter() {
	TiXmlNode *i = NULL;

	while(i = editLevel->IterateChildren("Color", i)) {
		if (i->Type() != TiXmlNode::ELEMENT)
			continue;

		TiXmlElement *colorXml = (TiXmlElement *)i;
		int temp;
		if (TIXML_SUCCESS == colorXml->QueryIntAttribute("layer", &temp) ?
			(temp != editLayer) : (0 != editLayer)) // If there's a layer, but it's not the one we're editing
			continue;

		if (KeyboardControl::focus) KeyboardControl::focus->loseFocus();

		if (colorCenter->hsv) {
			colorXml->SetDoubleAttribute("h", colorCenter->colors[0]->is);
			colorXml->SetDoubleAttribute("s", colorCenter->colors[1]->is);
			colorXml->SetDoubleAttribute("v", colorCenter->colors[2]->is);
			colorXml->RemoveAttribute("r");
			colorXml->RemoveAttribute("g");
			colorXml->RemoveAttribute("b");
		} else {
			colorXml->RemoveAttribute("h");
			colorXml->RemoveAttribute("s");
			colorXml->RemoveAttribute("v");
			colorXml->SetDoubleAttribute("r", colorCenter->colors[0]->is);
			colorXml->SetDoubleAttribute("g", colorCenter->colors[1]->is);
			colorXml->SetDoubleAttribute("b", colorCenter->colors[2]->is);
		}
		editing->SaveFile();
	}
}

class LayerSwitchWheelControl : public WheelControl {public:
	LayerSwitchWheelControl(double _text = 0, double _lo = 0, double _hi = 0, double _by = 1)
	: WheelControl(_text,_lo,_hi,_by) {}
	virtual void wheel(int dir) {
		SaveToColorCenter();
		colorCenter->setHsv(false);
		WheelControl::wheel(dir); 
		editLayer = this->is - 0.5; // Round to nearest minus one
		LoadFromColorCenter();
	}
};

bool dontLiveUpdate = false;
void UpdateSetupFields();
void LiveUpdateSetupFields();

class LiveWheelControl : public WheelControl {public:
	LiveWheelControl(double _text = 0, double _lo = 0, double _hi = 0, double _by = 1)
	: WheelControl(_text,_lo,_hi,_by) {}
	virtual void wheel(int dir) { WheelControl::wheel(dir); LiveUpdateSetupFields(); }
};
class LiveCheckControl : public CheckControl {public:
	LiveCheckControl(string _text = string(), bool _byImg = false, CheckGroup *_group = NULL)
	: CheckControl(_text, _byImg, _group) {}
	virtual void click() { CheckControl::click(); LiveUpdateSetupFields(); }
};
class LiveSelectControl : public SelectControl {public:
	LiveSelectControl(vector<string> &_options, int initial = 0)
	: SelectControl(_options, initial) {}
	virtual void wheel(int dir) { SelectControl::wheel(dir); LiveUpdateSetupFields(); }
};
	
void SaveSetupPane() {
	TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
	// TODO: What if no such thing?
	((TiXmlElement*)editLevel)->SetAttribute("name", setName->text);
	
	if (setFlag->checked)
		((TiXmlElement*)editLevel)->SetAttribute("flag", 1);	
	else
		((TiXmlElement*)editLevel)->RemoveAttribute("flag");

	UpdateSetupFields();
	typeXml->SetAttribute("after", (int)(setAfter->is+0.5));
	if (!setMessage->text.empty()) {
		TiXmlElement *messageXml = (TiXmlElement *)editLevel->IterateChildren("Message", NULL);
		if (!messageXml) {
			messageXml = new TiXmlElement("Message");
			editLevel->LinkEndChild(messageXml);
		}
		messageXml->SetAttribute("text", setMessage->text);
	}
		
	editing->SaveFile();
}

void rm_from_editpath(string target) {
	char filename2[FILENAMESIZE];
	snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), target.c_str());
	
	unlink(filename2); // DELETE
}

void deepLoad (TiXmlNode *target) {
	TiXmlElement *element = (TiXmlElement *)target;
	TiXmlNode *i = NULL;
	
	editSlice.clear();
	editSlicePath.clear();
	editLayer = 0;

	if (currentlyScrolling)
		lastlevelscroll = currentlyScrolling->scroll;
	currentlyScrolling = NULL;
//ERR("WTF %x %d", currentlyScrolling,  lastlevelscroll);
			
	while(i = element->IterateChildren("File", i)) {
		if (i->Type() != TiXmlNode::ELEMENT)
			continue;
		char filename2[FILENAMESIZE];
		snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), ((TiXmlElement *)i)->Attribute("src"));
		ERR("OOOO %d %s\n", editSlicePath.size(), filename2);
		editSlicePath.push_back( filename2 );
		slice *s = new slice();
		s->construct(filename2, false);
		editSlice.push_back(s);
	}
}

class PaletteControl : public CheckControl {
public:
	int tool;
	list<string> help;
	PaletteControl *egg;
	
	PaletteControl(string _text = string(), CheckGroup *_group = NULL, int _tool = 0, string _help = string()) : CheckControl(_text, false, _group), tool(_tool), egg(NULL)
		{ 
			if (!_help.empty()) {
				int	upto = 0;
				int from = 0;
				do {
					upto = _help.find('\n', upto+1);
					string sub = _help.substr(from, string::npos == upto ? _help.size() : upto-from);
					
					if (0 == from) sub = "Selected: " + sub;
					help.push_back(sub);
					
					from = upto + 1;
				} while (upto != string::npos);
			}
		}
	virtual ~PaletteControl() { if (egg) delete egg; }
	virtual void click() { 
		if (egg) {
			SDLMod mods = SDL_GetModState();
			if (mods & KMOD_LCTRL || mods & KMOD_RCTRL) {
				egg->click();
				return;
			}
		}
		if (!checked) { 
			if (!group || group->checked) 
				sjump.reset();
			CheckControl::click(); chosenTool = tool; 
			toolHelp = help;
		}
		haveLineStart = false;
		dragPoison = true;
	}
};

enum EditorMenuButton { EMLevel, EMIgnore };
class EditorMenuControl : public ControlBase
{public:
	EditorMenuButton me;
	TiXmlNode *target;
	int menuOffset;
	EditorMenuControl(string _text = string(), EditorMenuButton _me = EMLevel, TiXmlNode *_target = NULL, int offset = 0) : ControlBase(_text, true), me(_me), target(_target), menuOffset(offset) {}
	virtual void click() {
		switch (me) {
			case EMLevel: {
				sjump.reset();
				deepLoad(target);
				wantClearUi = true;
				want = WLevelMenu; // This will handle level redrawing
				editLevel = target;
				currentleveloffset = menuOffset;
			} break;
		}
	}
};

PaletteControl *happyCache, *angryCache;
enum MainMenuButton { MMPath1, MMEdit, MMDraw, MMVC, MMEC, MMOC, MMHalt, MMNewFile, MMEnteredNewFile, MMOSave, MMMBack, MMUnpause, MMDoubleBack, MMStopPlaying, MMStopCredits, MMResumeCredits,
					MMLTest, MMLWalls, MMLWallsDirty, MMLSetup, MMLColor, MMLAngle, MMLUpDown, MMLBack, MMEMore, MMENextLayer, MMEPrevLayer, MMLDupe, MMLDelete, MMLDupeConfirm, MMLDeleteConfirm, MMLLayers, MMLLayersConfirm,
					MMLSave, MMLDontSave, MMLDontSaveRevert, MMLSetupSave, MMLColorSave, MMLUp, MMLDown, MMEditPlay, MMNewLevel };
class MainMenuControl : public ControlBase
{public:
	MainMenuButton me;
	MainMenuControl(string _text = string(), MainMenuButton _me = MMPath1) : ControlBase(_text, true), me(_me) {}
	virtual void click() {
		sjump.reset();
		switch (me) {
			case MMPath1:
				want = WReset;
				wantClearUi = true;
				edit_mode = ENothing;
				break;
			case MMDraw:
				addFloater("Left and right click to draw, or spin the mousewheel.");
#if defined(__APPLE__)
				addFloater("(Or, hold down option or command when you click.)");
#endif
				addFloater("Press ESC to exit.");
				wantClearUi = true;
				paused = false;
				onEsc = WMainMenu;
				if (edit_mode != EPlayground) {
					edit_mode = EPlayground;
					clearEverything();
					dummyStage();	
					jumpman_reset();
				}
				break;
			case MMEdit:
				wantClearUi = true;
				want = WEditMenu;
				break;
			case MMNewFile:
				if (text == "New") {
					wantClearUi = true;
					want = WNewFileEntry;
				} else {
					wantClearUi = true;
					editDisplayName = text;
										
					loadEditorFile(dotJmp(text).c_str());
					want = WCurrentFileDirectory;
					lastlevelscroll = 0;
				}
				break;
			case MMEnteredNewFile: {
				wantClearUi = true;
				editDisplayName = scratchTextControl->text;
				string name = dotJmp(scratchTextControl->text);
				mkdir(name.c_str()
#ifndef WINDOWS
				, 0777
#endif
				); // CREATE DIR OF ENTERED NAME
				
				{
					char filename2[FILENAMESIZE];
					snprintf(filename2, FILENAMESIZE, "%s/index.xml", name.c_str());

					TiXmlDocument *newFile = new TiXmlDocument(filename2);
					TiXmlElement *root = new TiXmlElement("Jumpman");
					root->SetAttribute("Version", "1.0");
					newFile->LinkEndChild(root);
					newFile->SaveFile();
					delete newFile;
				}
				
				loadEditorFile(name.c_str());
				want = WCurrentFileDirectory;
				lastlevelscroll = 0;
				} break;
			case MMVC:
				controlsScreen(true);
				break;
			case MMEC:
				controlsScreen(false);
				break;
			case MMOC:
				onEsc = WIgnore;
				want = WDisplayOptions;
				wantClearUi = true;
				completelyHalted = true;
				break;
			case MMHalt:
				Quit();
				break;
				
			case MMEditPlay:
				want = WPlay;
				wantClearUi = true;
				edit_mode = ENothing;
				
				if (currentlyScrolling)
					lastlevelscroll = currentlyScrolling->scroll;
				currentlyScrolling = NULL;

				break;
			case MMNewLevel: { // DON'T DUPE; SAVE PNGS
				TiXmlElement *levelData = new TiXmlElement("Level");
				TiXmlElement *tempData;
				int levelNum = container[CMID]->controls.size()+1;
				
				char filename[FILENAMESIZE];
				snprintf(filename, FILENAMESIZE, "Level %d", levelNum);
				levelData->SetAttribute("name", filename);
				
				tempData = new TiXmlElement("Type");
				tempData->SetAttribute("camera", "0");
				tempData->SetAttribute("rots", "255");
				tempData->SetAttribute("dontrot", "0");
				tempData->SetAttribute("rspeed", "1");
				levelData->LinkEndChild(tempData);
				tempData = new TiXmlElement("Color");
				tempData->SetAttribute("r", "1.0");
				tempData->SetAttribute("g", "1.0");
				tempData->SetAttribute("b", "1.0");
				levelData->LinkEndChild(tempData);
				
				editSlice.clear();
				editSlicePath.clear();
				editLayer = 0;
				
				for(int c = 0; c < 2; c++) {
					char filename2[FILENAMESIZE];
					snprintf(filename, FILENAMESIZE, "%d.png", random());
					
					tempData = new TiXmlElement("File");
					tempData->SetAttribute("src", filename);
					tempData->SetAttribute("rot", c);
					levelData->LinkEndChild(tempData);
																				
					snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), filename);
					editSlicePath.push_back(filename2);
					
					{
						int width = 16, height = 16;
						std::vector<unsigned char> image;
						image.resize(width * height * 4);
						uint32_t *data = (uint32_t *)&image[0];

						for(unsigned x = 0; x < width; x++)
						for(unsigned y = 0; y < height; y++)
						{ 
							data[y * width + x] = htonl(
								(0 == c && (x == 0 || y == 0 || x == width-1 || y == height-1))
									? C_FLOOR : 0 );
						}
						
						std::vector<unsigned char> buffer;
						LodePNG::Encoder encoder;
						encoder.encode(buffer, image, width, height);

						LodePNG::saveFile(buffer, filename2);
					}
					
					slice *s = new slice();
					s->construct(filename2, false);
					editSlice.push_back(s);
				}
				
				if (currentlyScrolling)
					lastlevelscroll = currentlyScrolling->scroll;
				currentlyScrolling = NULL;
				currentleveloffset = lastlevelscroll;
				
				if (container[CMID]->controls.size() > 0) {
					TiXmlNode *ceiling = ((EditorMenuControl*)container[CMID]->controls[lastlevelscroll])->target;
					editLevel = ceiling->Parent()->InsertBeforeChild(ceiling, *levelData);
				} else {
					editLevel = editing->RootElement()->InsertEndChild(*levelData);
				}
				
				int temp; // Zoom out ending on anything with at least 10 levels
				if (levelNum >= 10 && TIXML_SUCCESS != editing->RootElement()->QueryIntAttribute("ending", &temp)) {
					editing->RootElement()->SetAttribute("ending", 1);
				}
				
				delete levelData;				
				editing->SaveFile();
								
				wantClearUi = true;
				want = WLevelMenu;
				break;
			} break;
				
			case MMLTest: {
				clearEverything();
				loadLevel(editLevel, editingPath.c_str());
				rePerspective = true;
				justLoadedCleanup();
				currentScoreKey = ""; // Make sure we don't accidentally smash some high scores
				
				jumpman_reset();
				wantClearUi = true;
				onEsc = WLevelMenu;
				onWin = WLevelMenu;
				paused = false;
			} break;
			
			case MMLWalls: case MMLWallsDirty: {
				edit_mode = EWalls;
				wantClearUi = true;
				want = WPalette;
				onEsc = (me == MMLWallsDirty ? WSave : WLevelMenu);
				onSave = WLevelMenu;
				level[jumpman_s].camera = cam_track;
			} break;
			
			case MMLSetup: {
				wantClearUi = true;
				want = WSetup;
				onEsc = WIgnore;
				break;
			}
			
			case MMLColor: {
				wantClearUi = true;
				want = WColor;
				onEsc = WIgnore;
				break;
			}
			
			case MMLAngle: {
				wantClearUi = true;
				want = WAngle;
				onEsc = WIgnore;
				break;
			}
			
			case MMLUpDown: {
				wantClearUi = true;
				want = WUpDown;
				onEsc = WLevelMenu;
				break;
			}
			
			case MMLUp: {
				wantClearUi = true; want = WUpDown; onEsc = WSaveThenLevelMenu;
				TiXmlNode *src = ((EditorMenuControl*)container[CMIDL]->controls[currentleveloffset])->target;
				currentleveloffset--;
				TiXmlNode *dst = ((EditorMenuControl*)container[CMIDL]->controls[currentleveloffset])->target;
				TiXmlNode *tmp = dst->Clone();
				editLevel = dst->Parent()->ReplaceChild(dst, *src);
				src->Parent()->ReplaceChild(src, *tmp);
				delete tmp;
				break;
			}
				
			case MMLDown: { // FIXME: Code duplication :( :( :(
				wantClearUi = true; want = WUpDown; onEsc = WSaveThenLevelMenu;
				TiXmlNode *src = ((EditorMenuControl*)container[CMIDL]->controls[currentleveloffset])->target;
				currentleveloffset++;
				TiXmlNode *dst = ((EditorMenuControl*)container[CMIDL]->controls[currentleveloffset])->target;
				TiXmlNode *tmp = dst->Clone();
				editLevel = dst->Parent()->ReplaceChild(dst, *src);
				src->Parent()->ReplaceChild(src, *tmp);
				delete tmp;
				break;
			}
			
			case MMLBack:
				wantClearUi = true;
				want = WCurrentFileDirectory;
				break;
				
			case MMOSave: {
				bool old_optWindow = optWindow;
				
				optColorblind = setOptions[0]->checked;
				optSlow = setOptions[1]->checked;
				optAngle = setOptions[2]->checked;
				optSplatter = setOptions[3]->checked;
				optWindow = setOptions[4]->checked;
				
				depopulateControlsScreen();
				
				completelyHalted = false;
				
				if (optWindow != old_optWindow)
					recreate_surface(optWindow);
				
				initFog(); // Let settings take effect
				
				if (optSplatter) {
					wantClearUi = true;
					want = WNothing;
					edit_mode = ENothing;
				
					char filename[FILENAMESIZE]; // Copied from WReset
					internalPath(filename, "Main.jmp");
					currentScoreKey = "/";
					wload(filename, true); // This is the difference
					
					break;
				}
			}
				
			case MMMBack:
				wantClearUi = true;
				want = WMainMenu;
				break;
				
			case MMStopPlaying: // Clumsily try to remove Jumpman from the world, then return to main menu.
				wantClearUi = true;
				want = WMainMenu;
				pantype = pan_dont;
				jumpmanstate = jumpman_normal;
				jumpman_unpinned = true;
				  externSpaceRemoveBody(level[jumpman_s], chassisShape);
				cpSpaceRemoveShape(level[jumpman_s].space, chassisShape);
				paused = false;
				break;
				
			case MMUnpause:
				wantClearUi = true;
				want = WUnpause;
				break;
			
			case MMStopCredits:
				wantClearUi = true;
				want = WMainMenu;
				endEnding();
				break;
			
			case MMResumeCredits:
				wantClearUi = true;
				want = WResumeCredits;
				break;
				
			case MMLDupe:
				wantClearUi = true;
				want = WLevelDupe;
				onEsc = WLevelMenu;
				break;
				
			case MMLDelete:
				wantClearUi = true;
				want = WLevelDelete;
				onEsc = WLevelMenu;
				break;
				
			case MMLDupeConfirm: { // They've requested we duplicate this level
				TiXmlNode *newLevel = editLevel->Clone(); // This is the duplicate we will add to the xml file
				TiXmlNode *i = NULL;
				
				while (i = newLevel->IterateChildren("File", i)) { // It will be the same as the old one, but all the files are duplicates too
					if (i->Type() != TiXmlNode::ELEMENT)
						continue;
					char filename[FILENAMESIZE];
					char filename2[FILENAMESIZE];
					char filename3[FILENAMESIZE];
					snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), ((TiXmlElement *)i)->Attribute("src"));
					snprintf(filename, FILENAMESIZE, "%d.png", random());
					snprintf(filename3, FILENAMESIZE, "%s/%s", editingPath.c_str(), filename);
					
#if WINDOWS
#define PLUSFLAGS "b"
#else
#define PLUSFLAGS
#endif

					FILE * file2 = fopen(filename2, "r" PLUSFLAGS); // 10-line cp implementation
					FILE * file3 = fopen(filename3, "w" PLUSFLAGS);
					if (!file2)
						FileBombBox(filename2);
					if (!file3)
						FileBombBox(filename3);
					while(1) {
						int c = fgetc(file2);
						if (EOF == c)
							break;
						fputc(c, file3);
					}
					fclose(file2);
					fclose(file3);
					
					((TiXmlElement *)i)->SetAttribute("src", filename);
				}
				
				editLevel->Parent()->InsertAfterChild(editLevel, *newLevel);
				wantClearUi = true;
				want = WSaveThenLevelMenu;
				sbell.reset();
				
				delete newLevel;
				break;
			}
				
			case MMLDeleteConfirm: { // They've asked to delete this level
				wantClearUi = true;
				want = WCurrentFileDirectory;
				TiXmlNode *i = NULL;
				
				while (i = editLevel->IterateChildren("File", i)) { // And all the files that go with it
					if (i->Type() != TiXmlNode::ELEMENT)
						continue;
					rm_from_editpath(((TiXmlElement *)i)->Attribute("src"));
				}				
				
				editLevel->Parent()->RemoveChild(editLevel);
				editing->SaveFile();
				
				splodey.reset();
				break;
			}
				
			case MMEMore:
				want = WMorePalette;
				break;
				
			case MMENextLayer: {
				SDLMod mods = SDL_GetModState();	// KLUDGE "TO THE MAX"
				if (mods & KMOD_LCTRL || mods & KMOD_RCTRL) {
					angryCache->click();
					break;
				}
				
				editLayer++;
				editLayer %= (editSlice.size()/2);				
				chassisShape->layers = (1 << editLayer);
				layerYell();
			} break;
				
			case MMEPrevLayer: {
				SDLMod mods = SDL_GetModState();
				if (mods & KMOD_LCTRL || mods & KMOD_RCTRL) {
					happyCache->click();
					break;
				}
				
				editLayer--;
				editLayer += (editSlice.size()/2);
				editLayer %= (editSlice.size()/2);
				chassisShape->layers = (1 << editLayer);
				layerYell();
			} break;
				
			case MMLSave: {
				for(int c = 0; c < editSlice.size(); c++) {
				  std::vector<unsigned char> image;
				  image.resize(editSlice[c]->width * editSlice[c]->height * 4);
				  uint32_t *data = (uint32_t *)&image[0];
				  for(unsigned x = 0; x < editSlice[c]->width; x++)
				  for(unsigned y = 0; y < editSlice[c]->height; y++)
				  {
					data[y * editSlice[c]->width + x] = htonl(editSlice[c]->pixel[x][y]);
				  }
				  
				  //encode and save
				  std::vector<unsigned char> buffer;
				  LodePNG::Encoder encoder;
				  encoder.encode(buffer, image, editSlice[c]->width, editSlice[c]->height);
				  
				  LodePNG::saveFile(buffer, editSlicePath[c].c_str());
				  }
			} // FALL THROUGH!:
			case MMLDontSaveRevert:
			case MMLDontSave:
				wantClearUi = true;
				want = onSave;
				
				if (me == MMLDontSaveRevert)
					deepLoad(editLevel); // Revert
				
				break;
				
			case MMLSetupSave: {
				SaveSetupPane();
				
				wantClearUi = true;
				want = WLevelMenu;
			} break;

			case MMLColorSave: {
				SaveToColorCenter();

				colorCenter = NULL;
				
				wantClearUi = true;
				want = WLevelMenu;
			} break;

			case MMLLayers: {
				SaveSetupPane();
				wantClearUi = true;
				want = WLayers;
			} break;
			
			case MMLLayersConfirm: {
				int layersCount = setLayers->is + 0.5;
				int layersOld = level[0].layers ? level[0].layers : 1;
				TiXmlNode *i = NULL;
				int temp;

							  ERR("DELETE DELETE DELETE DELETE DELETE %d %d\n", level[0].layers, layersCount);
							  
				if (layersOld > layersCount) {
					queue<TiXmlNode *> deleteme;
					while(i = editLevel->IterateChildren(i)) {
						if (i->Type() != TiXmlNode::ELEMENT)
							continue;
						if (i->ValueStr() != "File" && i->ValueStr() != "Color")
							continue;
						if (TIXML_SUCCESS != ((TiXmlElement *)i)->QueryIntAttribute("layer", &temp)
							|| temp < layersCount) // This must be 1. a file or color element 2. with a layer to be deleted
							continue;
						
							  ERR("DELETE: %d\n", temp);

						if (i->ValueStr() == "File") {
							rm_from_editpath(((TiXmlElement *)i)->Attribute("src"));
						}
						deleteme.push(i);
					}
					while(deleteme.size() > 0) {
						ERR("DELETE: %x\n", deleteme.front());
						editLevel->RemoveChild(deleteme.front());
						deleteme.pop();
					}
				} else if (layersOld < layersCount) {
					// Basically copied from MMNewLevel. I have been so cruel to the DRY principle in this program
					for(int c = layersOld; c < layersCount; c++) {
						TiXmlElement *tempData = new TiXmlElement("Color");
						tempData->SetAttribute("r", c%2?"1.0":"0.25");
						tempData->SetAttribute("g", (c/2)%2?"1.0":"0.25");
						tempData->SetAttribute("b", (c/4)%2?"1.0":"0.25");
						tempData->SetAttribute("layer", c);
						editLevel->LinkEndChild(tempData);
						
						for(int d = 0; d < 2; d++) {
							char filename[FILENAMESIZE];
							char filename2[FILENAMESIZE];
							snprintf(filename, FILENAMESIZE, "%d.png", random());
							
							tempData = new TiXmlElement("File");
							tempData->SetAttribute("src", filename);
							tempData->SetAttribute("rot", d);
							tempData->SetAttribute("layer", c);
							editLevel->LinkEndChild(tempData);
																						
							snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), filename);
							{
								int width = 16, height = 16;
								std::vector<unsigned char> image;
								image.resize(width * height * 4);
								uint32_t *data = (uint32_t *)&image[0];

								for(unsigned x = 0; x < width; x++)
								for(unsigned y = 0; y < height; y++)
								{ 
									data[y * width + x] = htonl(
										(0 == d && (x == 0 || y == 0 || x == width-1 || y == height-1))
											? C_FLOOR : 0 );
								}
								
								std::vector<unsigned char> buffer;
								LodePNG::Encoder encoder;
								encoder.encode(buffer, image, width, height);

								LodePNG::saveFile(buffer, filename2);
							}
						}
					}
				}
				
				if (layersCount > 1)
					((TiXmlElement *)editLevel)->SetAttribute ("layers", layersCount);
				else
					((TiXmlElement *)editLevel)->RemoveAttribute ("layers");
				
				editing->SaveFile();

				deepLoad(editLevel);

				wantClearUi = true;
				want = WLevelMenu;
			} break;
		}
	}
};

void UpdateSetupFields()
{
	TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);

	typeXml->SetDoubleAttribute("zoom", setZoom->is); // Geh?
	typeXml->SetAttribute("camera", (int)setCamera->checked);
	typeXml->SetAttribute("repeat", (int)setRepeat->checked);
#if FULL_REPROT
	if (setRepref->checked) {
		int reprot = 0;
		for(int c = 0; c < 4; c++) {
			reprot <<= 4;
			reprot |= ((int)(floor(setReprot[3-c]->is + 0.5)) & REPROTALL);
		} 
		typeXml->SetAttribute("reprot", reprot);
#else
	int repis = setReprot->is+0.5;
	if (setRepeat->checked && repis) {
		int repin = loops[repis];
		int repout = 0;
		for(int c = 0; c < 4; c++) {
			repout <<= 4;
			repout |= (repin % 10);
			repin /= 10;
		} 
		typeXml->SetAttribute("reprot", repout);
#endif
	} else {
		typeXml->RemoveAttribute("reprot");
	}
}

void LiveUpdateSetupFields() {	
	if (!dontLiveUpdate) {
		UpdateSetupFields();
		clearEverything(); // Overkill?
		loadLevel(editLevel, editingPath.c_str(), true);
		repaintEditSlice();
	}
}

void levelPopulateContainer(ContainerBase *c, bool fake = false) {
	int flagCount = 0;
   for( TiXmlNode *levelxml = editing->RootElement()->FirstChild(); levelxml; levelxml = levelxml->NextSibling() ) {
		if (levelxml->Type() != TiXmlNode::ELEMENT || levelxml->ValueStr() != "Level") continue;
		string src = srcFromLevel(levelxml);
		string name = nameFromLevel(levelxml);
		if (((TiXmlElement *)levelxml)->Attribute("flag")) {
			std::ostringstream str;
			str << "\nPath ";
			str << ++flagCount;
			name += str.str();
		}
		ControlBase *levelButton = new EditorMenuControl(name, fake?EMIgnore:EMLevel, levelxml, c->controls.size());
		
		if (!src.empty()) {
			char filename2[FILENAMESIZE];
			snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), src.c_str());
			levelButton->img = new slice();
			levelButton->img->construct(filename2, false);
		}
		c->add( levelButton );
	}
}

void wgo() {
	paused = false;
	onEsc = WPause;
	startTimer();
	jumpman_reset();
}

class FlagChoiceControl : public ControlBase
{public:
	int me; bool clearFirst;
	FlagChoiceControl(string _text = string(), int _me = 0, bool _clearFirst = false) : ControlBase(_text, true), me(_me), clearFirst(_clearFirst) {}
	virtual void click() {
		sjump.reset();
		
		if (clearFirst)
			wload(lastLoadedFilename, true);
		
		censorTimerData = (me != 0);
		
		jumpman_s = flags[me];
		jumpman_d = level[jumpman_s].deep+1;
		jumpman_flag = level[jumpman_s].flag-1;
ERR("ZZZZ NEWFLAG %d\n", jumpman_flag);
		
		if (level[0].haveEntry) {// This must be duplicating something, somewhere.
			jumpman_x = level[jumpman_s].entry_x;
			jumpman_y = level[jumpman_s].entry_y;
			jumpman_l = level[jumpman_s].entry_l;
		} else {
			jumpman_x = 0;
			jumpman_y = 0;
		}
		
		wantClearUi = true;
		wgo();
	}
};

void wload(string filename, bool suppressMenu) {
	clearEverything();
	loadGame(filename.c_str());
	lastLoadedFilename = filename;
	int availableLevelsCount = 1;

	if (scores.count(currentScoreKey))
		availableLevelsCount = scores[currentScoreKey].first.time.size() + 1;
	if (flags.size() < availableLevelsCount)
		availableLevelsCount = flags.size();
			
	if (suppressMenu || (availableLevelsCount <= 1 && scores[currentScoreKey].first.time.size() != 1)) {
		censorTimerData = false;
		wgo();
	} else {
		paused = true;
		onEsc = WMainMenu;
		
		pair<scoreinfo,scoreinfo> &s = scores[currentScoreKey];

		SCROLLMAX = 5;

		ScrollContainer *parent, *child;
		parent = new ScrollContainer(uiSpace, CMIDL);
		container[CMIDL] = parent;
		
		child = new ScrollContainer(uiSpace, CMID, false); // This is kinda messy...
		container[CMID] = child;
		parent->peers.push_back(child);
		child = new ScrollContainer(uiSpace, CMIDR, false);
		container[CMIDR] = child;
		parent->peers.push_back(child);
		
		bool completed = true, started = false;
		
		int firsttotaltime = 0, firsttotaldeaths = 0, secondtotaltime = 0, secondtotaldeaths = 0;
		
		for(int c = 0; c < availableLevelsCount; c++) {
			char filename2[FILENAMESIZE];	
			snprintf(filename2, FILENAMESIZE, "Path %d", c+1);
			container[CMIDL]->add( new FlagChoiceControl(filename2, c) );
			
			string left, right;
			if (c < s.first.time.size() && s.first.time[c]) {
				char filename[FILENAMESIZE];	
				
				started = true;
				
				firsttotaltime += s.first.time[c]; firsttotaldeaths += s.first.deaths[c];
				int seconds = s.first.time[c] / 1000;
				int minutes = seconds / 60;
				seconds %= 60;
				snprintf(filename, FILENAMESIZE, "%d:%s%d\n \n%d lives", minutes, seconds<10?"0":"", seconds, s.first.deaths[c]);
				left = filename;

				secondtotaltime += s.second.time[c]; secondtotaldeaths += s.second.deaths[c];
				seconds = s.second.time[c] / 1000; // Duplicated code from WMainMenu
				minutes = seconds / 60;
				seconds %= 60;
				snprintf(filename, FILENAMESIZE, "%d:%s%d\n \n%d lives", minutes, seconds<10?"0":"", seconds, s.second.deaths[c]);
				right = filename;
			} else 
				completed = false;
			container[CMID]->add( new ControlBase(left) );
			container[CMIDR]->add( new ControlBase(right) );
		}
		container[CMIDL]->commit();
		container[CMID]->commit();
		container[CMIDR]->commit();
		
		if (started) {
			container[CLEFT] = new ContainerBase(uiSpace, CMID);
			container[CRIGHT] = new ContainerBase(uiSpace, CMIDR);
			container[CLEFT]->add( new ControlBase("Best time:") );
			container[CRIGHT]->add( new ControlBase("Best score:") );
			for(int c = 0; c < availableLevelsCount && c < 5; c++) {
				container[CLEFT]->add(new ControlBase(""));
				container[CRIGHT]->add(new ControlBase(""));
			}
			
			if (completed && availableLevelsCount > 1) {
				char filename[FILENAMESIZE];	
				int seconds = firsttotaltime / 1000;
				int minutes = seconds / 60;
				seconds %= 60;
				snprintf(filename, FILENAMESIZE, "Total: %d:%s%d\n%d lives", minutes, seconds<10?"0":"", seconds, firsttotaldeaths);
				container[CLEFT]->add(new ControlBase(filename));
				
				seconds = secondtotaltime / 1000; // Duplicated code from WMainMenu
				minutes = seconds / 60;
				seconds %= 60;
				snprintf(filename, FILENAMESIZE, "Total: %d:%s%d\n%d lives", minutes, seconds<10?"0":"", seconds, secondtotaldeaths);
				container[CRIGHT]->add(new ControlBase(filename));
			} else {
				container[CLEFT]->add(new ControlBase(""));
				container[CRIGHT]->add(new ControlBase(""));
			}
			
			container[CLEFT]->commit();
			container[CRIGHT]->commit();
		}
	}
	
	videoStart(filename);
}

void serviceInterface() {
		if (wantClearUi) {
			haveLineStart = false;
			if (want == WIgnore) { // Awkward, but we need a way to eat escapes
				sland.reset();
				wantClearUi = false;
				return; // Bypass normal processing
			}
		
			for(int c = 0; c < CNUM; c++) {
				if (container[c]) {
					delete container[c];
					container[c] = NULL;
				}
			}
			
			resetScrollmax();
		} // Note: It's now possible to want without wantclearui-ing. USE THIS ONLY WITH GREAT CAUTION.
			
			switch (want) {
				case WMainMenuPaused:
					paused = true;
				case WMainMenu: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("Jumpman") );
					container[CMID]->add( new MainMenuControl("Game", MMPath1) );
					container[CMID]->add( new MainMenuControl("Editor", MMEdit) );
					container[CMID]->add( new MainMenuControl("Playground", MMDraw) );
//					container[CMID]->add( new MainMenuControl("View Controls", MMVC) );
					container[CMID]->add( new MainMenuControl("Options", MMOC) );
					//container[CMID]->add( new MainMenuControl("Controls", MMEC) );
					container[CMID]->add( new MainMenuControl("Quit", MMHalt) );
					container[CMID]->commit();
					
					if (haveTimerData && !censorTimerData) {
						// I can't believe STL string sucks so hard I'm finding myself doing this
						char filename2[FILENAMESIZE];
						int ticks = timerEndsAt - timerStartsAt;
						int seconds = ticks / 1000;
						int minutes = seconds / 60;
						seconds %= 60;
						
						snprintf(filename2, FILENAMESIZE, "Playtime: %d:%s%d\n \nLives lost: %d", minutes, seconds<10?"0":"", seconds, timerLives);
					
						container[CRIGHT] = new ContainerBase(uiSpace, CLEFT);
						container[CRIGHT]->add( new ControlBase(filename2) );
						for(int c = 0; c < 0; c++) 
							container[CRIGHT]->add( new ControlBase("") );
						container[CRIGHT]->commit();
					}
					
					container[CLEFT] = new ContainerBase(uiSpace, CRIGHT);
					for(int c = 0; c < 6; c++) 
						container[CLEFT]->add( new ControlBase("") );
					container[CLEFT]->add( new ControlBase("                      1.0.2") );
					container[CLEFT]->commit();
					
					drawControls = false;
					
					onEsc = WNothing;
				} break;
				
				case WDisplayOptions: {
					container[CRIGHT] = new ContainerBase(uiSpace, CRIGHT);
					container[CRIGHT]->add( new ControlBase("Options") );
					
					setOptions[4] = new CheckControl("Run in\na window", true);
					if (optWindow) setOptions[4]->click();
#if !LINUX
					container[CRIGHT]->add( setOptions[4] );
#endif
					
					setOptions[1] = new CheckControl("Tone down\nbackgrounds", true);
					if (optSlow) setOptions[1]->click();
					container[CRIGHT]->add( setOptions[1] );
					
					setOptions[0] = new CheckControl("I'm colorblind", true);
					if (optColorblind) setOptions[0]->click();
					container[CRIGHT]->add( setOptions[0] );
					
					setOptions[2] = new CheckControl("Angle hints", true);
					if (optAngle) setOptions[2]->click();
					container[CRIGHT]->add( setOptions[2] );
					
					setOptions[3] = new CheckControl("Splatter mode", true);
					if (optSplatter) setOptions[3]->click();
					if (haveWonGame)
						container[CRIGHT]->add( setOptions[3] );
					container[CRIGHT]->add( new MainMenuControl("Back", MMOSave) );
					container[CRIGHT]->commit();	
					
					container[CMIDL] = new SquishContainer(uiSpace, CMIDL);
					populateControlsScreen(container[CMIDL]);
					container[CMIDL]->commit();
				} break;
				
				case WEditMenu: {
					container[CMID] = new ScrollContainer(uiSpace, CMID);
					container[CMID]->add( new MainMenuControl("New", MMNewFile) );

					DIR *dird = opendir(".");
					dirent *dir;
					while (dir = readdir(dird)) {
						if (strlen(dir->d_name) < 4 || strncmp( ".jmp", dir->d_name + strlen(dir->d_name) - 4, 4 ))
							continue;
						dir->d_name[strlen(dir->d_name) - 4] = '\0'; // THIS DOESN'T HAVE SIDE EFFECTS I ASSUME
						container[CMID]->add( new MainMenuControl(dir->d_name, MMNewFile) );
					}
#if SELF_EDIT
					container[CMID]->add( new MainMenuControl("../../Internal/Main", MMNewFile) );
#endif
					container[CMID]->commit();
					onEsc = WMainMenu;
				} break;
				case WNewFileEntry: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("Enter a name:") );
					ControlBase *onEnter = new MainMenuControl("Save", MMEnteredNewFile);
					scratchTextControl = new TextControl("", true, onEnter);
					container[CMID]->add( scratchTextControl );
					container[CMID]->add( onEnter );
					container[CMID]->commit();
					onEsc = WMainMenu;
				} break;
				
				case WCurrentFileDirectory: {
					currentlyScrolling = new ScrollContainer(uiSpace, CMID);
					container[CMID] = currentlyScrolling;
					currentlyScrolling->scroll = lastlevelscroll;
					levelPopulateContainer(container[CMID]);
					container[CMID]->commit();

					container[CLEFT] = new ContainerBase(uiSpace, CLEFT);
					container[CLEFT]->add( new ControlBase(editDisplayName) );
					if (container[CMID]->controls.size() > 0)
						container[CLEFT]->add( new MainMenuControl("Play", MMEditPlay) );
					container[CLEFT]->commit();
					
					container[CRIGHT] = new ContainerBase(uiSpace, CRIGHT);
					container[CRIGHT]->add( new MainMenuControl("New Level", MMNewLevel) );
					container[CRIGHT]->commit();

					onEsc = WEditMenu;
				} break;

				// Ugly series of fallthroughs proceed

				case WSaveAngleThenLevelMenu: {
					TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
					
					int rotcount = 0;
					for(int c = 0; c < 8; c++) {
						if (level[jumpman_s].rots & (1<<c))
							rotcount++;
					}
					double rspeed = 1.0;
					if (rotcount)
						rspeed = 8.0/rotcount;
					ERR("rotcount %d rspeed %lf", rotcount, rspeed);
					
					typeXml->SetAttribute("rots", level[jumpman_s].rots);
					typeXml->SetAttribute("dontrot", level[jumpman_s].dontrot);
					typeXml->SetAttribute("rspeed", rspeed);
				}
					

				case WSaveThenLevelMenu:
					editing->SaveFile();
										
				case WLevelMenu: {
					container[CMIDL] = new ContainerBase(uiSpace, CMIDL);
					container[CMIDR] = new ContainerBase(uiSpace, CMIDR);
					
					container[CMIDL]->add( new MainMenuControl("Test", MMLTest) );
					container[CMIDR]->add( new MainMenuControl("Edit", MMLWalls) );
					container[CMIDL]->add( new MainMenuControl("Setup", MMLSetup) );
					container[CMIDR]->add( new MainMenuControl("Move", MMLUpDown) );
					container[CMIDL]->add( new MainMenuControl("Color", MMLColor) );
					container[CMIDR]->add( new MainMenuControl("Angles", MMLAngle) );
					container[CMIDL]->add( new MainMenuControl("Dupe", MMLDupe) );
					container[CMIDR]->add( new MainMenuControl("Delete", MMLDelete) );
					
					container[CMIDL]->commit();
					container[CMIDR]->commit();
				
					container[CMID] = new ContainerBase(uiSpace, CMID);
					for(int c = 0; c < 6; c++)
						container[CMID]->add(new ControlBase(""));
					container[CMID]->add( new MainMenuControl("Back", MMLBack) );
					container[CMID]->commit();
					
					{ // Previous button
						TiXmlNode *sib = NULL;
						for(TiXmlNode *n = editLevel->PreviousSibling(); n && !sib; n = n->PreviousSibling()) {
							if (n->Type() == TiXmlNode::ELEMENT && n->ValueStr() == "Level")
								sib = n;
						}
						if (sib) {
							container[CLEFT] = new ContainerBase(uiSpace, CLEFT);
							ControlBase *levelButton = new EditorMenuControl("Last Level", EMLevel, sib, currentleveloffset-1);
							for(int c = 0; c < 6; c++)
								container[CLEFT]->add(new ControlBase(""));
							container[CLEFT]->add(levelButton);
							container[CLEFT]->commit();
						}							
					}
					
					{ // Next button -- FIXME code duplication
						TiXmlNode *sib = NULL;
						for(TiXmlNode *n = editLevel->NextSibling(); n && !sib; n = editLevel->NextSibling()) {
							if (n->Type() == TiXmlNode::ELEMENT && n->ValueStr() == "Level")
								sib = n;
						}
						if (sib) {
							container[CRIGHT] = new ContainerBase(uiSpace, CRIGHT);
							ControlBase *levelButton = new EditorMenuControl("Next Level", EMLevel, sib, currentleveloffset+1);
							for(int c = 0; c < 6; c++)
								container[CRIGHT]->add(new ControlBase(""));
							container[CRIGHT]->add(levelButton);
							container[CRIGHT]->commit();
						}							
					}
					
					paused = true;
					edit_mode = ENothing;
					onEsc = WCurrentFileDirectory;
					drawPalette = true;
					editLayer = 0;
					
					// There may be some redundancies here, at some point I should clean that up...
					clearEverything();
					loadLevel(editLevel, editingPath.c_str(), true);
					repaintEditSlice();

					jumpman_s = 0;
					scan_x = 0; scan_y = 0; scan_r = 1; // Do this somewhere else?
					jumpman_reset(false);
					chassis->p.x = 0; chassis->p.y = 0; // Center camera (does it matter if this happens before or after jumpman_reset()?)
				} break;
				
				case WPalette: {
					char filename[FILENAMESIZE];
					PaletteControl *b;
					slice *s;
					
					container[CLEFT] = new ColumnContainer(uiSpace, CLEFT);

					b = new PaletteControl("", &paletteGroup, C_FLOOR, "Floor\nDraw the floors and walls of your level with these.");
					s = new slice(); b->img = s;
					internalPath(filename, "wall.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);
					paletteDefault = b; b->click(); // Must start off selected

					b = new PaletteControl("", &paletteGroup, C_INVIS, "Invisible Wall\nRoving enemies will bounce off of these.\nEverything else passes through as if nothing were there.");
					s = new slice(); b->img = s;
					internalPath(filename, "invisible.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);

					b->egg = new PaletteControl("", &paletteGroup, C_LOOSE3, "Invisible Shrapnel\nLike the invisible wall, but disappears instantly when the bomb goes off.\nSelect this by control-clicking the \"invisible wall\" icon.");
					

					b = new PaletteControl("", &paletteGroup, C_MARCH, "Spiny\nBasic enemy. Marches until it hits something, then turns around.");
					s = new slice(); b->img = s;
					internalPath(filename, "kyou_spiny 1.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);

					b = new PaletteControl("", &paletteGroup, C_ING, "Hunter\nWill chase after Jumpman whenever he is present.");
					s = new slice(); b->img = s;
					internalPath(filename, "eyes5_hunter 2.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);


					b = new PaletteControl("", &paletteGroup, C_STICKY, "Sticky\nMoves back and forth like a spiny, but isn't subject to gravity.\nRotates along with the wall it's aligned with.");
					s = new slice(); b->img = s;
					internalPath(filename, "kyou_sticky 1.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);
					b = new PaletteControl("", &paletteGroup, C_SWOOP, "Swoopy\nFlying enemy. Always flies at a 45 degree angle to gravity.");
					s = new slice(); b->img = s;
					internalPath(filename, "kyou_swoopy 2.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);					

					b = new PaletteControl("", &paletteGroup, C_ENTRY, "Entrance\nIf this tile is present, Jumpman starts the level at that position.\nYou can only have one of these in a level."); // TODO: special draw behavior
					s = new slice(); b->img = s;
					internalPath(filename, "jumpman1 3.png");
					s->construct(filename, false);	
					container[CLEFT]->add(b);
					
					b->egg = new PaletteControl("", &paletteGroup, C_REENTRY, "Checkpoint\nWhen Jumpman touches this, his position becomes his new spawn point.\nSelect this by control-clicking the \"Entrance\" icon.");

					
					b = new PaletteControl("", &paletteGroup, C_EXIT, "Exit\nIf Jumpman touches this, he escapes to the next level.");
					s = new slice(); b->img = s;
					internalPath(filename, "exit.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);
					
					b = new PaletteControl("", &paletteGroup, C_LAVA, "Lava\nThe flashy stuff. Kills Jumpman with one touch, effects nothing else.");
					s = new slice(); b->img = s;
					internalPath(filename, "invisible_lava.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);
					
					b = new PaletteControl("", &paletteGroup, C_BALL, "Superball\nKills Jumpman with a touch, but also kills enemies. Bounces.\nActually the only way to kill enemies in this game.");
					s = new slice(); b->img = s;
					internalPath(filename, "ball.png");
					s->construct(filename, false);
					container[CLEFT]->add(b);

					container[CLEFT]->add( new ControlBase() );
					container[CLEFT]->add( new ControlBase() );

					container[CMIDL] = new ContainerBase(uiSpace, CLEFT);
					for(int c = 0; c < 5; c++)
						container[CMIDL]->add( new ControlBase() );
					container[CMIDL]->add( new MainMenuControl("More", MMEMore) );
					container[CMIDL]->commit();
					container[CLEFT]->commit();
										
				} 
				if (!beenUsingFullPalette)
					break;
				
				case WMorePalette: {
					delete container[CMIDL]; container[CMIDL] = NULL;
					container[CMIDL] = new ContainerBase(uiSpace, CLEFT);
					for(int c = 0; c < 5; c++)
						container[CMIDL]->add( new ControlBase() );
					
					if (container[CRIGHT]) { // LESS
						paletteDefault->click();
						delete container[CRIGHT]; container[CRIGHT] = NULL;
						container[CMIDL]->add( new MainMenuControl("More", MMEMore) );
						beenUsingFullPalette = false;
					} else { // MORE
						char filename[FILENAMESIZE];
						PaletteControl *b;
						slice *s;

						container[CMIDL]->add( new MainMenuControl("Less", MMEMore) );

						container[CRIGHT] = new ColumnContainer(uiSpace, CRIGHT);


						b = new PaletteControl("", &paletteGroup, C_LINE, "Line Tool\nClick or right-click twice with this\nto draw or erase a straightened line of wall blocks.");
						s = new slice(); b->img = s;
						internalPath(filename, "drag3.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);
						
						b = new PaletteControl("", &paletteGroup, C_SCROLL, "Scroll Tool\nClick and drag to move blocks. Stuff at the edges will wrap.");
						s = new slice(); b->img = s;
						internalPath(filename, "drag2.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);
						

						b = new PaletteControl("", &paletteGroup, C_BOMB, "Bomb\nIf Jumpman touches this, all \"shrapnel\" tiles (the blinking\n floors) come unpinned from the wall and clatter to the floor.");
						s = new slice(); b->img = s;
						internalPath(filename, "kyou_bomb 1.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);
						
						b = new PaletteControl("", &paletteGroup, C_LOOSE, "Shrapnel\nThese act like normal floor spaces until the bomb is touched,\nthen fall apart into a bunch of loose, slightly bouncy objects.");
						s = new slice(); b->img = s;
						internalPath(filename, "bombable.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);
												
						b->egg = new PaletteControl("", &paletteGroup, C_LOOSE2, "Evaporating Shrapnel\nLike the shrapnel, but disappears instantly when the bomb goes off.\nSelect this by control-clicking the \"shrapnel\" icon.");
						
						{
							PaletteControl *sb = new PaletteControl("", &paletteGroup, C_BOMB2, "Superbomb\nExactly like the bomb, but Jumpman can't trigger it.\nIt only triggers when the superball touches it.");
							s = new slice(); sb->img = s;
							internalPath(filename, "kyou_bomb 2.png");
							s->construct(filename, false);
						
							if (level[0].layers > 1) {
								b = new PaletteControl("", &paletteGroup, C_PAINT, "Paintbrush\nIf Jumpman touches this, he changes color.\nOnly works in levels with color layering.");
								s = new slice(); b->img = s;
								internalPath(filename, "paintbrush.png");
								s->construct(filename, false);
								b->egg = sb;
								container[CRIGHT]->add(b);
							} else {
								container[CRIGHT]->add(sb);
							}
						}
						
						b = new PaletteControl("", &paletteGroup, C_NOROT, "No Rotation Zone\nYou can pass through this freely, but can't\nrotate while touching it.");
						s = new slice(); b->img = s;
						internalPath(filename, "norot.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);

						happyCache = new PaletteControl("", &paletteGroup, C_HAPPY, "Happy Ball\nSuperball that does not harm Jumpman, only enemies.");
						s = new slice(); happyCache->img = s;
						internalPath(filename, "ball_happy.png");
						s->construct(filename, false);

						angryCache = new PaletteControl("", &paletteGroup, C_ANGRY, "Angry Ball\nSuperball that does not harm enemies, only Jumpman.");
						s = new slice(); angryCache->img = s;
						internalPath(filename, "ball_sad.png");
						s->construct(filename, false);
							
						if (level[0].layers > 1) {
							container[CRIGHT]->add( new MainMenuControl("(Last\nLayer)", MMEPrevLayer) );
							container[CRIGHT]->add( new MainMenuControl("(Next\nLayer)", MMENextLayer) );
						} else {
							container[CRIGHT]->add(happyCache);
							container[CRIGHT]->add(angryCache);
						}


						b = new PaletteControl("", &paletteGroup, C_ARROW, "Arrow\nDoesn't do anything. For decoration.");
						s = new slice(); b->img = s;
						internalPath(filename, "arrow.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);
												
						b = new PaletteControl("", &paletteGroup, C_BACK, "Unexit\nExit that takes you back a level rather than forward.\nNot used in the actual game itself. Use with caution.");
						s = new slice(); b->img = s;
						internalPath(filename, "exit2.png");
						s->construct(filename, false);
						container[CRIGHT]->add(b);
						
						
						container[CRIGHT]->add( new PaletteControl("Resize\n(Even)", &paletteGroup, C_RESIZE, "Resize (Even)\nClick a square and the place you clicked\nwill become the new outer border of the level.\nThis is easier to use if the grid is turned on.") );
									
						container[CRIGHT]->add( new PaletteControl("Resize\n(Odd)", &paletteGroup, C_RESIZE2, "Resize (Odd)\nClick a square and the place you clicked\nwill become the new outer border of the level.\nThis is easier to use if the grid is turned on.") );

						container[CRIGHT]->commit();
						
						beenUsingFullPalette = true;
					}
					container[CMIDL]->commit();
				} break;
				
				case WSave: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					
					container[CMID]->add( new ControlBase("", false) );
					container[CMID]->add( new ControlBase("", false) );
					container[CMID]->add( new ControlBase("Save this level?", false) );
					container[CMID]->add( new MainMenuControl("Yes, save", MMLSave) );
					container[CMID]->add( new MainMenuControl("No, revert", MMLDontSaveRevert) );
					container[CMID]->add( new ControlBase("", false) );
					container[CMID]->add( new MainMenuControl("Keep editing", MMLWallsDirty) );
					container[CMID]->commit();
					onEsc = WIgnore;
					drawPalette = true;
					paused = true;
				} break;
				
				case WLevelDupe: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("Make a copy of this level?", false) );
					container[CMID]->add( new MainMenuControl("Yes", MMLDupeConfirm) );
					container[CMID]->add( new MainMenuControl("No", MMLDontSave) );
					container[CMID]->commit();
					onSave = WLevelMenu;
					onEsc = WIgnore;
				} break;
				
				case WLevelDelete: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("*Delete* this level and all its files?", false) );
					container[CMID]->add( new MainMenuControl("Yes, delete", MMLDeleteConfirm) );
					container[CMID]->add( new MainMenuControl("No, don't", MMLDontSave) );
					container[CMID]->commit();
					onSave = WLevelMenu;
					onEsc = WIgnore;
				} break;
					
				case WSetup: {
					dontLiveUpdate = true;
					
					container[CMIDL] = new ContainerBase(uiSpace, CMIDL);
				
					setCamera = new LiveCheckControl("Fixed camera", true);
					container[CMIDL]->add( setCamera );

//					container[CMIDL]->add( new ControlBase("", false) );

					container[CMIDL]->add( new ControlBase("Distance to next level:") );

					setAfter = new WheelControl(1, 1, 4, 1);
					container[CMIDL]->add( setAfter );

					container[CMIDL]->add( new ControlBase("Zoom:") );

					setZoom = new LiveWheelControl(1, 0.10, 4, 0.10);
					container[CMIDL]->add( setZoom );
				
					container[CMIDL]->commit();
				
					container[CMID] = new ContainerBase(uiSpace, CMID);
					setName = new TextControl(nameFromLevel(editLevel), false);
					container[CMID]->add( setName );
					
					container[CMID]->add( new ControlBase("", false) );
					
					container[CMID]->add( new ControlBase("Message (optional):") );
					setMessage = new TextControl("", false);
					container[CMID]->add( setMessage );
					{
						TiXmlElement *messageXml = (TiXmlElement *)editLevel->IterateChildren("Message", NULL);
						if (messageXml)
							setMessage->text = messageXml->Attribute("text");
					}

					setFlag = new CheckControl("Flag", true);
					{ int temp; if (TIXML_SUCCESS == ((TiXmlElement *)editLevel)->QueryIntAttribute("flag", &temp)) setFlag->click(); }
					container[CMID]->add( setFlag );
					
					container[CMID]->add( new ControlBase("", false) );
					container[CMID]->add( new MainMenuControl("Save", MMLSetupSave) );
					container[CMID]->commit();
					
					container[CMIDR] = new ContainerBase(uiSpace, CMIDR);
					
					setRepeat = new LiveCheckControl("Repeat", true);
					container[CMIDR]->add( setRepeat );
					
#if FULL_REPROT
					setRepref = new LiveCheckControl("Strange loop", true);
					container[CMIDR]->add( setRepref );

					for(int c = 0; c < 4; c++) {
						setReprot[c] = new LiveWheelControl(0, 0, 7, 1);
						container[CMIDR]->add( setReprot[c] );
					}
#else
					{
						vector<string> options;
						for(int c = 0; c < NUMLOOPS; c++) {
							char filename2[FILENAMESIZE];
							snprintf(filename2, FILENAMESIZE, "%c %c %c %c", '0'+(loops[c]/1000)%10, '0'+(loops[c]/100)%10, '0'+(loops[c]/10)%10, '0'+(loops[c]*1)%10);
							options.push_back(filename2);
						}
						container[CMIDR]->add( new ControlBase("Twisted?:") );
						setReprot = new LiveSelectControl(options);
						container[CMIDR]->add(setReprot);
						container[CMIDR]->add( new ControlBase("") );
					}
#endif

					container[CMIDR]->add( new MainMenuControl("Layers", MMLLayers) );

					container[CMIDR]->commit();

					TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
					int checked;

					// FIXME: This sucks.
					checked = 0; typeXml->QueryIntAttribute("camera", &checked); if (checked) setCamera->click();
					checked = 0; typeXml->QueryIntAttribute("repeat", &checked); if (checked) setRepeat->click();
					checked = 0; if (TIXML_SUCCESS == typeXml->QueryIntAttribute("reprot", &checked)) {
#if FULL_REPROT
						setRepref->click();
						for(int c = 0; c < 4; c++) {
							setReprot[c]->is = checked & REPROTALL;
							setReprot[c]->changedIs();
							checked >>= 4;
						}
#else
						int repdec = 0;
						for(int c = 0; c < 4; c++) {
							repdec *= 10;
							repdec += (checked & REPROTALL);
							checked >>= 4;
						}
						for(int c = 0; c < NUMLOOPS; c++) {
							if (loops[c] == repdec) {
								setReprot->is = c;
								setReprot->changedIs();
							}
						}
#endif
					}

					checked = 1; if (TIXML_SUCCESS == typeXml->QueryIntAttribute("after", &checked)) {	
						setAfter->is = checked;
						setAfter->changedIs();
					}
					double zchecked = 1; if (TIXML_SUCCESS == typeXml->QueryDoubleAttribute("zoom", &zchecked)) {	
						setZoom->is = zchecked;
						setZoom->changedIs();
					}
					
					dontLiveUpdate = false;
				} break;

				case WColor: {
					colorCenter = new ColorCenter("RGB/HSV");
									
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMIDL] = new ContainerBase(uiSpace, CMIDL);
					container[CMIDR] = new ContainerBase(uiSpace, CMIDR);
					
					if (level[0].layers > 1) {
						container[CMIDL]->add(new ControlBase("For layer:"));
						setLayers = new LayerSwitchWheelControl(1, 1, level[0].layers, 1);
						container[CMIDR]->add(setLayers);
						
						container[CMIDL]->add(new ControlBase(""));
						container[CMID]->add(new ControlBase(""));
						container[CMIDR]->add(new ControlBase(""));
					} else {
						container[CMID]->add( new ControlBase("Use the scrollwheel to set values below.") );
					}
					
					container[CMID]->add( colorCenter ); // Uncomment when ready
					for(int c = 0; c < 4; c++)
						container[CMID]->add(new ControlBase(""));
					container[CMID]->add( new MainMenuControl("Save", MMLColorSave) );
					container[CMID]->commit();
					
					for(int c = 0; c < 3; c++)
						container[CMIDL]->add( colorCenter->labels[c] );

					for(int c = 0; c < 3; c++)
						container[CMIDR]->add( colorCenter->colors[c] );

					if (level[0].layers > 1) {						
						container[CMIDL]->add(new ControlBase(""));
						container[CMIDR]->add(new ControlBase(""));
						container[CMIDL]->add(new ControlBase(""));
						container[CMIDR]->add(new ControlBase(""));
					}
					container[CMIDL]->commit();
					container[CMIDR]->commit();

					LoadFromColorCenter();
				} break;
				
				case WLayers: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("This sets the number of color layers this level has.\nIf there is more than one layer, Jumpman will be stuck\n"
						"inside one at a time, and will only be able to move between\nthem using paintbrushes. In the editor, new buttons will\nappear to let you choose which layer you are editing.\n"
						"\nKeep in mind that if you reduce the number of layers in this \nlevel, the files for the extra layers will be deleted.") );
						
					container[CMID]->add(new ControlBase("\n\nLayers:"));
					setLayers = new WheelControl(1, 1, 16, 1);
					setLayers->is = level[0].layers ? level[0].layers : 1;
					setLayers->changedIs();
					container[CMID]->add( setLayers );
					container[CMID]->add(new ControlBase(""));
					container[CMID]->add( new MainMenuControl("Save", MMLLayersConfirm) );
					container[CMID]->commit();

					onSave = WLevelMenu;
				} break;
				
				case WAngle: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("Click to add or remove an allowed rotation point.\nRight-click to add or remove a barrier.\nESC to save and exit.") );
					container[CMID]->commit();

					edit_mode = EAngle;
					level[jumpman_s].rspeed = 1;
					onEsc = WSaveAngleThenLevelMenu;
				} break;

				case WUpDown: {
					container[CMIDL] = new ContainerBase(uiSpace, CMIDL);
					levelPopulateContainer(container[CMIDL], true);
					container[CMIDL]->controls[currentleveloffset]->highlighted = true;
					container[CMIDL]->commit();
					
					container[CMIDL]->y = -container[CMIDL]->controls[currentleveloffset]->p.y;
					container[CMIDL]->recommit();
					
					container[CMIDR] = new ContainerBase(uiSpace, CMIDR);
					if (0 < currentleveloffset)
						container[CMIDR]->add( new MainMenuControl("Up", MMLUp) );
					else
						container[CMIDR]->add( new ControlBase("Up", false) );
						
					if (currentleveloffset < container[CMIDL]->controls.size()-1)
						container[CMIDR]->add( new MainMenuControl("Down", MMLDown) );
					else
						container[CMIDR]->add( new ControlBase("Down", false) );
					
					container[CMIDR]->commit();
					
				} break;
														
				case WPlay: { // Duplication...?
					currentScoreKey = editingPath;					
					wload(editingPath); // Will set WPause
				} break;
										
				case WReset: {
					char filename[FILENAMESIZE];
					internalPath(filename, "Main.jmp");
					currentScoreKey = "/";
					wload(filename);
				} break;

				case WEndingExit: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					container[CMID]->add( new ControlBase("Skip the credits?", false) );
					container[CMID]->add( new MainMenuControl("Yes, skip", MMStopCredits) );
					container[CMID]->add( new MainMenuControl("No, keep watching", MMResumeCredits) );
					container[CMID]->commit();
					onEsc = WResumeCredits;
					drawPalette = true;
				} break;
				
				case WResumeCredits: {
					onEsc = WEndingExit;
				} break;

				case WPause: {
					container[CMID] = new ContainerBase(uiSpace, CMID);
					
					container[CMID]->add( new ControlBase("Paused", false) );
					container[CMID]->add( new ControlBase("", false) );
					container[CMID]->add( new ControlBase("Quit the game?", false) );
					container[CMID]->add( new MainMenuControl("Yes, quit", MMStopPlaying) );
					container[CMID]->add( new MainMenuControl("No, keep playing", MMUnpause) );
					container[CMID]->add( new ControlBase("", false) );
					{
						char filename2[FILENAMESIZE];	
						snprintf(filename2, FILENAMESIZE, "Restart Path %d", jumpman_flag+1);
						
						container[CMID]->add( new FlagChoiceControl(filename2, jumpman_flag, true) );
					}
					container[CMID]->commit();
					onEsc = WUnpause;
					drawPalette = true;
										
					pause(true);
				} break;
				
				case WUnpause: {
					pause(false);
					onEsc = WPause;
				} break;
				
				default:break;
			}
			
			want = WNothing;
			wantClearUi = false;
}
