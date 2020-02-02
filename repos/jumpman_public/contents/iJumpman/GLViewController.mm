//
//  GLViewController.m
//  iJumpman
//
//  Created by Andi McClure on 4/24/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "EAGLView.h"
#import "iJumpman.h"
#import "GLViewController.h"

#include "lodepng.h"
#include "color.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "TinyLang.h"

#define JUMP_TO_ENDING_DEBUG 0

#if 0&&TARGET_IPHONE_SIMULATOR
#define JLDB @"http://127.0.0.1:8000"
#else
#define JLDB @"http://jldb.runhello.com"
#endif

bool wantResetEverything = false;

Screen escScreen = PAUSE_SCREEN, winScreen = MAIN_SCREEN;
JumpView *lastGl = nil;
UITabBarController *lastOptions = nil; // KLUDGE

bool dirtyLevel = false;
vector<bool>dirtySlice;

bool needRecenter = false;

unsigned int lastChosenTool = 0;
const NSString *lastChosenToolImage = nil;

WebTarget whyWeb = NO_WEB;
DirectoryTarget whyDirectory = NO_DIRECTORY;
HelpTarget whyHelp = NO_HELP, originalWhyHelp = NO_HELP;
Screen afterHelp = NO_SCREEN;
bool help_ok = false;
int stageWeb = 0;
string editBaseName;

NSString *login_name = nil, *login_pass = nil, *login_email = nil;
bool have_saved_login = false;

int lastLevelTag = -1;

void killMusic();

void goWeb(WebTarget web) {
    whyWeb = web;
    stageWeb = 0;
    switchScreen(WEB_SCREEN, kCATransitionPush, kCATransitionFromRight);
}

UINavigationController *addNav(UIViewController *to, NSString *backString = _(@"Save"), bool withEdit = false) {
    UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:to];

    CGRect frame = CGRectMake(0.0, 0.0, 100.0, 40.0);
    
    UIBarButtonItem *backButton = [[[UIBarButtonItem alloc]
                                    initWithTitle: backString
                                    style:UIBarButtonItemStyleBordered // Could be "done" but this looks silly?
                                    target:to action:@selector(doEsc:)] autorelease]; 
    
    to.navigationItem.leftBarButtonItem = backButton;
    
    if (withEdit) {
        UIBarButtonItem *editButton = [[[UIBarButtonItem alloc]
                                        initWithTitle: _(@"Edit")
                                        style:UIBarButtonItemStyleBordered // Could be "done" but this looks silly?
                                        target:to action:@selector(doEdit:)] autorelease];
        editButton.possibleTitles = [NSSet setWithObjects:_(@"Edit"), _(@"Done"), nil];
        
        to.navigationItem.rightBarButtonItem = editButton;
    }
    
    return nav;
}

NSString *PAD(NSString *in) {
    if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad)
        return in;
    else 
        return [@"ipad-" stringByAppendingString: in];
}

UIViewController * getScreen(Screen screen) {
    switch(screen) {
        case NO_SCREEN:
            return nil;
        case AFTER_ENDING_MAIN_SCREEN:
            wantResetEverything = true;
            endEnding();
        case MAIN_SCREEN:
            return [[GLMainController alloc] initWithNibName:PAD(@"MainMenu") bundle:nil];
        case DEBUG_SCREEN:
#if !FOG_DEBUG
            return [[GLDebugController alloc] initWithNibName:@"DebugWindow" bundle:nil];
#else
            return [[GLLopt3Controller alloc] initWithNibName:@"Lopt3Window" bundle:nil];
#endif
        case OPTIONS_SCREEN:
            return [[GLOptionsController alloc] initWithNibName:PAD(@"OptionsWindow") bundle:nil];
        case EDITOR_SCREEN: {
            UIViewController *ed = [[GLEditorController alloc] initWithNibName:PAD(@"EditorWindow") bundle:nil];
            ed.title = _(@"Edit");
            return ed;
        }
        case PAUSE_SCREEN:
            return [[GLPauseController alloc] initWithNibName:PAD(@"PauseWindow") bundle:nil];
        case PALETTE_SCREEN:
            return [[GLPaletteController alloc] initWithStyle:UITableViewStylePlain];
        case DIRECTORY_SCREEN:
            return addNav([[GLDirectoryController alloc] initWithStyle:UITableViewStylePlain], _(@"Back"), true);
        case LEVEL_SCREEN:
            escScreen = LEVEL_SCREEN; winScreen = escScreen; // So the wrong place to do this.
            pause(true); // Doubly so
            killMusic(); // Like quadruply so

            return [[GLLevelController alloc] initWithNibName:PAD(@"LevelWindow") bundle:nil];
        case CREATE_SCREEN:
            return [[GLCreateController alloc] initWithNibName:PAD(@"CreateWindow") bundle:nil];
        case PACK_SCREEN:
            return [[GLPackController alloc] initWithNibName:PAD(@"PackWindow") bundle:nil];
        case SIGNUP_SCREEN:
            return [[GLSignupController alloc] initWithNibName:PAD(@"SignupWindow") bundle:nil];
        case LOPTIONS_SCREEN: { 
            dirtyLevel = true; // FIXME: Not necessary! Defer this
            
            UITabBarController *tabBarController = [[UITabBarController alloc] initWithNibName:nil bundle:nil];
            // Create three different custom view controllers
            GLLopt1Controller *viewController1 = [[[GLLopt1Controller alloc] initWithNibName:PAD(@"Lopt1Window") bundle:nil] autorelease];
            GLLopt2Controller *viewController2 = [[[GLLopt2Controller alloc] initWithNibName:PAD(@"Lopt2Window") bundle:nil] autorelease];
            GLLopt3Controller *viewController3 = [[[GLLopt3Controller alloc] initWithNibName:PAD(@"Lopt3Window") bundle:nil] autorelease];
            GLLopt4Controller *viewController4 = [[[GLLopt4Controller alloc] initWithNibName:PAD(@"Lopt4Window") bundle:nil] autorelease];
            GLLopt5Controller *viewController5 = [[[GLLopt5Controller alloc] initWithNibName:PAD(@"Lopt5Window") bundle:nil] autorelease];
            tabBarController.viewControllers = [NSArray arrayWithObjects:addNav(viewController1), addNav(viewController3), addNav(viewController4), addNav(viewController5), addNav(viewController2), nil];

            lastOptions = tabBarController;
            return tabBarController;
        }
        case INDEX_SCREEN:
            return [[GLIndexController alloc] initWithNibName:PAD(@"IndexWindow") bundle:nil];
        case WARP_SCREEN:
            return [[GLWarpController alloc] initWithNibName:PAD(@"WarpWindow") bundle:nil];
        case HELP_SCREEN: {
            if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad) {
                NSString *whichHelp = nil;
                switch (whyHelp) { // Maybe not the best place to do this.
                    case STARTUP_HELP: case OPTIONS_HELP:
                        originalWhyHelp = whyHelp;
                        if (savedControlMove & CButtonMove)
                            whyHelp = BUTTON_ESC_HELP;
                        else
                            whyHelp = MOVE_HELP;
                    break;
                }
                switch (whyHelp) { // Maybe not the best place to do this.
                    case MOVE_HELP: whichHelp = @"HelpWindow"; break;
                    case JUMP_HELP: whichHelp = @"HelpWindow2"; break;
                    case ESC_HELP: whichHelp = @"HelpWindow3"; break;
                    case BUTTON_ESC_HELP: whichHelp = @"HelpWindow4"; break;
                    case ROT_GRAV_HELP: whichHelp = @"HelpWindow5"; break;
                    case ROT_KNOB_HELP: whichHelp = @"HelpWindow6"; break;
                }
                UIViewController *help = [[GLHelpController alloc] initWithNibName:whichHelp bundle:nil];
                
                if (whyHelp == BUTTON_ESC_HELP) {
                    CGAffineTransform transform = CGAffineTransformMakeRotation(M_PI/2);
                    help.view.transform = transform;
                    CGPoint p = {160,240};
                    help.view.center = p;
                }
                
                return help;
            } else {
                UIViewController *help = [[GLMegaHelpController alloc] initWithNibName:@"ipad-MegaHelpWindow" bundle:nil];
                                
                return help;
            }
        }
        case SKIP_SCREEN:
            return [[GLSkipController alloc] initWithNibName:PAD(@"SkipWindow") bundle:nil];
        case SKIP2_SCREEN:
            return [[GLSkipController2 alloc] initWithNibName:PAD(@"SkipWindow2") bundle:nil];            
        case DELETE_SCREEN:
            return [[GLDeleteController alloc] initWithNibName:PAD(@"DeleteWindow") bundle:nil];
        case DELETE_PACK_SCREEN:
            return [[GLDeletePackController alloc] initWithNibName:PAD(@"DeletePackWindow") bundle:nil];
        case WEB_SCREEN: {
            UIViewController *web = [[GLWebController alloc] initWithNibName:PAD(@"WebWindow") bundle:nil];
            return addNav(web, _(@"Back"));
        }
    }    
    return nil;
}


#define kAnimationKey @"transitionViewAnimation"

// Method to replace a given subview with another using a specified transition type, direction, and duration. From TransitionView example code.
void replaceSubview(UIView *selfView, UIView * oldView, UIView *newView, NSString *transition, NSString *direction, NSTimeInterval duration) {
	// If a transition is in progress, do nothing
	if(transitioning) {
		return;
	}
	
	if (oldView) {
		NSArray *subViews = [selfView subviews];
		NSUInteger index;
		
		if ([oldView superview] == selfView) {
			// Find the index of oldView so that we can insert newView at the same place
			for(index = 0; [subViews objectAtIndex:index] != oldView; ++index) {}
			[oldView removeFromSuperview];
		}
		
		// If there's a new view and it doesn't already have a superview, insert it where the old view was
		if (newView && ([newView superview] == nil))
			[selfView insertSubview:newView atIndex:index];
	} else {
		[selfView addSubview: newView];
	}
	
    if (!(duration > 0))
        return;
    
	// Set up the animation
	CATransition *animation = [CATransition animation];
	[animation setDelegate:oldView]; // CAAnimation retains its delegate, so oldView will not be deleted for the duration
	
	// Set the type and if appropriate direction of the transition, 
	if (transition == kCATransitionFade) {
		[animation setType:kCATransitionFade];
	} else {
		[animation setType:transition];
		[animation setSubtype:direction];
	}
	
	// Set the duration and timing function of the transtion -- duration is passed in as a parameter, use ease in/ease out as the timing function
	[animation setDuration:duration];
	[animation setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
	
	[[selfView layer] addAnimation:animation forKey:kAnimationKey];
}

void rinseGl();

void switchScreen(Screen screen, NSString *transition, NSString *direction, float duration) {
    UIViewController *newScreen = getScreen(screen);
    
    JumpView *newBase = [[JumpView alloc] initWith: lastGl];
    
    [lastGl bury];
    rinseGl();
    [newBase startAnimation];
    
    if (newScreen) {
        [newBase addSubview: [newScreen view]];
    }
    
    replaceSubview(globalWindow, lastGl, newBase, transition, direction, newScreen?duration:0);

    [newBase release];
    lastGl = newBase;
}


@implementation GLWithEscController : UIViewController {
}
- (void) doEsc:(id)sender {
    switchScreen(escScreen, kCATransitionPush, kCATransitionFromLeft);
}
@end

// --------------------------------------------------------------------------------------------

// "Interface.cpp" utility functions

void MaybeChangeEnding(int count) {
    int temp; // Zoom out ending on anything with at least 10 levels
    ERR("Count is %d", count);
    if (count >= 10 && TIXML_SUCCESS != editing->RootElement()->QueryIntAttribute("ending", &temp)) {
        editing->RootElement()->SetAttribute("ending", 1);
        dirtyLevel = true; // Probably the case already
        ERR(" DID IT!");
    }
    ERR("\n");
}

void PushLevel() { // Does NOT save
    TiXmlElement *levelData = new TiXmlElement("Level");
    TiXmlElement *tempData;

    levelData->SetAttribute("name", "");

    tempData = new TiXmlElement("Type");
    tempData->SetAttribute("camera", "0");
    tempData->SetAttribute("rots", "255");
    tempData->SetAttribute("dontrot", "0");
    tempData->SetAttribute("rspeed", "1");
    levelData->LinkEndChild(tempData);
    tempData = new TiXmlElement("Color"); // Good colors? Maybe I should use HSL.
    tempData->SetDoubleAttribute("h", 360.0*(double(random())/RAND_MAX));
    tempData->SetAttribute("s", "0.75");
    tempData->SetAttribute("v", "1.0");
    levelData->LinkEndChild(tempData);

    wipe(editSlice);
    wipe(editSliceOld);
    editSlicePath.clear();
    editLayer = 0;

    for(int c = 0; c < 2; c++) {
        char filename[FILENAMESIZE];
        char filename2[FILENAMESIZE];
        snprintf(filename, FILENAMESIZE, "%d.png", (int)random());
        
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

    // editLevel = ceiling->Parent()->InsertBeforeChild(ceiling, *levelData); // TODO?
    editLevel = editing->RootElement()->InsertEndChild(*levelData);

    delete levelData;				
}

string nameFromLevel(TiXmlNode *_element) {
    TiXmlElement *element = (TiXmlElement *)_element;
    const char *explicitName = element->Attribute("name");
    if (explicitName && explicitName[0]) // Exists and != ""
        return explicitName;
    // TODO: # trick from debug screen... need a "which level am I?" pretty bad
    return string();
}

string nameFromTag(int tag) {
    char filename2[64];
    snprintf(filename2, 64, _("(Level %d)"), tag+1);
    return filename2;
}

string nameFromPath(int tag) {
    char filename2[64];
    snprintf(filename2, 64, _("Path %d"), tag+1);
    return filename2;
}

void rm_from_editpath(string target) {
	char filename2[FILENAMESIZE];
	snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), target.c_str());
	
	unlink(filename2); // DELETE
}

void backupEditSlice() {
    wipe(editSliceOld);
    for(int c = 0; c < editSlice.size(); c++)
        editSliceOld.push_back(editSlice[c]->clone());
}

void restoreEditSlice() {
    for(int c = 0; c < editSlice.size(); c++) {
        delete editSlice[c];
        editSlice[c] = editSliceOld[c]->clone();
        dirtySlice[c] = false;
    }
}

// --------------------------------------------------------------------------------------------

@implementation LevelBabysitter

@synthesize selectedRow;

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
	string name = level[row].name;
	if (name.empty()) {
		std:stringstream oss; // GRR
		oss << "#" << row;
		name = oss.str();
	}
	return toNs(name);
}
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
	return 1;
}
- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
	return level.size();
}

//- (CGFloat)pickerView:(UIPickerView *)pickerView rowHeightForComponent:(NSInteger)component {
//	return 30;
//}

- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component {
	return 300;
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {
	selectedRow = row;
}

@end

@implementation GLDebugController

- (IBAction)finished:(id)sender {
	ERR("FINISHED! %d, %d, %d (scores %d)", [moveChoice selectedSegmentIndex], [tiltChoice selectedSegmentIndex], levelSelect.selectedRow, (int)scores.count(currentScoreKey));

//	setControlMove(1 << [moveChoice selectedSegmentIndex]);
//	setControlRot (1 << [tiltChoice selectedSegmentIndex]);
	
	textureMode = drawSwitch.on; 
	optAxis = axisSwitch.on;
	
	if (controlSwitch.on) {
//		if (CPush == controlMove) setControlMove(CPush | CMoveAuto);
//		if (CTilt == controlRot) setControlRot(CTilt | CKnob);
	}
	
    switchScreen(NO_SCREEN);
	
	jumpman_s = levelSelect.selectedRow;
	jumpman_d = level[jumpman_s].deep+1;
	
    level[jumpman_s].tryLoad();
	jumpman_x = level[jumpman_s].entry_x; // How did I do this in the other version?
	jumpman_y = level[jumpman_s].entry_y;
	jumpman_l = level[jumpman_s].entry_l;	
	
    escScreen = PAUSE_SCREEN;
    winScreen = MAIN_SCREEN;
	pause(false);
	onEsc = WPause;
	startTimer();
	wantEsc(true);
	jumpman_reset();

//	[self autorelease]; // SAFE?
}

/*
// The designated initializer. Override to perform setup that is required before the view is loaded.
- (id)initWithNibName:(NSString *)nibNameOrnil bundle:(NSBundle *)nibBundleOrnil {
    if (self = [super initWithNibName:nibNameOrnil bundle:nibBundleOrnil]) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}
*/

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}


- (void)dealloc {
    [super dealloc];
}


@end

void wgo(bool suppressMenu = false) {
    if ((availableLevelsCount > 1 || lastLoadedFrom == FromDown || lastLoadedFrom == FromOwn) && !suppressMenu) {
        ERR("WANT LEVEL SELECT MENU\n");
        help_ok = false;
        switchScreen(WARP_SCREEN, kCATransitionPush, kCATransitionFromRight);
    } else {
        if (help_ok)
            switchScreen(HELP_SCREEN, kCATransitionPush, kCATransitionFromTop);
        else {
            switchScreen(NO_SCREEN);
        
            pause(false);
            onEsc = WPause;
            escScreen = PAUSE_SCREEN;
            startTimer();
            wantEsc(true);
            jumpman_reset();                    
        }
    }
}

static void ugly_reset() {
    if (!wantResetEverything) return;
    wload(FromInternal, "Main.jmp", true);
}

// --------------------------------------------------------------------------------------------

@implementation GLMainController

- (void) viewWillAppear:(BOOL)animated { // Awkward: This may have changed since load
    if (justWonGame) {
        l1.hidden = false;
        l2.hidden = false;
    }
}

- (IBAction)pressed:(id)sender {
	switch ([sender tag]) {
		case 1:
            ugly_reset();

            whyHelp = STARTUP_HELP;
            afterHelp = NO_SCREEN;
            help_ok = true;
            
			wgo();
            winScreen = MAIN_SCREEN;
            break;
		case 2:
            wantResetEverything = true; // Want reset-- entered editor
            whyDirectory = EDIT_DIRECTORY;
            switchScreen(DIRECTORY_SCREEN, kCATransitionPush, kCATransitionFromRight);
			break;
		case 3:
            switchScreen(OPTIONS_SCREEN, kCATransitionPush, kCATransitionFromRight);		
			break;
		case 4:
            ugly_reset();
            switchScreen(DEBUG_SCREEN, kCATransitionPush, kCATransitionFromRight);
            break;
		case 5:
			this_side_up();
			break;
        case 6: // Download
            whyDirectory = PLAY_DIRECTORY;
            switchScreen(DIRECTORY_SCREEN, kCATransitionPush, kCATransitionFromRight);
            break;
        case 7: // Clear high scores
            void LoseHighScores();
            LoseHighScores();
            dirtyScores = false;
            break;
	}
}


@end

// --------------------------------------------------------------------------------------------

void populateColorControl(UISegmentedControl *control) {
    for(int c = 0; c < 2; c++) {
        int li = c==0 ? editLayer - 1 : editLayer + 1;
        li += (editSlice.size()/2);
        li %= (editSlice.size()/2);
        
        const int width = 16;
        const int height = 16;
        std::vector<unsigned char> image;
        image.resize(width * height * 4);
        uint32_t *data = (uint32_t *)&image[0];
        for(unsigned x = 0; x < width; x++)
            for(unsigned y = 0; y < height; y++) { 
                if (0 == x || 0 == y || width-1 == x || height-1 == y) // Desirable?
                    data[y * width + x] = packColor(0,0,0);
                else
                    data[y * width + x] = packColor(level[0].r[li], level[0].g[li], level[0].b[li]);
            }
                
        std::vector<unsigned char> buffer;
        LodePNG::Encoder encoder;
        encoder.encode(buffer, image, width, height);        
        
        [control setImage:[UIImage imageWithData:[NSData dataWithBytes:&buffer[0] length:buffer.size()]] forSegmentAtIndex:c];        
    }
}

int remember_holdrot = -1;

@implementation GLEditorController

- (void)viewWillAppear:(BOOL)animated {
    remember_holdrot = holdrot;
    holdrot = 0;
}

- (void)viewDidDisappear:(BOOL)animated {
    holdrot = remember_holdrot;
}

- (void) viewDidLoad {
    {   // Bottom toolbar
        NSArray *items = toolbar.items; // SO MANY MEMORY LEAKS
        NSMutableArray *newItems = [NSMutableArray arrayWithCapacity:7];
        bool wantTool = lastChosenTool;

        if (!wantTool) {
            lastChosenTool = C_MARCH;
            lastChosenToolImage = @"kyou_spiny 1.png";
            chosenTool = C_FLOOR;
        } else
            chosenTool = lastChosenTool;

        b5 = [[UIBarButtonItem alloc] initWithImage: [UIImage imageNamed: @"nothing.png"]
                                                style:UIBarButtonItemStyleBordered 
                                               target:self action:@selector(pressed:)];
        b5.tag = 5;

        b1 = [[UIBarButtonItem alloc] initWithImage: [UIImage imageNamed: @"wall.png"]
                                              style:  wantTool?UIBarButtonItemStyleBordered:UIBarButtonItemStyleDone 
                                             target:self action:@selector(pressed:)];
        b1.tag = 1;

        b2 = [[UIBarButtonItem alloc] initWithImage: [UIImage imageNamed: lastChosenToolImage]
                                              style: !wantTool?UIBarButtonItemStyleBordered:UIBarButtonItemStyleDone 
                                             target:self action:@selector(pressed:)];
        b2.tag = 2;

        [newItems addObject: b5];
        [newItems addObject: b1];
        [newItems addObject: b2];
        [newItems addObjectsFromArray:items];
        toolbar.items = newItems;
    }
        
    { // Top toolbar
        bool thereAreLayers = editSlice.size() > 2;
        NSArray *items = topbar.items; // SO MANY MEMORY LEAKS
        NSMutableArray *newItems = [NSMutableArray arrayWithCapacity:6];
        
        NSArray *arrayOfImages = [NSArray arrayWithObjects: [UIImage imageNamed:@"downarrow.png"],
                                  [UIImage imageNamed:@"uparrow.png"], nil];
        
        UISegmentedControl *segmentedControl = [[UISegmentedControl alloc] initWithItems: arrayOfImages];
        CGRect frame = CGRectMake(0, 0, thereAreLayers?68:90, 30);
        segmentedControl.frame = frame;
        segmentedControl.momentary = YES;
        segmentedControl.segmentedControlStyle = UISegmentedControlStyleBar;
        [segmentedControl addTarget:self action:@selector(rot:) forControlEvents:UIControlEventValueChanged];
        
        [newItems addObjectsFromArray: items];

        if (editSlice.size() > 2) { // Layers
            NSArray *arrayOfImages2 = [NSArray arrayWithObjects: [NSString string], [NSString string], nil];
            UISegmentedControl *segmentedControl2 = [[UISegmentedControl alloc] initWithItems: arrayOfImages2];
            populateColorControl(segmentedControl2);
            CGRect frame = CGRectMake(0, 0, 68, 30);
            segmentedControl2.frame = frame;
            segmentedControl2.momentary = YES;
            segmentedControl2.segmentedControlStyle = UISegmentedControlStyleBar;
            [segmentedControl2 addTarget:self action:@selector(col:) forControlEvents:UIControlEventValueChanged];
            [newItems addObject: [[UIBarButtonItem alloc] initWithCustomView:segmentedControl2]];
            seg = segmentedControl2;
        }
        
        [newItems addObject: [[UIBarButtonItem alloc] initWithCustomView:segmentedControl]];
        topbar.items = newItems;
    }
}

- (IBAction)pressed:(id)sender {
	switch ([sender tag]) {
        case 5:
            chosenTool = C_NOTHING;
            b5.style = UIBarButtonItemStyleDone;
            b1.style = UIBarButtonItemStyleBordered;
            b2.style = UIBarButtonItemStyleBordered;
            break;
		case 1:
			chosenTool = C_FLOOR;
            b5.style = UIBarButtonItemStyleBordered;
            b1.style = UIBarButtonItemStyleDone;
            b2.style = UIBarButtonItemStyleBordered;            
			break;
		case 2:
			chosenTool = lastChosenTool;
            b5.style = UIBarButtonItemStyleBordered;
            b1.style = UIBarButtonItemStyleBordered;
            b2.style = UIBarButtonItemStyleDone;            
			break;
        case 3:
            switchScreen(PALETTE_SCREEN, kCATransitionPush, kCATransitionFromTop);            
            break;
		case 4:
			pause(!paused);
			if (!paused) {
				justLoadedCleanup();
				jumpman_reset();
			}
            // Too many blue things? Use an icon instead?
            pausebutton.style = paused ? UIBarButtonItemStyleDone : UIBarButtonItemStyleBordered;
			break;
        case 10:
            restoreEditSlice();

            cpVect p = chassis->p, v = chassis->v; // CHEAP CHEAP CHEAP CHEAP! dry
            
            repaintEditSlice();
            reentryEdit();
            
            chassis->p = p; chassis->v = v;
            
            break;
	}
}

- (void) doEsc:(id)sender {
    [super doEsc:sender];
    textureMode = true;
    pause(true);
    edit_mode = ENothing;
    wipe(editSliceOld); // Unnecessary but harmless
}

- (void)rot:(id)sender {
    UISegmentedControl *sc = (UISegmentedControl *)sender;
    
    NSInteger index = sc.selectedSegmentIndex;
    
    rotkey(index?ROTL:ROTR);
}

- (void)col:(id)sender {
    UISegmentedControl *sc = (UISegmentedControl *)sender;
    
    NSInteger index = sc.selectedSegmentIndex;
    
    editLayer += (index ? 1 : -1);
    editLayer += (editSlice.size()/2);
    editLayer %= (editSlice.size()/2);
    chassisShape->layers = (1 << editLayer);
    // Don't call ChassisLayerHasChanged because texture mode is off
//    layerYell(); // BEEP
    { // DRY... and generally awful
		int lc = 1 << (level[jumpman_s].layers/2);
		sbell.w = 100;
		sbell.w *= lc;
		sbell.w /= chassisShape->layers;
                if (!sbell.w) sbell.w++;
//		ERR("l %d lc %d w %d\n", a->layers, lc, sbell.w); 
		sbell.reset();
	}    
    
    populateColorControl(seg);
}

@end

// --------------------------------------------------------------------------------------------

LoadedFrom deletePackFrom = FromOwn;
string deletePackName;

@implementation GLDeletePackController

- (void)viewWillAppear:(BOOL)animated {
    if (deletePackFrom != FromOwn)
        l1.text = _(@"Really delete forever the level pack");
    string temp = "\""; temp += deletePackName; temp += _("\"?");
    l2.text = toNs(temp);
}

- (IBAction)finished:(id)sender {	
    switch ([sender tag]) {
        case 2:
        if (deletePackFrom == FromOwn || deletePackFrom == FromDown && deletePackName.size()) {
            char filename2[FILENAMESIZE];
            char filename3[FILENAMESIZE];
            switch(deletePackFrom) {
                case FromOwn: {
                    userPath(filename2);
                    snprintf(filename3, FILENAMESIZE, "%s/%s/%s.jmp", filename2, DIRPATH, deletePackName.c_str());
                } break;
                case FromDown: {
                    userPath(filename2);
                    snprintf(filename3, FILENAMESIZE, "%s/%s/%s.jmp", filename2, DOWNPATH, deletePackName.c_str());
                    break;
                }
            }    
            
            NSFileManager *fm = [NSFileManager defaultManager];
            ERR("DELETING WHOLE DIRECTORY %s\n", filename3);
            [fm removeItemAtPath:toNs(filename3) error:nil];
          
        }
        case 1:
            switchScreen(DIRECTORY_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
    }
}

@end

// --------------------------------------------------------------------------------------------

// TODO: use strings instead of const char *s?
vector<string> dircontents, downcontents;

void populateDirectory(vector<string> &contents, const char *path) {
    char filename[FILENAMESIZE];
    char filename2[FILENAMESIZE];
    userPath(filename);
    snprintf(filename2, FILENAMESIZE, "%s/%s", filename, path);
    
    printf("%s\n", filename2); // DELETE ME
    DIR *dird = opendir(filename2);
    dirent *dir;
    while (dir = readdir(dird)) {
        int len = strlen(dir->d_name);
        if (len < 4 || len >= FILENAMESIZE || strncmp( ".jmp", dir->d_name + len - 4, 4 ))
            continue;
        char * jmp = new char[len+1];
        strncpy(jmp, dir->d_name, len+1);        
        jmp[strlen(dir->d_name) - 4] = '\0';
        contents.push_back(jmp);
    }    
    closedir(dird);    
}

@implementation GLDirectoryController

- (void)viewDidAppear:(BOOL)animated {
    wantResetEverything = true; // Want reset-- entering menu. May be set redundantly later.
    clearEverything();
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1 + (dircontents.size() ? 1 : 0) + (PLAY_DIRECTORY==whyDirectory&&downcontents.size()?1:0);
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (PLAY_DIRECTORY==whyDirectory) {
        if (0==section)
            return _(@"Get more:");
        if (2==section||!dircontents.size())
            return _(@"Downloaded level packs:");
        return _(@"Your level packs:");
    } else {
        return section? _(@"Load level pack:") : _(@"Create level pack:");
    }
}

- (id)initWithStyle:(UITableViewStyle)style {
    // Just a hook so we know to initialize dircontents
    dircontents.clear(); downcontents.clear();
    
    clearEverything(); // Move to willdesplay or something, flickering is intolerable
    
    populateDirectory(dircontents, DIRPATH);
    populateDirectory(downcontents, DOWNPATH);
        
    return [super initWithStyle:style];
}

- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
    return section ? (2==section||!dircontents.size()?downcontents:dircontents).size() : 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger section = indexPath.section;
    NSUInteger row = indexPath.row;
    UITableViewCell *cell = [[UITableViewCell alloc] initWithFrame:CGRectMake(0,0,0,0) reuseIdentifier:nil];
    cell.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    
    NSString *label;
    
    if (0 == section) {
        if (PLAY_DIRECTORY == whyDirectory)
            label = _(@"Download...");
        else
            label = _(@"New...");
    } else if (2==section||!dircontents.size())
        label = toNs(downcontents[row]);
    else
        label = toNs(dircontents[row]);
    
    setText(cell, label);
    return [cell autorelease];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger row = indexPath.row; 
    NSUInteger section = indexPath.section; 
    if (PLAY_DIRECTORY == whyDirectory) {
        if (section == 0) {
            escScreen = MAIN_SCREEN;
            goWeb(DIRECTORY_WEB);
        } else if (row != NSNotFound) {
            bool downloaded = 2==section||!dircontents.size();
            string display = (downloaded?downcontents:dircontents)[row];

            wload(downloaded?FromDown:FromOwn, display.c_str(), true); // Playing, scores

            wgo();
            winScreen = MAIN_SCREEN;
        }
    } else {
        if (section == 0) {
            switchScreen(CREATE_SCREEN, kCATransitionPush, kCATransitionFromRight);
        } else if (row != NSNotFound) {
            // Save editDisplayName?
            loadEditorFile(dircontents[row].c_str());
#if 0
            deepEditLoad(editing->RootElement()->FirstChild());
            needRecenter = false;
            
            switchScreen(LEVEL_SCREEN, kCATransitionPush, kCATransitionFromRight);
#else
            switchScreen(INDEX_SCREEN, kCATransitionPush, kCATransitionFromRight);
#endif
        }        
    }
}

- (void) doEsc:(id)sender {
    switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromLeft);
}

- (void) doEdit:(id)sender {
    if (!self.editing) {
        [self setEditing:YES animated:YES];
        self.navigationItem.rightBarButtonItem.title = _(@"Done");
    } else {
        [self setEditing:NO animated:YES];
        self.navigationItem.rightBarButtonItem.title = _(@"Edit");
    }
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 0 == indexPath.section ? UITableViewCellEditingStyleNone : UITableViewCellEditingStyleDelete;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger row = indexPath.row; 
    NSUInteger section = indexPath.section; 
    bool downloaded = 2==section||!dircontents.size();
    string display = (downloaded?downcontents:dircontents)[row];

    deletePackFrom = downloaded?FromDown:FromOwn;
    deletePackName = display;
    
    switchScreen(DELETE_PACK_SCREEN, kCATransitionPush, kCATransitionFromRight);
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLCreateController

- (void)viewWillAppear:(BOOL)animated {
    [textField becomeFirstResponder];
}

- (BOOL)textFieldShouldReturn:(UITextField *)discard {
    const char *ns = toString(textField.text);
    
    if (!ns)
        return NO;
    
    string editDisplayName = ns;
    
    if (editDisplayName.empty())
        return NO;
    
    lastLevelTag = 0;
    
    char filename[FILENAMESIZE]; // duplicated from loadEditorFile. ugh
    char filename2[FILENAMESIZE];
    
    userPath(filename2); // What is this towers of hanoi nonsense
    snprintf(filename, FILENAMESIZE, "%s/%s/%s.jmp", filename2, DIRPATH, editDisplayName.c_str());
    
    mkdir(filename, 0777);
    
    snprintf(filename2, FILENAMESIZE, "%s/index.xml", filename);
    
    TiXmlDocument *newFile = new TiXmlDocument(filename2);
    TiXmlElement *root = new TiXmlElement("Jumpman");
    root->SetAttribute("Version", "1.0");
    newFile->LinkEndChild(root);
    newFile->SaveFile();
    delete newFile;
    
    loadEditorFile(editDisplayName.c_str()); // Code duplication full circle FIXME!
    
    PushLevel();
    editing->SaveFile();
    
    dirtyLevel = false; // Just in case
    
    deepEditLoad(editing->RootElement()->FirstChild()); // Further code duplication, this time from DirectoryScreen
    needRecenter = false;
    
    switchScreen(LEVEL_SCREEN, kCATransitionPush, kCATransitionFromRight);    
    
    return NO;
}

- (BOOL)textField:(UITextField *)lTextField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    for(int b = 0; b < [string length]; b++) {
        unichar c = [string characterAtIndex: b];
        if (c>127 || c=='"' || c=='/' || c=='\\' || c == '*' || c == '?' || c == '<'
            || c == '>' || c == '|' || c == ':' 
            || (c == '.' && b == 0 && 0==[[lTextField text] length])) {
            sland.reset();
            return NO;
        }
    }
    return YES;
}

- (IBAction)finished:(id)sender {
    [self textFieldShouldReturn: nil];
}


@end


// --------------------------------------------------------------------------------------------

@implementation GLPackController

- (void)viewWillAppear:(BOOL)animated {
    [nameField setText: toNs(editBaseName)];
    [summaryField setText: toNs(editing->RootElement()->Attribute("summary"))];
#if 0
    int easyMode;
    if (TIXML_SUCCESS == editing->RootElement()->QueryIntAttribute("easier", &easyMode)) {
        easyButton.checked = easyMode;
    }
#endif
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
    playButton.enabled = NO;
    editButton.enabled = NO;
}

- (void)easyMode:(id)sender {
#if 0
    if (easyButton.checked)
        editing->RootElement()->SetAttribute ("easier", 1);
    else
        editing->RootElement()->RemoveAttribute ("easier");    
    dirtyLevel = true;
#endif
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    if (textField == nameField) {
        if (!(textField.text && [textField.text length]))
            return NO;
            
        const char *ns = toString(textField.text);
        char filename[FILENAMESIZE];
        char filename2[FILENAMESIZE];
        
        userPath(filename2); // What is this towers of hanoi nonsense
        snprintf(filename, FILENAMESIZE, "%s/%s/%s.jmp", filename2, DIRPATH, ns);
        
        int errcode = rename(editingPath.c_str(), filename);
        
        if (0 != errcode) {
            ERR("Move error %d [%s] [%s]\n", errno, editingPath.c_str(), filename2);
            return NO;
        }
            
        loadEditorFile(ns);
    }
    
    if (textField == summaryField && textField.text && [textField.text length]) {
        const char *ns = toString(textField.text);
        editing->RootElement()->SetAttribute("summary", ns);
        dirtyLevel = true;
    }
    
    [textField resignFirstResponder];
    playButton.enabled = YES;
    editButton.enabled = YES;
    
    return YES; // TODO: Rename pack where appropriate
}

- (IBAction)finished:(id)sender {
	switch ([sender tag]) {
        case 1: // Playtest
            switchScreen(NO_SCREEN); // Dupe of GLMainController?
            cleanAll();
            currentScoreKey = ""; // Make sure we don't accidentally smash some high scores
            wload(FromOwn, editBaseName); // Playtest -- no scores
            wgo(availableLevelsCount <= 1); // suppress menu if nothing to display
            winScreen = PACK_SCREEN;
            break;
        case 2: // Edit
            switchScreen(INDEX_SCREEN, kCATransitionPush, kCATransitionFromRight);
            break;
        case 3: { // Upload
            // (Will "clean" later)
            if (!have_saved_login) {
                switchScreen(SIGNUP_SCREEN, kCATransitionPush, kCATransitionFromRight);
            } else {
                escScreen = PACK_SCREEN;
                goWeb(LOGIN_WEB);
            }
        } break;
        case 4: // Main menu
            cleanAll();
            switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;            
    }
}

- (BOOL)textField:(UITextField *)lTextField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    if (lTextField == summaryField)
        return YES;
    
    for(int b = 0; b < [string length]; b++) {
        unichar c = [string characterAtIndex: b];
        if (c>127 || c=='"' || c=='/' || c=='\\' || c == '*' || c == '?' || c == '<'
            || c == '>' || c == '|' || c == ':' 
            || (c == '.' && b == 0 && 0==[[lTextField text] length])) {
            sland.reset();
            return NO;
        }
    }
    return YES;
}

@end

// --------------------------------------------------------------------------------------------

#define NUMSECTIONS 3

struct Tool {
    unsigned int id;
    const char *name;
    NSString *image;
};

struct ToolSection {
    const Tool *tools;
    int count;
    NSString *title;
};

#define NUMTOOLS1 16
const Tool tools1[NUMTOOLS1] = {
    {C_MARCH,   "Spiny",    @"kyou_spiny 1.png"},
    {C_ING,     "Hunter",   @"eyes5_hunter 2.png"},
    {C_STICKY,  "Sticky",   @"kyou_sticky 1.png"},
    {C_SWOOP,   "Swoopy",   @"kyou_swoopy 2.png"},
    {C_INVIS,   "Invisible Wall",   @"invisible.png"},
    {C_ENTRY,   "Entrance", @"invisible_entry.png"},
    {C_EXIT,    "Exit",     @"exit.png"},
    {C_LAVA,    "Lava",     @"invisible_lava.png"},
    {C_BALL,    "Superball",    @"ball.png"},
    {C_BOMB,    "Bomb",     @"kyou_bomb 1.png"},
    {C_LOOSE,   "Shrapnel", @"bombable.png"},
    {C_PAINT,   "Paintbrush",   @"paintbrush.png"},
    {C_NOROT,   "No Rotation Zone", @"norot.png"},
    {C_HAPPY,   "Happy Ball",   @"ball_happy.png"},
    {C_ANGRY,   "Angry Ball",   @"ball_sad.png"},
    {C_BOMB2,   "Superbomb",    @"kyou_bomb 2.png"},
};

#define NUMTOOLS2 5
const Tool tools2[NUMTOOLS2] = {
    // Tools - allow?
    {C_ERASER,  "Eraser",  @"blank.png"},
    {C_LINE,    "Line tool",    @"drag4.png"},
    {C_SCROLL,  "Scroll tool",  @"drag2.png"},
    {C_RESIZE,  "Resize tool (even)",  @"resize.png"},
    {C_RESIZE2,  "Resize tool (odd)",  @"resize.png"},
};

#define NUMTOOLS3 5
const Tool tools3[NUMTOOLS3] = {  
    // Advanced
    {C_ARROW,   "Arrow",        @"arrow.png"},
    {C_BACK,    "Unexit",       @"exit2.png"},
    {C_REENTRY, "Checkpoint",   @"invisible_entry.png"},
    {C_LOOSE2,  "Evaporating Shrapnel", @"evaporatable.png"},
    {C_LOOSE3,  "Invisible Shrapnel",   @"invisevaporate.png"},
};

#define NUMTOOLSECTIONS 3
const ToolSection tools[NUMTOOLSECTIONS] = 
    {{tools1, NUMTOOLS1, @"Objects:"},
     {tools2, NUMTOOLS2, @"Tools:"},
     {tools3, NUMTOOLS3, @"Advanced:"}};

@implementation GLPaletteController

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return NUMTOOLSECTIONS;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return _(tools[section].title);
}

- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
    return tools[section].count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger section = indexPath.section;
    NSUInteger row = indexPath.row;
    UITableViewCell *cell = [[UITableViewCell alloc] initWithFrame:CGRectMake(0,0,0,0) reuseIdentifier:nil];
    cell.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    setText(cell, toNs(_(tools[section].tools[row].name)));
    return [cell autorelease];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger row = indexPath.row;
    NSUInteger section = indexPath.section;
    if (row != NSNotFound) {
        lastChosenTool = tools[section].tools[row].id;
        lastChosenToolImage = tools[section].tools[row].image;
        switchScreen(EDITOR_SCREEN, kCATransitionPush, kCATransitionFromBottom);
    }
}

- (void)viewDidAppear:(BOOL)animated {
    completelyHalted = true;
    drawGrid = false;
}
- (void)viewWillDisappear:(BOOL)animated {
    completelyHalted = false;
    drawGrid = true;
}

@end

// --------------------------------------------------------------------------------------------

#define BUTTONPAD 20
#define BUTTONSPAN 57
#define SCROLLHEIGHT 480

@interface XMLButton : UIButton {
    TiXmlNode *level; // How does one set this at init, anyway?
    NSString * _scores[4];
}
@property TiXmlNode *level;
@property (readonly, getter=string1) NSString **scores;
@end
@implementation XMLButton
@synthesize level;
- (NSString **) string1 { return &_scores[0]; }
- (void)dealloc { 
    for(int c = 0; c < 4; c++)
        [_scores[c] release];
    [super dealloc];
}
@end

vector<XMLButton *> indexButtons;
int selectedButton = -1; // Someday should persist?
int scrollMax = 0;
float scrollClamp(float y) {
    if (y > scrollMax-SCROLLHEIGHT) y = scrollMax-SCROLLHEIGHT;
    if (y < 0) y = 0;
    return y;
}

@implementation GLWithScrollController
@synthesize scroll; // Necessary?

- (XMLButton *) addLevelButton:(TiXmlNode *)payload at:(int)count name: (string &)name {
    UIFont *allFont = [UIFont systemFontOfSize: 15];
    
    CGRect frame = CGRectMake(20.0, 20.0+BUTTONSPAN*count, 130.0, 37.0);
    //            UIImage *buttonImage = [UIImage imageNamed:@"grayButton.png"];
    XMLButton *stopButton = [[XMLButton alloc] initWithFrame:frame];
    stopButton.level = payload;
    for(int c = 0; c < 4; c++)
        stopButton.scores[c] = nil;
    
    [stopButton setTitle:toNs(name) forState:UIControlStateNormal];
    //    [stopButton setTitleColor:[UIColor blackColor] forState:UIControlEventTouchDown];
    [stopButton setBackgroundImage:[UIImage imageNamed:@"25-75.png"] forState:UIControlStateNormal];
    [stopButton setBackgroundImage:[UIImage imageNamed:@"50-50-100-50.png"] forState:UIControlEventTouchDown];
    [stopButton setBackgroundImage:[UIImage imageNamed:@"50-50-100-50.png"] forState:UIControlStateSelected];
    stopButton.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    stopButton.contentHorizontalAlignment = UIControlContentHorizontalAlignmentCenter;
    setFont(stopButton, allFont);
    stopButton.tag = count;
    [stopButton addTarget:self action:@selector(level:) forControlEvents:UIControlEventTouchUpInside];
    //            [stopButton setBackgroundColor:[UIColor clearColor]];
    // then add the button to a UIView like this:
    [scroll addSubview: stopButton];        
    return stopButton;
}

- (void) viewDidLoad {    
    scroll.delegate = self;
    
    scrollMax = 20+BUTTONSPAN*indexButtons.size();
    scroll.contentSize = CGSizeMake(320,scrollMax); // Too many numeric constants!
}

- (void)viewWillDisappear:(BOOL)animated {
    selectedButton = -1;
    indexButtons.clear();
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
    if (selectedButton >= 0) {
        [self deselectButton];
    }
}

- (IBAction)level:(id)sender {
    XMLButton *chosen = (XMLButton *)sender;
            
    if (selectedButton >= 0)
        indexButtons[selectedButton].selected = NO;
    selectedButton = chosen.tag;
    
    indexButtons[selectedButton].selected = YES; // Coulda just used Choen // WHAT DOES THIS MEAN
}

- (void)deselectButton {
    if (selectedButton >= 0) {
        indexButtons[selectedButton].selected = NO;
        selectedButton = -1;    
    }
}

@end

@implementation GLIndexController

- (XMLButton *) addLevelButton:(TiXmlNode *)levelxml at:(int)count {
    string name = nameFromLevel(levelxml);
    if (name.empty())
        name = nameFromTag(count);
    
    return [self addLevelButton:levelxml at:count name:name];
}

- (void) viewDidLoad {    
    clearEverything();
    
    int count = 0;
        
    // Adapted from LevelPopulateContainer-- someday must duplicate?
    // TODO: Re-add image/src code
    for( TiXmlNode *levelxml = editing->RootElement()->FirstChild(); levelxml; levelxml = levelxml->NextSibling() ) {
		if (levelxml->Type() != TiXmlNode::ELEMENT || levelxml->ValueStr() != "Level") continue;

        indexButtons.push_back( [self addLevelButton: levelxml at: count] );
        
        count++;
	}
    
    [super viewDidLoad];
}

- (IBAction)pressed:(id)sender {
    int debugTag = [sender tag];
    switch(debugTag) {
        case 1: // New Level
            lastLevelTag = indexButtons.size();
            PushLevel();
            MaybeChangeEnding(indexButtons.size()+1);
            dirtyLevel = true;
            deepEditLoad(editing->RootElement()->LastChild()); // Further code duplication, this time from DirectoryScreen
            needRecenter = false;
            switchScreen(LEVEL_SCREEN, kCATransitionPush, kCATransitionFromRight);    
            break;
        case 2: // Pack Settings
            switchScreen(PACK_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
        case 3: // Back to Main Menu
            cleanAll();
            switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
            
        case 4:
            lastLevelTag = selectedButton;
            deepEditLoad(indexButtons[selectedButton].level);
            needRecenter = false;
            switchScreen(LEVEL_SCREEN, kCATransitionPush, kCATransitionFromRight);    
            break;
            
        // Basically everything that follows is copied from interface.cpp
        case 5:   // Move up
        case 6: { // Move down
            int dir = debugTag==5?-1:1;
            int otherButton = selectedButton+dir;
            if (otherButton >= indexButtons.size() || otherButton < 0)
                break;
            
            // Swap places. This requires changing four things:
            // Tags; frames; indexButtons placement; and the XML itself.
            XMLButton *temp = indexButtons[selectedButton];
            CGRect tempFrame = temp.frame;
            indexButtons[selectedButton].frame = indexButtons[otherButton].frame;
            indexButtons[otherButton].frame = tempFrame;
            indexButtons[selectedButton].tag = otherButton;
            indexButtons[otherButton].tag = selectedButton;
            indexButtons[selectedButton] = indexButtons[otherButton];
            indexButtons[otherButton] = temp;
            
            // XML:
            TiXmlNode *src = indexButtons[selectedButton].level;
            TiXmlNode *dst = indexButtons[otherButton].level;
            TiXmlNode *tmp = dst->Clone();
            indexButtons[selectedButton].level = dst->Parent()->ReplaceChild(dst, *src);
            indexButtons[otherButton].level = src->Parent()->ReplaceChild(src, *tmp);
            delete tmp;            
            dirtyLevel = true;
            
            // Adjust scroll
            CGPoint at = scroll.contentOffset;
            selectedButton = otherButton;
            at.y = scrollClamp(at.y + BUTTONSPAN*dir);
            scroll.contentOffset = at;
            
            break;
        }
        case 7: { // Duplicate
            TiXmlNode *currentLevel = indexButtons[selectedButton].level;
            TiXmlNode *newLevel = currentLevel->Clone(); // This is the duplicate we will add to the xml file
            TiXmlNode *addedLevel = NULL;
            TiXmlNode *i = NULL;
            
            const char *explicitName = ((TiXmlElement*)newLevel)->Attribute("name");
            if (explicitName && explicitName[0]) { // Exists and != ""
                char filename[FILENAMESIZE];
                snprintf(filename, FILENAMESIZE, "%s copy", explicitName);
                ((TiXmlElement*)newLevel)->SetAttribute("name", filename);
            }
            
            while (i = newLevel->IterateChildren("File", i)) { // It will be the same as the old one, but all the files are duplicates too
                if (i->Type() != TiXmlNode::ELEMENT)
                    continue;
                char filename[FILENAMESIZE];
                char filename2[FILENAMESIZE];
                char filename3[FILENAMESIZE];
                snprintf(filename2, FILENAMESIZE, "%s/%s", editingPath.c_str(), ((TiXmlElement *)i)->Attribute("src"));
                snprintf(filename, FILENAMESIZE, "%ld.png", random());
                snprintf(filename3, FILENAMESIZE, "%s/%s", editingPath.c_str(), filename);
                FILE * file2 = fopen(filename2, "r"); // 10-line cp implementation
                FILE * file3 = fopen(filename3, "w");
                if (!file2) {
                    FileBombBox(filename2); return;
                }
                if (!file3) {
                    FileBombBox(filename3); return;
                }
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
            
            addedLevel = currentLevel->Parent()->InsertAfterChild(currentLevel, *newLevel);
            sbell.reset();
            
            delete newLevel;      
            dirtyLevel = true;
            
            // Insert new button into indexButtons, displacing everything after. 
            // This is silly. Maybe I could get away with just destroying the
            // ViewController and redrawing the whole thing? I dunno.            
            indexButtons[selectedButton].selected = false;
            XMLButton *newButton = [self addLevelButton: addedLevel at: selectedButton+1];
            for(int c = selectedButton+1; c < indexButtons.size(); c++) {
                XMLButton *temp = indexButtons[c];
                temp.tag = c+1;
                CGRect at = temp.frame;
                at.origin.y += BUTTONSPAN;
                temp.frame = at;

                indexButtons[c] = newButton;
                newButton = temp;
            }
            indexButtons.push_back(newButton);
            selectedButton = -1;
            scrollMax += BUTTONSPAN;
            scroll.contentSize = CGSizeMake(320,scrollMax); // Numeric constants PLUS code duplication
            
            MaybeChangeEnding(indexButtons.size());
            
            break;
        }
        case 8: // Delete
            editLevel = indexButtons[selectedButton].level; // Do NOT deepeditload, what if it's corrupt
            switchScreen(DELETE_SCREEN, kCATransitionPush, kCATransitionFromRight);    
            break;
    }    
}

- (IBAction)level:(id)sender {
    const UIView *buttons[] = {l1,b1,b2,b3,b4,b5};
    XMLButton *chosen = (XMLButton *)sender;
    
    l1.text = [chosen titleForState: UIControlStateNormal]; // Do I like this?
    
    for(int c = 0; c < 6; c++)
        buttons[c].hidden = false;
    
    [super level:sender];
}

- (void)deselectButton {
    const UIView *buttons[] = {l1,b1,b2,b3,b4,b5};
    
    for(int c = 0; c < 6; c++)
        buttons[c].hidden = true;
    
    [super deselectButton];
}

@end

NSString *totalscores[4] = {nil,nil,nil,nil};
int didntcomplete = -1;

@implementation GLWarpController

- (XMLButton *) addLevelButtonAt:(int)incount {
    string name = nameFromPath(incount);
    if (name.empty())
        name = nameFromTag(incount);
    
    return [self addLevelButton:nil at:incount name:name];
}

- (void) viewDidLoad {        
    int incount = 0;
    char filename[FILENAMESIZE];	
    
    pair<scoreinfo,scoreinfo> &s = scores[currentScoreKey];
    bool completed = true, started = false; // NEITHER OF THESE GET USED?!
    int firsttotaltime = 0, firsttotaldeaths = 0, secondtotaltime = 0, secondtotaldeaths = 0;
        
    // Adapted from LevelPopulateContainer-- someday must duplicate?
    // TODO: Re-add image/src code
    for( int count = 0; count < level.size() && incount < availableLevelsCount; count++ ) {        
        if (0 == count || level[count].flag) { // 0==count is redundant
            XMLButton *made = [self addLevelButtonAt: incount];
            indexButtons.push_back( made );
            
            if (!availableLevelsAreFake) { // Assemble high scores -- copied from interface.cpp
                const int c = incount; // ew
                if (c < s.first.time.size() && s.first.time[c]) {
                    
                    started = true;
                    
                    firsttotaltime += s.first.time[c]; firsttotaldeaths += s.first.deaths[c];
                    int seconds = s.first.time[c] / 1000;
                    int minutes = seconds / 60;
                    seconds %= 60;
                    snprintf(filename, FILENAMESIZE, "%d:%s%d", minutes, seconds<10?"0":"", seconds);
                    made.scores[0] = [toNs(filename) retain];
                    snprintf(filename, FILENAMESIZE, _("%d lives"), s.first.deaths[c]);
                    made.scores[1] = [toNs(filename) retain];
                    
                    secondtotaltime += s.second.time[c]; secondtotaldeaths += s.second.deaths[c];
                    seconds = s.second.time[c] / 1000; // Duplicated code from WMainMenu
                    minutes = seconds / 60;
                    seconds %= 60;
                    snprintf(filename, FILENAMESIZE, "%d:%s%d", minutes, seconds<10?"0":"", seconds);
                    made.scores[2] = [toNs(filename) retain];
                    snprintf(filename, FILENAMESIZE, _("%d lives"), s.second.deaths[c]);
                    made.scores[3] = [toNs(filename) retain];
                } else {
                    completed = false; // May be because of a skip
                    if (didntcomplete < 0)
                        didntcomplete = c;
                }
            }
            
            incount++;
        }
	}
    
    if (completed) {
        int seconds = firsttotaltime / 1000;
        int minutes = seconds / 60;
        seconds %= 60;
        snprintf(filename, FILENAMESIZE, "%d:%s%d", minutes, seconds<10?"0":"", seconds);
        totalscores[0] = [toNs(filename) retain];
        snprintf(filename, FILENAMESIZE, _("%d lives"), firsttotaldeaths);
        totalscores[1] = [toNs(filename) retain];

        seconds = secondtotaltime / 1000; // Duplicated code from WMainMenu
        minutes = seconds / 60;
        seconds %= 60;
        snprintf(filename, FILENAMESIZE, "%d:%s%d", minutes, seconds<10?"0":"", seconds);
        totalscores[2] = [toNs(filename) retain];
        snprintf(filename, FILENAMESIZE, _("%d lives"), secondtotaldeaths);
        totalscores[3] = [toNs(filename) retain];
    }
    
    [super viewDidLoad];
    [self deselectButton];
}

- (void)viewDidDisappear:(BOOL)animated {
    for(int c = 0; c < 4; c++) {
        [totalscores[c] release];
        totalscores[c] = nil;
    }
    didntcomplete = -1;
}

- (IBAction)pressed:(id)sender {
    int debugTag = [sender tag];
    switch(debugTag) {
        case 1: // Start
            wgo(true);
            break;
        case 3: // Back to Main Menu
            switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
    }    
}

- (IBAction)level:(id)sender {
    XMLButton *chosen = (XMLButton *)sender;

    int flag = chosen.tag;
#if !JUMP_TO_ENDING_DEBUG
    int real_level = flags[flag];
#else
    int real_level = level.size()-1;
#endif
    int level_depth = level[real_level].deep;
    
    jumpman_s = real_level;
    jumpman_d = level_depth+1;
    jumpman_flag = level[real_level].flag-1;
    ERR("ZZZZ NEWFLAG %d\n", jumpman_flag);
    
    level[real_level].tryLoad();
    
    if (level[real_level].haveEntry) {// This must be duplicating something, somewhere.
        jumpman_x = level[real_level].entry_x;
        jumpman_y = level[real_level].entry_y;
        jumpman_l = level[real_level].entry_l;
    } else {
        jumpman_x = 0; // Am I allowed to skip this step?
        jumpman_y = 0;
    }
    chassis->p.x = jumpman_x;
    chassis->p.y = jumpman_y;
    
    const UIView *buttons[] = {b1,l1,l2,l3,l4,l5,l6,l7,l8};
    
    l1.text = [chosen titleForState: UIControlStateNormal]; // Do I like this?
    if (availableLevelsAreFake) {
        1; // Do nothing.
    } else if ([chosen.scores[0] length]) {
        l2.text = _(@"Best time");
        l3.text = chosen.scores[0];
        l4.text = chosen.scores[1];
        l5.text = _(@"Best score");
        l6.text = chosen.scores[2];
        l7.text = chosen.scores[3];
    } else {
        l2.text = _(@"(Not completed)");
        l3.text = @""; l4.text = @""; l5.text = @"";
        l6.text = @""; l7.text = @"";
    }
    
    for(int c = 0; c < 9; c++)
        buttons[c].hidden = false;
    m1.hidden = true;
    
    [super level:sender];
}

- (void)deselectButton {
    const UIView *buttons[] = {b1,l1,l2,l3,l4,l5,l6,l7,l8};
    
    for(int c = 0; c < 9; c++)
        buttons[c].hidden = true;
    
    if (lastLoadedFrom == FromDown) {
        l1.text = toNs(lastLoadedFilename);
        if (lastLoadedAuthor.size())
            m1.text = [_(@"by ") stringByAppendingString: toNs(lastLoadedAuthor)];                  
        l1.hidden = false;
        m1.hidden = false;
        l8.hidden = false;
    }
    
    if (availableLevelsAreFake) {
        l2.text = _(@"(Playtest mode,");
        l3.text = _(@"no scores)");
        l4.text = @""; l5.text = @"";
        l6.text = @""; l7.text = @"";
        l2.hidden = false;
        l3.hidden = false;
        l8.hidden = false;
    } else if (scores[currentScoreKey].first.time.size() >= flags.size()) {
        if (totalscores[0] && [totalscores[0] length]) {
            l2.text = _(@"Best time (total)");
            l3.text = totalscores[0];
            l4.text = totalscores[1];
            l5.text = _(@"Best score (total)");
            l6.text = totalscores[2];
            l7.text = totalscores[3];
            
            const UIView *buttons[] = {l2,l3,l4,l5,l6,l7,l8};
            for(int c = 0; c < 7; c++)
                buttons[c].hidden = false;
        } else {
            char filename[FILENAMESIZE];
            snprintf(filename, FILENAMESIZE, _("(Path %d not"), didntcomplete+1);
            l2.text = toNs(filename);
            l3.text = _(@"completed)");
            
            l2.hidden = false;
            l3.hidden = false;
            l8.hidden = false;            
        }
    }
    
    [super deselectButton];
}

@end

@implementation GLDeleteController

- (IBAction)finished:(id)sender {	
    switch ([sender tag]) {
        case 2: {
            TiXmlNode *i = NULL;
            
            while (i = editLevel->IterateChildren("File", i)) { // And all the files that go with it
                if (i->Type() != TiXmlNode::ELEMENT)
                    continue;
                rm_from_editpath(((TiXmlElement *)i)->Attribute("src"));
            }				
            
            editLevel->Parent()->RemoveChild(editLevel);
            
            // FIXME FIXME FIXME: Maybe just save immediately? If this happens then the program crashes
            // before a save then things could break very badly
            dirtyLevel = true;
        }
        case 1:
            switchScreen(INDEX_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
    }
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLLevelController

// Might not always be valid.
TiXmlNode *sibLeft = NULL;
TiXmlNode *sibRight = NULL;

// TODO: viewWillAppear:(BOOL)animated, viewDidLoad, viewDidDisappear:(BOOL)animated
- (void) viewDidLoad {
    { // Previous button
        TiXmlNode *sib = NULL;
        for(TiXmlNode *n = editLevel->PreviousSibling(); n && !sib; n = n->PreviousSibling()) {
            if (n->Type() == TiXmlNode::ELEMENT && n->ValueStr() == "Level")
                sib = n;
        }
        sibLeft = sib;
        levelLeft.hidden = sib?NO:YES;
    }
    
    { // Next button -- FIXME code duplication
        TiXmlNode *sib = NULL;
        for(TiXmlNode *n = editLevel->NextSibling(); n && !sib; n = editLevel->NextSibling()) {
            if (n->Type() == TiXmlNode::ELEMENT && n->ValueStr() == "Level")
                sib = n;
        }
        sibRight = sib;
        levelRight.hidden = sib?NO:YES;
    }    
}

- (void) viewWillAppear:(BOOL)animated { // Awkward: This may have changed since load
    string name = nameFromLevel(editLevel);
    if (name.empty())
        name = nameFromTag(lastLevelTag);
    levelName.text = toNs(name);
}

- (IBAction)finished:(id)sender {
    int debugTag = [sender tag];
    
    if (debugTag == 1 || debugTag == 2 || debugTag == 3) {
        if (needRecenter) {
            cleanSlices();
            deepEditLoad(editLevel);
        }
        needRecenter = true;
    }
    
    switch(debugTag) {
        case 1: // Test
            // TODO: Prevent hibernation
            clearEverything();
            currentlyLoadedPath = editingPath;
            loadLevel(editLevel, editingPath.c_str());
            rePerspective = true;
            if (!level[0].loaded)
                level[0].realForceLoad();
            justLoadedCleanup();
            currentScoreKey = ""; // Make sure we don't accidentally smash some high scores
            
            jumpman_reset();
            pause(false);
            wantEsc(true);
            wantHibernate = false;
            switchScreen(NO_SCREEN);
            break;
        case 2: // Edit
            backupEditSlice();
            edit_mode = EWalls;
			setControlMove(CEditMove, false);
			setControlRot(CEditRot, false);
			textureMode = false;
            level[jumpman_s].camera = cam_track;
            lastChosenTool = 0;
            lastChosenToolImage = @"kyou_spiny 1.png";
            switchScreen(EDITOR_SCREEN, kCATransitionPush, kCATransitionFromRight);
            
            void editorYell(const char *msg, float r, float g, float b);
            editorYell(_("Tap or drag with one finger to draw.\n\n\n\n\n\n\n\nDrag with two fingers to scroll or zoom."), 1, 1, 1);
            break;
        case 3: // Options
            switchScreen(LOPTIONS_SCREEN, kCATransitionPush, kCATransitionFromRight);
            break;
        case 6: // Index
            cleanSlices(); // Leaving this level, save it
            switchScreen(INDEX_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
        case 5: // <    next
        case 7: // >    prev
            cleanSlices(); // Leaving this level, save it
            
            lastLevelTag += (debugTag==5?-1:1);
            
            deepEditLoad(debugTag==5?sibLeft : sibRight);
            needRecenter = false;
            
            // Bottom/Top or Left/Right?
            switchScreen(LEVEL_SCREEN, kCATransitionPush, debugTag==5?kCATransitionFromBottom:kCATransitionFromTop);            
            break;
    }
}

@end

// --------------------------------------------------------------------------------------------

// Normal
@implementation GLLopt1Controller

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
    self = [super initWithNibName:nibName bundle:nibBundle];
    if (self) {
        self.title = _(@"Basic"); self.tabBarItem.image = [UIImage imageNamed:@"jumpman1 1.png"];
    }
    return self;
}

- (void)viewDidLoad {
    TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
    
    int checked;
    checked = 0; typeXml->QueryIntAttribute("camera", &checked); levelFixed.checked = checked;
    checked = 0; ((TiXmlElement*)editLevel)->QueryIntAttribute("flag", &checked); levelFlag.checked = checked;
    levelFallout.checked = NULL != typeXml->Attribute("fallout");
    
    if (TIXML_SUCCESS != typeXml->QueryIntAttribute("after", &checked)) checked = 1;
    {
        const UIButton *buttons[] = {b1,b2,b3};
        for (int c = 0; c < 3; c++) {
            buttons[c].selected = c+1 == checked; // Ugly?
        }
    }
    
    {
        const char *explicitName = ((TiXmlElement*)editLevel)->Attribute("name");
        if (explicitName && explicitName[0]) // Exists and != ""
            levelName.text = toNs(explicitName);
    }
    
    {
        TiXmlElement *messageXml = (TiXmlElement *)editLevel->IterateChildren("Message", NULL);
        if (messageXml)
            levelMessage.text = toNs(messageXml->Attribute("text"));
    }    
}

- (IBAction)segment:(id)sender
{
    const UIButton *buttons[] = {b1,b2,b3};
    
    for (int c = 0; c < 3; c++) {
        buttons[c].selected = buttons[c] == sender;
    }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

- (void)viewWillAppear:(BOOL)animated {
    completelyHalted = true;
}

- (void)viewWillDisappear:(BOOL)animated { // See SaveSetupPane
    TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);

    if (levelName.text)
        ((TiXmlElement*)editLevel)->SetAttribute("name", toString(levelName.text));
    
    if (levelMessage.text) {
        TiXmlElement *messageXml = (TiXmlElement *)editLevel->IterateChildren("Message", NULL);
		if (!messageXml) {
			messageXml = new TiXmlElement("Message");
			editLevel->LinkEndChild(messageXml);
		}
		messageXml->SetAttribute("text", toString(levelMessage.text));        
    }
    
    if (levelFlag.checked)
		((TiXmlElement*)editLevel)->SetAttribute("flag", 1);	
	else
		((TiXmlElement*)editLevel)->RemoveAttribute("flag");  
    
    if (levelFallout.checked)
        typeXml->SetAttribute ("fallout", "");
    else
        typeXml->RemoveAttribute ("fallout");
    
    typeXml->SetAttribute("camera", (int)levelFixed.checked);
    
    {
        const UIButton *buttons[] = {b1,b2,b3}; // so brittle
        for (int c = 0; c < 3; c++) {
            if (buttons[c].selected)
                typeXml->SetAttribute("after", c+1);
        }
    }    
    
    completelyHalted = false;
}

@end

// Rotation ahead:
// The rotation system in Jumpman was cobbled together experimentally with no specific idea what
// I was doing. As a result I am no longer able to really understand how, if at all, it works
// or has ever worked, and I can't replace it with something more logical without rewriting
// the whole nightmarish thing from scratch. As a result my attempts to add on to it consist of
// "find something narrow that works, set it up, don't touch it again". Maybe I'll find a good
// way of doing this if I ever make a sequel :(
inline unsigned int reprot_build(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
    return (a&REPROTALL)<<12|(b&REPROTALL)<<8|(c&REPROTALL)<<4|(d&REPROTALL)<<0;
}
// Note: Just plain breaks if you mix 1313 with anything else. Whatever
void reprot_combine(unsigned int &a, unsigned int b) {
    unsigned int d = 0;
    for(int c = 0; c < 4; c++) {
        unsigned int an = (a >> (4*c));
        unsigned int bn = (b >> (4*c));
        unsigned int cn = 
            ( ((an&REPROTREF)^(bn&REPROTREF)) & REPROTREF)
        |   ( ((an&REPROTROT)+(bn&REPROTROT)) & REPROTROT);
            
        d |= (cn << (4*c));
//        ERR("[%x:%x:%x:%x], ", an, bn, cn, (cn << (4*c)));
    }
    
//    ERR("%x • %x = %x\n", a, b, d);
    
    a = d;
}

inline void vhh(CheckButton *a) {
    a.checked = false;
    a.hidden = true;
}

// Repeat
@implementation GLLopt2Controller

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
    self = [super initWithNibName:nibName bundle:nibBundle];
    if (self) {
        self.title = _(@"Repeat"); self.tabBarItem.image = [UIImage imageNamed:@"Main.jmp/449259899.png"];
    }
    return self;
}

- (void)viewDidLoad {
    TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
    int checked;
    checked = 0; typeXml->QueryIntAttribute("repeat", &checked); levelRepeat.checked = checked;

    if (levelRepeat.checked) {
        checked = 0; typeXml->QueryIntAttribute("reprot", &checked);
//        ERR("[[[[[[%x]]]]]]\n", checked);
        if (2 == (checked & 3))  b4.checked = true;
        if (0x20 == (checked & 0x30)) b3.checked = true;
        if (4 == (checked & 4))  b6.checked = true;
        if (0x40 == (checked & 0x40)) b5.checked = true;
        if (3 == (checked & 3)) {
            b1.checked = true;
            if (0x30 == (checked & 0x30))
                b7.checked = true;
        }
        if (0x3000 == (checked & 0x3000)) {
            b2.checked = true;
            if (0x300 == (checked & 0x300))
                b7.checked = true;
        }
    }
    
    [self visibilityHandle];
}

- (void)viewWillAppear:(BOOL)animated {
    level[jumpman_s].zoom = 1; level[jumpman_s].camera = cam_track; //zdry
    scan_r = (36.0*16)/level[jumpman_s].base_width * 2.0/3.0/3;
}

- (void)viewWillDisappear:(BOOL)animated {
}

- (void)visibilityHandle {
    const CheckButton *buttons[7] = {b1,b2,b3,b4,b5,b6,b7};
    for(int c = 0; c < 7; c++)
        buttons[c].hidden = !levelRepeat.checked;

    if (b1.checked) vhh(b2);
    if (b2.checked) vhh(b1);
    if (b1.checked || b2.checked) {
        vhh(b3); vhh(b4); vhh(b5); vhh(b6);
    } else {
        vhh(b7);
    }
    if (b3.checked || b4.checked || b5.checked || b6.checked) {
        vhh(b1); vhh(b2);
    }
}

- (void)updated:(id)sender {
    TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
    
    [self visibilityHandle];
    
    typeXml->SetAttribute("repeat", (int)levelRepeat.checked);
    
    if (levelRepeat.checked) {
        unsigned int reprots = 0;
        
        if (b1.checked) reprot_combine(reprots, reprot_build(1,3,1,3));
        if (b2.checked) reprot_combine(reprots, reprot_build(3,1,3,1));
        if (b3.checked) reprot_combine(reprots, reprot_build(2,0,2,0));
        if (b4.checked) reprot_combine(reprots, reprot_build(0,2,0,2));
        if (b5.checked) reprot_combine(reprots, reprot_build(4,0,4,0));
        if (b6.checked) reprot_combine(reprots, reprot_build(0,4,0,4));
        if (b7.checked) reprot_combine(reprots, reprot_build(4,6,6,4));

        typeXml->SetAttribute("reprot", reprots);
    } else {
        typeXml->SetAttribute("reprot", 0);
    }

    ERR("\n\n");
    
    deepEditLoad(editLevel);
    
    level[jumpman_s].zoom = 1; level[jumpman_s].camera = cam_track; //zdry
    scan_r = (36.0*16)/level[jumpman_s].base_width * 2.0/3.0/3;
}

@end

UILabel *zoomFeedback = nil;
void tryZoomFeedback() {
    if (zoomFeedback) {
        char FEED[32];
#if !FOG_DEBUG
        snprintf(FEED, 32, "%.2fx", scan_r);
#else
        extern float fogf;
        snprintf(FEED, 32, "%.2fx", fogf);
#endif
        zoomFeedback.text = toNs(FEED);
    }
}

// Zoom
@implementation GLLopt3Controller

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
    self = [super initWithNibName:nibName bundle:nibBundle];
    if (self) {
        self.title = _(@"Zoom"); self.tabBarItem.image = [UIImage imageNamed:@"resize.png"];
    }
    return self;
}

- (void)viewWillAppear:(BOOL)animated {
#if !FOG_DEBUG
    TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
#endif
    
    setControlMove(CEditMove, false);
    edit_mode = EZoom;
    zoomFeedback = levelZoom;

#if !FOG_DEBUG
    level[jumpman_s].zoom = 1; // zdry
    
    int camera = 0; typeXml->QueryIntAttribute ("camera", &camera); level[jumpman_s].camera = (camera_type)camera;
    double zchecked = 1; if (TIXML_SUCCESS == typeXml->QueryDoubleAttribute("zoom", &zchecked)) {	
        scan_r = zchecked;
    } else scan_r = 1;
#endif
    tryZoomFeedback();
}

- (void)viewWillDisappear:(BOOL)animated {
#if !FOG_DEBUG
    TiXmlElement *typeXml = (TiXmlElement *)editLevel->IterateChildren("Type", NULL);
    typeXml->SetDoubleAttribute("zoom", scan_r);
#endif
    zoomFeedback = nil;
    edit_mode = ENothing;
}

@end

// Angles
@implementation GLLopt4Controller

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
    self = [super initWithNibName:nibName bundle:nibBundle];
    if (self) {
        self.title = _(@"Angles"); self.tabBarItem.image = [UIImage imageNamed:@"rot.png"];
    }
    return self;
}

- (void)viewWillAppear:(BOOL)animated {
    edit_mode = EAngle;
}

- (void)viewWillDisappear:(BOOL)animated {
    edit_mode = ENothing;
    
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

@end

// Color
@implementation GLLopt5Controller

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
    self = [super initWithNibName:nibName bundle:nibBundle];
    if (self) {
        self.title = _(@"Color"); self.tabBarItem.image = [UIImage imageNamed:@"paintbrush.png"];
    }
    hsv = NO;
    return self;
}

- (void)viewWillAppear:(BOOL)animated {
    level[jumpman_s].zoom = 1; level[jumpman_s].camera = cam_track; //zdry
    scan_r = (36.0*16)/level[jumpman_s].base_width * 2.0/3.0;
    
    if (level[0].layers > 0) {
        char name[10];
        snprintf(name, 10, _("Layer %d"), editLayer+1);
        lay1.text = toNs(name);
        populateColorControl(seg);

        lay2.hidden = YES;
        textureMode = false;
    } else {
        seg.hidden = YES;
    }

    [self loadColors];
}

- (void)viewWillDisappear:(BOOL)animated {
    //    typeXml->SetAttribute("repeat", (int)levelRepeat.checked);
    textureMode = true;
}

- (void)loadColors {
    float r = level[0].r[editLayer], g = level[0].g[editLayer], b = level[0].b[editLayer];
    if (hsv)
        RGBtoHSV(r, g, b, &r, &g, &b);
    s1.value = r;
	s2.value = g;
	s3.value = b;
}
- (void)doSetup:(id)sender {
    switch([sender tag]) {
        case 1: { // hsv
            hsv = !hsv;
            if (hsv) {
                l1.text = @"H";
                l2.text = @"S";
                l3.text = @"V";
            } else {
                l1.text = @"R";
                l2.text = @"G";
                l3.text = @"B";
            }         

            float is1 = s1.value, is2 = s2.value, is3 = s3.value;
            if (hsv) {
                RGBtoHSV(is1, is2, is3, &is1, &is2, &is3);
                if (!isnormal(is1)) is1 = 0; // Sometimes hue comes out meaningless
                s1.maximumValue = 360;
            } else {
                HSVtoRGB(&is1, &is2, &is3, is1, is2, is3);
                s1.maximumValue = 1;
            }       
            s1.value = is1; s2.value = is2; s3.value = is3;
            
            break;
        }
        case 2: {// layers;
            GLLopt6Controller *viewController6 = [[[GLLopt6Controller alloc] initWithNibName:PAD(@"Lopt6Window") bundle:nil] autorelease];
            NSMutableArray *subControllers = [NSMutableArray arrayWithArray:[lastOptions viewControllers]];
            [subControllers replaceObjectAtIndex: 3 withObject: addNav(viewController6)]; // Kludge: Always 3?
            lastOptions.viewControllers = subControllers;
            break;
        }
    }
}
- (void)doSlider:(id)sender {
    TiXmlNode *i = NULL;
    
	while(i = editLevel->IterateChildren("Color", i)) {
		if (i->Type() != TiXmlNode::ELEMENT)
			continue;
        
		TiXmlElement *colorXml = (TiXmlElement *)i;
		int temp;
		if (TIXML_SUCCESS == colorXml->QueryIntAttribute("layer", &temp) ?
			(temp != editLayer) : (0 != editLayer)) // If there's a layer, but it's not the one we're editing
			continue;
        
        float is1 = s1.value, is2 = s2.value, is3 = s3.value;

		if (hsv) {
			colorXml->SetDoubleAttribute("h", is1);
			colorXml->SetDoubleAttribute("s", is2);
			colorXml->SetDoubleAttribute("v", is3);
			colorXml->RemoveAttribute("r");
			colorXml->RemoveAttribute("g");
			colorXml->RemoveAttribute("b");
		} else {
			colorXml->RemoveAttribute("h");
			colorXml->RemoveAttribute("s");
			colorXml->RemoveAttribute("v");
			colorXml->SetDoubleAttribute("r", is1);
			colorXml->SetDoubleAttribute("g", is2);
			colorXml->SetDoubleAttribute("b", is3);
		}
        
        if (hsv) { // "Feedback"
			HSVtoRGB(&level[0].r[editLayer], &level[0].g[editLayer], &level[0].b[editLayer], is1, is2, is3);
		} else {
			level[0].r[editLayer] = is1; level[0].g[editLayer] = is2; level[0].b[editLayer] = is3;
		}
        if (textureMode)
            repaintEditSlice();
	}    
}

- (void)col:(id)sender {
    UISegmentedControl *sc = (UISegmentedControl *)sender;
    NSInteger index = sc.selectedSegmentIndex;
    editLayer += (index ? 1 : -1);
    editLayer += (editSlice.size()/2);
    editLayer %= (editSlice.size()/2);
    chassisShape->layers = (1 << editLayer);
    { // DRY... and generally awful
		int lc = 1 << (level[jumpman_s].layers/2);
		sbell.w = 100;
		sbell.w *= lc;
		sbell.w /= chassisShape->layers;
        if (!sbell.w) sbell.w++;
//        		ERR("l %d lc %d w %d\n", chassisShape->layers, lc, sbell.w); 
		sbell.reset();
	}    
    populateColorControl(sc);
    char name[10];
    snprintf(name, 10, "Layer %d", editLayer+1);
    lay1.text = toNs(name);  
    [self loadColors];
}

@end

// LAYERS PANE
@implementation GLLopt6Controller

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
    self = [super initWithNibName:nibName bundle:nibBundle];
    if (self) {
        self.title = _(@"Layers"); self.tabBarItem.image = [UIImage imageNamed:@"paintbrush.png"];
    }
    recursionLock = NO;
    return self;
}

- (IBAction)segment:(id)sender {
    const UIButton *buttons[] = {b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16};
    for (int c = 0; c < 16; c++) {
        buttons[c].selected = buttons[c] == sender;
    }    
}

- (void)viewWillAppear:(BOOL)animated {
    const UIButton *buttons[] = {b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16};
    int layers = level[jumpman_s].layers; if (!layers) layers = 1;
    for (int c = 0; c < 16; c++) {
        buttons[c].selected = c+1 == layers; // Ugly?
    }    
}

- (void)finished:(id)sender {
    if (!recursionLock) { // Possibly not necessary?
        recursionLock = YES;
        GLLopt5Controller *viewController5 = [[[GLLopt5Controller alloc] initWithNibName:PAD(@"Lopt5Window") bundle:nil] autorelease];
        NSMutableArray *subControllers = [NSMutableArray arrayWithArray:[lastOptions viewControllers]];
        [subControllers replaceObjectAtIndex: 3 withObject: addNav(viewController5)]; // Kludge: Always 3?
        lastOptions.viewControllers = subControllers;
    }
}

- (void)viewDidDisappear:(BOOL)animated {
    [self finished: nil];
}

- (void)viewWillDisappear:(BOOL)animated {
    const UIButton *buttons[] = {b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16};
    int layersCount = 1;
    int layersOld = level[0].layers ? level[0].layers : 1;
    TiXmlNode *i = NULL;
    int temp;
    for (int c = 0; c < 16; c++) {
        if (buttons[c].selected) {
            layersCount = c+1;
            break;
        }
    }    

    ERR("DELETE DELETE DELETE DELETE DELETE %d %d\n", layersOld, layersCount);
    
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
            ERR("DELETE: %p\n", deleteme.front());
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
                snprintf(filename, FILENAMESIZE, "%ld.png", random());
                
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
    
    dirtyLevel = true;
    cleanAll();
    
    deepEditLoad(editLevel);
    
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLWithSettingsController

- (int) adjustButtons:(int)row {
    int rselection = -1;
    
    { // ROW 1
        int selection = -1;
        const int count = 3;
        const UIButton *buttons[count] = {b1_1,b1_2,b1_3};
        NSString *labels[count] = {_(@"Music: Off"), _(@"Music: Soundtrack"), _(@"Music: iTunes library")};
        
        for(int c = 0; c < count; c++)
            if (buttons[c].selected) selection = c;
        if (selection>=0) l1.text = labels[selection];
        if (row == 1) rselection = selection;
    }
    
    { // ROW 2
        int selection = -1;
        const int count = 2;
        const UIButton *buttons[count] = {b2_1,b2_2};
        NSString *labels[count] = {_(@"Controls: Finger gestures"), _(@"Controls: Buttons")};
        
        for(int c = 0; c < count; c++)
            if (buttons[c].selected) selection = c;
        if (selection>=0) l2.text = labels[selection];
        if (row == 2) rselection = selection;
    }
    { // ROW 3
        int selection = 0; // NOTE: NOT -1
        const int count = 3;
        const UIButton *buttons[count] = {b3_1,b3_2,b3_3};
        NSString *labels[count] = {_(@"To rotate: Rotate Phone"), _(@"To rotate: Flick Phone"), _(@"To rotate: Finger twist")};
        
        for(int c = 0; c < count; c++) {
            if (buttons[c].selected) selection = c;
            buttons[c].hidden = buttons[c] == b3_2 || b2_2.selected; // Always hide flick button
        }
        l3.hidden = b2_2.selected;
        if (selection>=0) l3.text = labels[selection];
        if (row == 3) rselection = selection;
    }
    
    return rselection;
}

// Sound
- (IBAction) radio1:(id)sender {
    const int count = 3;
    const UIButton *buttons[count] = {b1_1,b1_2,b1_3};
    for (int c = 0; c < count; c++)
        buttons[c].selected = buttons[c] == sender;
    int selection = [self adjustButtons:1];
    
    if (selection>=0 && selection != audioMode) {
        audioMode = (AudioMode)selection;

        SetAudioCategory(audioMode);

        killMusic();
    }
    
    dirtySettings = true;
}

// Controls
- (IBAction) radio2:(id)sender {
    const int count = 2;
    const UIButton *buttons[count] = {b2_1,b2_2};
    for (int c = 0; c < count; c++)
        buttons[c].selected = buttons[c] == sender;
    int selection = [self adjustButtons:2];
    
    switch (selection) {
        case 0:
            setControlMove(CPush | CMoveAuto);
            setControlRot(b3_3.selected ? (CTilt|CKnob) : (b3_2.selected?CKnob:CGrav));
            if (!b3_3.selected && !b3_2.selected && !b3_1.selected) b3_1.selected = true; // ...Man *what*
            holdrot = 0;
            break;
        case 1:
            setControlMove(CButtonMove);
            setControlRot(CButtonRot);
            if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad)
                holdrot = 3;
            break;
    }
    controlMove = CMoveDisabled;
    controlRot = CRotDisabled;
    adjustControlRot();
    
    dirtySettings = true;
}

// Tilt
- (IBAction) radio3:(id)sender {
    const int count = 3;
    const UIButton *buttons[count] = {b3_1,b3_2,b3_3};
    for (int c = 0; c < count; c++)
        buttons[c].selected = buttons[c] == sender;
    int selection = [self adjustButtons:3];
    
    switch (selection) {
        case 0:
            setControlRot(CGrav); // REMOVE CKNOB
            break;
        case 1:
            setControlRot(CTilt | CKnob);
            break;
        case 2:
            setControlRot(CKnob);
            break;
            
    }
    controlRot = CRotDisabled;
    adjustControlRot();
    
    dirtySettings = true;
}

- (void) viewWillAppear:(BOOL)animated {
    
    if (audioMode == NO_AUDIO)
        b1_1.selected = true;
    if (audioMode == LOCAL_AUDIO)
        b1_2.selected = true;
    if (audioMode == SHUFFLE_AUDIO)
        b1_3.selected = true;
    
    if (savedControlMove == (CPush | CMoveAuto))
        b2_1.selected = true;
    if (savedControlMove == CButtonMove)
        b2_2.selected = true;
    
    if (savedControlRot & CGrav)
        b3_1.selected = true;
    else if (savedControlRot & CTilt)
        b3_2.selected = true;
    else if (savedControlRot & CKnob)
        b3_3.selected = true;
    
    [self adjustButtons:-1];
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLPauseController

- (void) viewWillAppear:(BOOL)animated {
    // Is skip allowed right now?
    skipButton.hidden = (jumpmanstate == jumpman_pan && pantype == pan_deep) || willSkipNext>=0 || skipOnRebirth || packBansSkip
        || jumpman_s+1 >= level.size();

    [super viewWillAppear:animated];
}

void unPause() {
    switchScreen(NO_SCREEN);
    if (tilt) tilt->reset();
    pause(false);
    wantEsc(true);    
}

- (IBAction)finished:(id)sender {	
	int debugTag = [sender tag];
	if (debugTag) { // REMOVE ME LATER
		switch (debugTag) {
			case 1: // Debug only
				SaveHibernate();
				break;
			case 2: // Debug only
				LoadHibernate();
				// didHibernate = false; // Suppress hib deletion. Could be useful in debugging..?
				break;
			case 3: {
                killMusic();
                currentScoreKey = ""; // Dead to me
                switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromTop);
				
				// From interface.h mmstopplaying
				pantype = pan_dont;
				jumpmanstate = jumpman_normal;
				jumpman_unpinned = true;
                if (jumpman_s < level.size()) {
                    externSpaceRemoveBody(level[jumpman_s], chassisShape);
                    cpSpaceRemoveShape(level[jumpman_s].space, chassisShape);				
                }
				
				if (didHibernate) { // ...DRY?
					LoseHibernate();
					didHibernate = false;
				}
				wantHibernate = false;		
                wantResetEverything = true; // Want reset-- quitting from pause
			} break;
            case 8: {
                switchScreen(SKIP_SCREEN, kCATransitionPush, kCATransitionFromRight);
                break;
            }
            case 10: {
                whyHelp = OPTIONS_HELP;
                afterHelp = PAUSE_SCREEN;
                switchScreen(HELP_SCREEN, kCATransitionPush, kCATransitionFromTop);
                break;
            }
		}
	} else {
        unPause();
	}
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLOptionsController

- (void) viewWillAppear:(BOOL)animated {
    // OPTCOLORBLIND BUTTON HERE
    colorblindButton.checked = optColorblind;
    splatterButton.checked = optSplatter;
    splatterButton.hidden = !haveWonGame;
    
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
    completelyHalted = true;
}
- (void)viewWillDisappear:(BOOL)animated {
    completelyHalted = false;
}

- (IBAction)finished:(id)sender {	
	int debugTag = [sender tag];
	if (debugTag) { // REMOVE ME LATER
        switch(debugTag) {
            case 10: {
                whyHelp = OPTIONS_HELP;
                afterHelp = OPTIONS_SCREEN;
                switchScreen(HELP_SCREEN, kCATransitionPush, kCATransitionFromTop);
                break;
            }
            case 11:
                escScreen = OPTIONS_SCREEN;
                goWeb(COPYRIGHT_WEB);
                break;
        }
	} else {
        // SAVE, GO TO MAIN
        if (optColorblind != colorblindButton.checked) {
            optColorblind = colorblindButton.checked;
            dirtySettings = true;
        }
        if (haveWonGame)
            optSplatter = splatterButton.checked;
        switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromLeft);
	}
}

@end

// --------------------------------------------------------------------------------------------

void doSkipNow() {
    pair<scoreinfo,scoreinfo> &s = scores[currentScoreKey]; // We need to know if we've beaten this path
    int flag = jumpman_flag+1;
    
    // Skips on paths you've already beaten are "free"
    // (though, your high score is lost to flagIsSkipPoisoned in the methods below)
    // If available levels are not fake, and this flag has not been beaten, and this isn't a reskip of some kind
    if ( !availableLevelsAreFake && ! (flag <= s.first.time.size())) { // remember flag = idx+1
        currentSkip = jumpman_s; // Then there are consequences
        
        cpVect p = cpv(jumpman_x, jumpman_y);

//      p = cpvrotate( p, level[jumpman_s].staticBody->rot); // Remove accumulated rotate (noop on track?)
        
        currentSkipX = p.x;
        currentSkipY = p.y;
    }
    
    exitedBySkipping = true;    
    if (jumpmanstate != jumpman_normal) {
        skipOnRebirth = true;
        return;
    }
    jumpmanstate = jumpman_wantexit; 
    exit_direction = 1;
}

@implementation GLSkipController

- (IBAction)finished:(id)sender {	
    switch ([sender tag]) {
        case 2: {
            pair<scoreinfo,scoreinfo> &s = scores[currentScoreKey]; // We need to know if we've beaten this path
            int flag = jumpman_flag + 1;
            ERR("UM? flag %d, size %d, currentSkip %d\n", flag, (int)s.first.time.size(), currentSkip);
            if (currentSkip<0 || availableLevelsAreFake || (flag <= s.first.time.size())) { // Free skips the second time through
                flagIsSkipPoisoned = true;
                doSkipNow();
                unPause();
            } else {
                switchScreen(SKIP2_SCREEN, kCATransitionPush, kCATransitionFromRight);
            }
            break;
        }
        case 1:
            switchScreen(PAUSE_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
    }
}

@end

// --------------------------------------------------------------------------------------------

void rotenDelta(float delta);

@implementation GLSkipController2

- (void)viewWillAppear:(BOOL)animated {
    before_jumpman_s = jumpman_s;
    before_jumpman_p = chassis->p;
    before_jumpman_entry = cpv(jumpman_x, jumpman_y);
    before_jumpman_a = roten;

    externSpaceRemoveBody(level[before_jumpman_s], chassisShape);
    cpSpaceRemoveShape(level[before_jumpman_s].space, chassisShape);				

    rotenDelta(-roten);    
    before_jumpman_l = jumpman_l;
    
    jumpman_s = currentSkip;
    jumpman_d = level[jumpman_s].deep + 1;
    
    // DRY DRY DRY!!! copied from ijumpman.cpp
    level[jumpman_s].tryLoad();
    chassis->p.x = currentSkipX;
    chassis->p.y = currentSkipY;
    
//    chassis->p = cpvunrotate( chassis->p, level[jumpman_s].staticBody->rot); // Restore accumulated rotate (noop on track or reset?)

    roten = 0;
}

- (IBAction)finished:(id)sender {	
    switch ([sender tag]) {
        case 2: {
            flagIsSkipPoisoned = true;
            
            jumpman_x = chassis->p.x; // Remember, these are just equal to currentSkipX, currentSkipY
            jumpman_y = chassis->p.y;
            
            willSkipNext = before_jumpman_s;
            willSkipNextX = before_jumpman_entry.x;
            willSkipNextY = before_jumpman_entry.y;
            
            level[before_jumpman_s].rots = level[jumpman_s].orots; // Is this redundant? In theory you can't come back to this level.
            level[before_jumpman_s].dontrot = level[jumpman_s].odontrot;            
            
            jumpman_reset();
            unPause();
            break;
        }
        case 1:
            jumpman_s = before_jumpman_s;
            chassis->p = before_jumpman_p;
            rotenDelta(before_jumpman_a);            
            jumpman_l = before_jumpman_l;
            jumpman_d = level[jumpman_s].deep + 1;
            
            externSpaceAddBody(level[jumpman_s], chassisShape);
            cpSpaceAddShape(level[jumpman_s].space, chassisShape);				
            
            switchScreen(PAUSE_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
    }
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLHelpController

- (void) viewWillAppear:(BOOL)animated {
    whyNow = whyHelp;
    
}

- (void)viewDidAppear:(BOOL)animated {
    completelyHalted = true;
}
- (void)viewWillDisappear:(BOOL)animated {
    completelyHalted = false;
}

- (IBAction)finished:(id)sender {
    Screen where = HELP_SCREEN;
    NSString *direction = kCATransitionFromRight;
    bool goAnywhere = true;
    
    switch (whyNow) {
        case MOVE_HELP:
            whyHelp = JUMP_HELP;
            break;
        case JUMP_HELP:
            if (originalWhyHelp == OPTIONS_HELP) {
                if (savedControlRot & CGrav)
                    whyHelp = ROT_GRAV_HELP;
                else
                    whyHelp = ROT_KNOB_HELP;
            } else {
                whyHelp = ESC_HELP;
            }
            break;
        case ROT_GRAV_HELP: case ROT_KNOB_HELP:
            whyHelp = ESC_HELP;
            break;
        default:
            where = afterHelp;
            direction = kCATransitionFromBottom;
            if (where == NO_SCREEN) { // I.E. if we were invoked by wgo
                help_ok = false;
                goAnywhere = false;
                wgo();
            }
            break;
    }
    
    if (goAnywhere)
        switchScreen(where, kCATransitionPush, direction);
}

@end

@implementation GLMegaHelpController

- (void) viewWillAppear:(BOOL)animated {    
    bool visibleMove = !(savedControlMove & CButtonMove);
    bool visibleRot = whyHelp == OPTIONS_HELP;
    bool visibleGrav = savedControlRot & CGrav;
    
    const int countIfVisibleMove = 13;
    const int countIfVisibleRot = 3;
    UIView *ifVisibleMove[countIfVisibleMove] = {r1, r2, r3, r4, p1, p2, p3, p4, p5, b1, b2, b3, b4};
    UIView *ifVisibleRot[countIfVisibleRot] = {t1, t2, t3};
    UIView *ifVisibleRot2[countIfVisibleRot] = {g1, g2, g3};
    
    if (visibleMove) {
        CGAffineTransform transform = {-1,0,0,1,0,0};
        r4.transform = transform;
        p1.transform = transform;
    }    
    
    for(int c = 0; c < countIfVisibleMove; c++)
        ifVisibleMove[c].hidden = !(visibleMove);
    for(int c = 0; c < countIfVisibleRot; c++)
        ifVisibleRot[c].hidden = !(visibleMove && visibleRot && visibleGrav);
    for(int c = 0; c < countIfVisibleRot; c++)
        ifVisibleRot2[c].hidden = !(visibleMove && visibleRot && !visibleGrav);
}

- (void)viewDidAppear:(BOOL)animated {
    completelyHalted = true;
}
- (void)viewWillDisappear:(BOOL)animated {
    completelyHalted = false;
}

- (IBAction)finished:(id)sender {
    Screen where = HELP_SCREEN;
    NSString *direction = kCATransitionFromRight;
    bool goAnywhere = true;

    where = afterHelp;
    direction = kCATransitionFromBottom;
    if (where == NO_SCREEN) { // I.E. if we were invoked by wgo
        help_ok = false;
        goAnywhere = false;
        wgo();
    }
    
    if (goAnywhere)
        switchScreen(where, kCATransitionPush, direction);
}

@end

// --------------------------------------------------------------------------------------------

@implementation GLSignupController

- (void)viewWillAppear:(BOOL)animated {
    if (login_name) nameField.text = login_name;
    if (login_pass) passField.text = login_pass;
    if (login_email) emailField.text = login_email;
    
    [link loadHTMLString:
     @"<html><head><style>\n<!--\nbody {margin: 0px 0px; text-align:center; background-color: black; color:white; font-family:helvetica; font-size: 17px; }\na:link { color: #FF7F00 }\na:visited { color: #FF7F00; }\na:active { color: #7F7F7F; }\n// -->\n</style></head><body><a href=\"about:tos\">These terms of service</a> apply.</span></body></html>"
                 baseURL:nil];
    
    //    [nameField setText: toNs(editBaseName)];
    //    [authorField setText: toNs(editing->RootElement()->Attribute("author"))];
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
    //    playButton.enabled = NO;
    //    editButton.enabled = NO;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    
    //    if (textField == authorField && textField.text && [textField.text length]) {
    //        const char *ns = toString(textField.text);
    //        editing->RootElement()->SetAttribute("author", ns);
    //        dirtyLevel = true;
    //    }
    
    [textField resignFirstResponder];
    //    playButton.enabled = YES;
    //    editButton.enabled = YES;
    
//  NSLog(@"n %@ p %@ e %@\n", [nameField text], [passField text], [emailField text]);
    [login_name release]; login_name = [[nameField text] retain];
    [login_pass release]; login_pass = [[passField text] retain];
    [login_email release]; login_email = [[emailField text] retain];
    
    return YES; // TODO: Rename pack where appropriate
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
    if ([[[request URL] absoluteString] hasSuffix:@"tos"]) {       
        escScreen = SIGNUP_SCREEN;
        goWeb(TOS_WEB);
        return NO;
    }
    return YES;
}

- (IBAction)finished:(id)sender {
	switch ([sender tag]) {
        case 3: 
            switchScreen(PACK_SCREEN, kCATransitionPush, kCATransitionFromLeft);
            break;
        case 4: 
            escScreen = SIGNUP_SCREEN;
            goWeb(LOGIN_WEB);
            break;
    }
}

- (BOOL)textField:(UITextField *)lTextField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    return YES;
}

@end

// --------------------------------------------------------------------------------------------

#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

int arnothing(struct archive *a, void *client_data) { 
    return (ARCHIVE_OK);
}
ssize_t arwrite(struct archive *a, void *client_data, const void *buff, size_t n) {
    NSMutableData *data = (NSMutableData *)client_data;
    [data appendBytes:buff length: n]; // TODO: Get too large and fail (10 MB is too large?)
    return n;
}

// Modeled on http://www.cocoadev.com/index.pl?HTTPFileUpload
NSData* generateFormData(NSDictionary* dict)
{
	NSString* boundary = [NSString stringWithString:@"_insert_some_boundary_here_"];
	NSArray* keys = [dict allKeys];
	NSMutableData* result = [[NSMutableData alloc] initWithCapacity:100];
    
    string basename = toString((NSString*)[dict objectForKey:@"filename"]); // TODO: not very abstract. Even necessary?
    
	int i;
	for (i = 0; i < [keys count]; i++) 
	{
		id value = [dict valueForKey: [keys objectAtIndex: i]];
		[result appendData:[[NSString stringWithFormat:@"--%@\r\n", boundary] dataUsingEncoding:NSASCIIStringEncoding]];
		if ([value isKindOfClass: [NSString class]])
		{
			[result appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"\r\n\r\n", [keys objectAtIndex:i]] dataUsingEncoding:NSASCIIStringEncoding]];
			[result appendData:[[NSString stringWithFormat:@"%@",value] dataUsingEncoding:NSASCIIStringEncoding]];
		}
		else if ([value class] == [NSURL class] && [value isFileURL])
		{
			[result appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"; filename=\"%@\"\r\n", [keys objectAtIndex:i], [[value path] lastPathComponent]] dataUsingEncoding:NSASCIIStringEncoding]];
			[result appendData:[[NSString stringWithString:@"Content-Type: application/octet-stream\r\n\r\n"] dataUsingEncoding:NSASCIIStringEncoding]];

            int re = 0;
            struct archive * ar = archive_write_new();
            re = archive_write_set_compression_gzip(ar);
//            archive_write_set_compression_none(ar);
            re = archive_write_set_format_ustar(ar);
            re = archive_write_open(ar, result, arnothing, arwrite, arnothing);
            
            char buff[8192]; int len; int fd;
            DIR *dird = opendir(toString([value path]));
            dirent *dir;
            while (dir = readdir(dird)) {
                int len2 = strlen(dir->d_name);

                if (len2 == 0 || dir->d_name[0] == '.' || dir->d_name[0] == '/') // Don't want hidden files
                    continue;
                
                string eso = toString([value path]);
                string exo = basename;
                eso += "/"; exo += "/";
                eso += dir->d_name;
                exo += dir->d_name;
                
                fd = open(eso.c_str(), O_RDONLY);
                len = read(fd, buff, sizeof(buff));
                struct stat st;
                
                stat(eso.c_str(), &st);
                
                struct archive_entry *en = archive_entry_new();
                archive_entry_copy_stat(en, &st);
                archive_entry_set_pathname(en, exo.c_str() );
                re = archive_write_header(ar, en);
                
                while ( len > 0 ) {
                    re = archive_write_data(ar, buff, len);
                    len = read(fd, buff, sizeof(buff));
                }
                archive_entry_free(en);
                close(fd);
            }
            re = archive_read_finish(ar);
		} else {
            ERR("EPIC SERIALIZE FAIL\n");
        }
		[result appendData:[[NSString stringWithString:@"\r\n"] dataUsingEncoding:NSASCIIStringEncoding]];
	}
	[result appendData:[[NSString stringWithFormat:@"--%@--\r\n", boundary] dataUsingEncoding:NSASCIIStringEncoding]];
	
//    { int fd = open("/tmp/POST.dat", O_WRONLY|O_CREAT, 0644); write(fd, [result bytes], [result length]); close(fd); }
    
	return [result autorelease];
}

@implementation GLWebController

- (void) awakeFromNib {
    dl = nil; // Must be a better way to init this
    
    [web setBackgroundColor:[UIColor blackColor]]; // Where to do this? To do this?
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    [self tick];
}

- (void) viewWillAppear:(BOOL)animated {
    [self screenWithString:_(@"Loading...")];
}

- (void) tick {
    if (stageWeb > 0) // Clumsy.
        return;
    stageWeb++;
    
    switch (whyWeb) {
        case COPYRIGHT_WEB: {
            char filename[FILENAMESIZE];
            internalPath(filename, "README.html");
            NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: [NSURL fileURLWithPath:toNs(filename)]];
            [web loadRequest: request];
        } break;                        
        case LOGIN_WEB: {
            NSString *url = JLDB @"/login"; // TODO: Need like POST parameters.
            NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: [NSURL URLWithString: url]];
            [request setHTTPMethod: @"POST"];

            NSArray *keys = [NSArray arrayWithObjects:@"username", @"password", @"email", nil]; // TODO: @filename should be .tar.gz, or at least .zmp
            NSArray *objects = [NSArray arrayWithObjects: login_name, login_pass, login_email, nil];
            NSDictionary *dict = [NSDictionary dictionaryWithObjects:objects forKeys:keys];            
            
            [request addValue: @"multipart/form-data; boundary=_insert_some_boundary_here_" forHTTPHeaderField: @"Content-Type"];
            [request setHTTPBody: generateFormData(dict)];
            
            dl = [[NSURLConnection connectionWithRequest: request delegate: self] retain];
            tarfile = [[NSMutableData data] retain];
            break;
        }
        case DIRECTORY_WEB: {
            NSString *url = JLDB @"/browse";
            NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: [NSURL URLWithString: url]];
            [web loadRequest: request];
        } break;
        case TOS_WEB: {
            NSString *url = JLDB @"/tos";
            NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: [NSURL URLWithString: url]];
            [web loadRequest: request];
        } break;            
        case UPLOAD_WEB: {
            NSString *url = JLDB @"/upload";
            NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: [NSURL URLWithString: url]];
            [request setHTTPMethod: @"POST"];
            
            //            NSInputStream* stream = [NSInputStream inputStreamWithFileAtPath: toNs(filename2)];
            
            const char *author = editing->RootElement()->Attribute("author");
            const char *shouldAuthor = [login_name UTF8String];
            if (shouldAuthor && (!author || strcmp(author, shouldAuthor))) { // If exist and unequal
                editing->RootElement()->SetAttribute("author", shouldAuthor);
                dirtyLevel = true;
            }
            
            cleanAll();
                
            // Make this go away
            const char *summary = editing->RootElement()->Attribute("summary");
            NSArray *keys = [NSArray arrayWithObjects:@"file", @"filename", @"summary", nil]; // TODO: @filename should be .tar.gz, or at least .zmp
            NSArray *objects = [NSArray arrayWithObjects:[NSURL fileURLWithPath: toNs(editingPath) isDirectory: YES], toNs(editBaseName+".jmp"), toNs(summary), nil];
            NSDictionary *dict = [NSDictionary dictionaryWithObjects:objects forKeys:keys];
            
            [request addValue: @"multipart/form-data; boundary=_insert_some_boundary_here_" forHTTPHeaderField: @"Content-Type"];
            [request setHTTPBody: generateFormData(dict)];
            //            [request setHTTPBodyStream: stream];
            
            [web loadRequest: request];    
            break;
        }
        default: {
            NSString *url = JLDB @"/";
            
            [web loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: url]]];
            break;
        }            
    }
}

- (void)viewWillDisappear:(BOOL)animated {
    if (dl) {
        [dl cancel];
        [dl release];
        [tarfile release];
    }
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error { 
    int e = [error code];
    printf("Error code %d\n", e);
    
    [self screenWithString:_(@"Could not connect to the level server (error %d)."), e];    
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
    if (whyWeb == COPYRIGHT_WEB) {
        NSString *url = [[request URL] absoluteString];
        if ([url hasPrefix:@"http"]) {
            [[UIApplication sharedApplication] openURL:[request URL]];
            return NO;
        }
    } else if (whyWeb == DIRECTORY_WEB) {
        NSString *url = [[request URL] absoluteString];
        if ([url hasSuffix:@".tgz"]) {
            [self screenWithString:_(@"Downloading...")];        // Likewise
            
            dl = [[NSURLConnection connectionWithRequest: request delegate: self] retain];
            tarfile = [[NSMutableData data] retain];
            
            return NO;
        }
    }
    return YES;
}

- (void) screenWithString:(NSString *)format,... {
    va_list argList;
    va_start(argList, format);
    NSString *s1 = [[NSString alloc] initWithFormat: format arguments: argList];
    va_end(argList);
    NSString *s2 = [[NSString alloc] initWithFormat: 
                   @"<html><body bgcolor=\"black\" text=\"white\"><span style=\"font-family:helvetica;font-size:17px\">%@</span></body></html>",
                   s1];
    [web loadHTMLString:s2 baseURL:nil];
    [s1 release];
    [s2 release];
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    switch (whyWeb) {
        case LOGIN_WEB: {
            ERR("AAAA\n");
            if ([response isKindOfClass: [NSHTTPURLResponse class]]) {
                NSHTTPURLResponse *http = (NSHTTPURLResponse *)response;
                ERR("[%d]\n", [http statusCode]);
                if ([http statusCode] == 200) {
                    [dl cancel];
                    escScreen = PACK_SCREEN;
                    whyWeb = UPLOAD_WEB;
                    stageWeb = 0;
                    
                    have_saved_login = true;
                    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
                    [prefs setObject:login_name forKey:@"login_name"];
                    [prefs setObject:login_pass forKey:@"login_pass"];
                    [prefs setObject:login_email forKey:@"login_email"];
                    
                    [self tick];                    
                } else {
                    // Do nothing
                }
            } else {
                [self screenWithString:_(@"An impossible error occurred<br><br>I don't even know")];    
            }
        }
    }
}


- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    ERR("...");
    [tarfile appendData:data];
}

- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection { 
switch(whyWeb) {
case DIRECTORY_WEB: {
    char filename[FILENAMESIZE]; // duplicated from textFieldShouldReturn. double ugh
    char filename2[FILENAMESIZE];
    
    struct archive *ar;
    struct archive_entry *en;
    string dir; bool havedir = false;
    int re = 0;
    
    userPath(filename2);
    snprintf(filename, FILENAMESIZE, "%s/%s", filename2, DOWNPATH);
        
    ar = archive_read_new();
    re = archive_read_support_compression_gzip(ar);
    re = archive_read_support_format_tar(ar);
    re = archive_read_open_memory(ar, [tarfile mutableBytes], [tarfile length]); // WAIT WHY ARE BYTES NOT CONSTANT
    while (archive_read_next_header(ar, &en) == ARCHIVE_OK) {        
        ERR("%s\n",archive_entry_pathname(en));
        string exo = archive_entry_pathname(en);

        // TODO: Super clumsy. Exploitable. Why do we even need directories
        string::size_type slash = exo.find("/"); if (slash == string::npos) continue;
        string fdir(exo, 0, slash);
        string::size_type slash2 = fdir.find(".jmp");
        if (slash2 == string::npos || slash2 != fdir.length()-4) continue;
        
        if (havedir) {
            if (fdir != dir)
                continue;
        } else {
            dir = fdir;
            havedir = true;
            string adir = filename; adir += "/"; adir += dir;
            ERR("%s\n",adir.c_str());
            re = mkdir(adir.c_str(), 0777);
        }
        
        string eso = filename; eso += "/"; eso += exo;
        ERR("%s\n", eso.c_str());
        
        int fd = open(eso.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        re = archive_read_data_into_fd(ar, fd);
        close(fd);
    } 
    re = archive_read_finish(ar); 

    string ldir(dir, 0, dir.length()-4);
#if 0
    clearEverything();
    loadEditorFile(ldir.c_str());
    wantResetEverything = true; // Want reset-- opening a downloaded pack
    switchScreen(PACK_SCREEN, kCATransitionPush, kCATransitionFromRight);
#else
    clearScoreKey(FromDown, ldir.c_str());
    wload(FromDown, ldir.c_str(), true); // Playing, scores
    
    wgo();
    winScreen = MAIN_SCREEN;    
#endif
}break;
case LOGIN_WEB: {
    [web loadHTMLString:[[[NSString alloc] initWithData:tarfile encoding:NSASCIIStringEncoding] autorelease] baseURL:nil];
}break;
}
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    [self webView:web didFailLoadWithError:error];
}

@end