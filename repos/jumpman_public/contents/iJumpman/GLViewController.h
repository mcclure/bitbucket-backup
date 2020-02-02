//
//  GLViewController.h
//  iJumpman
//
//  Created by Andi McClure on 4/24/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "mutils.h"

@interface LevelBabysitter : NSObject <UIPickerViewDelegate, UIPickerViewDataSource> {
	int selectedRow;
}
@property(readonly) int selectedRow;
- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component;
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView;
- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component;
- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component;
@end

@interface GLDebugController : UIViewController {
    IBOutlet UISegmentedControl *moveChoice;
    IBOutlet UISegmentedControl *tiltChoice;
	IBOutlet LevelBabysitter *levelSelect;
    IBOutlet UISwitch *drawSwitch;
    IBOutlet UISwitch *axisSwitch;
    IBOutlet UISwitch *controlSwitch;
}

- (IBAction)finished:(id)sender;
@end

@interface GLMainController : UIViewController {	
	IBOutlet UILabel *l1, *l2;
}

- (IBAction)pressed:(id)sender;

@end

@interface GLCreateController : UIViewController {
    IBOutlet UITextField *textField;
}

- (void)viewWillAppear:(BOOL)animated;
- (BOOL)textFieldShouldReturn:(UITextField *)textField;
- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string;
- (IBAction)finished:(id)sender;
@end

@interface GLPackController : UIViewController {
    IBOutlet UITextField *nameField, *summaryField;
    IBOutlet UIButton *playButton, *editButton;
    IBOutlet CheckButton *easyButton;
}

- (void)viewWillAppear:(BOOL)animated;
- (void)textFieldDidBeginEditing:(UITextField *)textField;
- (BOOL)textFieldShouldReturn:(UITextField *)textField;
- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string;
- (IBAction)finished:(id)sender;
- (void)easyMode:(id)sender;
@end

@interface GLSignupController : UIViewController {
    IBOutlet UITextField *nameField, *passField, *emailField;
    IBOutlet UIWebView *link;
}

- (void)viewWillAppear:(BOOL)animated;
- (void)textFieldDidBeginEditing:(UITextField *)textField;
- (BOOL)textFieldShouldReturn:(UITextField *)textField;
- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string;
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (IBAction)finished:(id)sender;
@end


@interface GLWithEscController : UIViewController {
}
- (void) doEsc:(id)sender;
@end

@interface GLEditorController : GLWithEscController {	
    IBOutlet UIToolbar *toolbar, *topbar;
    IBOutlet UIBarButtonItem *pausebutton;
    UIBarButtonItem *b5, *b1, *b2;
    UISegmentedControl *seg;
}

- (IBAction)pressed:(id)sender;
- (void) doEsc:(id)sender;
- (void)rot:(id)sender;
- (void)col:(id)sender;

@end

@interface GLWithScrollController : UIViewController <UIScrollViewDelegate> {
    IBOutlet UIScrollView *scroll;
}
@property(readonly) UIScrollView *scroll; // Necessary?
- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView;
- (IBAction)level:(id)sender;
- (void)viewWillDisappear:(BOOL)animated;
- (IBAction)deselectButton;
@end

@interface GLIndexController : GLWithScrollController {
    IBOutlet UILabel *l1;
    IBOutlet UIButton *b1, *b2, *b3, *b4, *b5;
}
- (IBAction)pressed:(id)sender;
- (IBAction)level:(id)sender;
@end

@interface GLWarpController : GLWithScrollController {
    IBOutlet UILabel *l1, *l2, *l3, *l4, *l5, *l6, *l7, *l8, *m1; // m > l
    IBOutlet UIButton *b1;
}
- (IBAction)pressed:(id)sender;
- (IBAction)level:(id)sender;
@end

@interface GLSkipController : UIViewController {
}
- (IBAction)finished:(id)sender;
@end

@interface GLSkipController2 : UIViewController {
    int before_jumpman_s;
    cpVect before_jumpman_p, before_jumpman_entry;
    cpFloat before_jumpman_a;
    cpFloat before_jumpman_l;
}
- (IBAction)finished:(id)sender;
@end

@interface GLDeleteController : UIViewController {
}
- (IBAction)finished:(id)sender;
@end

@interface GLDeletePackController : UIViewController {
    IBOutlet UILabel *l1, *l2;
}
- (IBAction)finished:(id)sender;
@end

@interface GLDirectoryController : UITableViewController {
    
}

- (void)viewDidAppear:(BOOL)animated;
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView;
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section;
- (id)initWithStyle:(UITableViewStyle)style;
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section;
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath;
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath;
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath;
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath;

- (void) doEsc:(id)sender;
- (void) doEdit:(id)sender;

@end

@interface GLPaletteController : UITableViewController {
    
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView;
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section;
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section;
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath;
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath;
- (void)viewDidAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;

@end

// Normal
@interface GLLopt1Controller : GLWithEscController {
    IBOutlet UITextField *levelName, *levelMessage;
    IBOutlet CheckButton *levelFixed, *levelFlag, *levelFallout;
    
    IBOutlet UIButton *b1, *b2, *b3;
}

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (void)viewDidLoad;
- (IBAction)segment:(id)sender;
- (BOOL)textFieldShouldReturn:(UITextField *)textField;
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;

@end

// Repeat
@interface GLLopt2Controller : GLWithEscController {
    IBOutlet CheckButton *levelRepeat,
        *b1, *b2, *b3, *b4, *b5, *b6, *b7;
}

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (void)viewDidLoad;
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;
- (void)updated:(id)sender;
- (void)visibilityHandle;

@end

// Zoom
void tryZoomFeedback();
@interface GLLopt3Controller : GLWithEscController {
    IBOutlet UILabel *levelZoom;
}

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;

@end

// Angles
@interface GLLopt4Controller : GLWithEscController {
}

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;

@end

// Color
@interface GLLopt5Controller : GLWithEscController {
    IBOutlet UISlider *s1, *s2, *s3;
    IBOutlet UILabel *l1, *l2, *l3;

    IBOutlet UILabel *lay1, *lay2;
    IBOutlet UISegmentedControl *seg;

    BOOL hsv;
}

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;
- (void)loadColors;
- (void)doSetup:(id)sender;
- (void)doSlider:(id)sender;
- (void)col:(id)sender;

@end

// Layers
@interface GLLopt6Controller : GLWithEscController {
    IBOutlet UIButton *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8, *b9, *b10, *b11, *b12, *b13, *b14, *b15, *b16;
    BOOL recursionLock;
}

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (IBAction)segment:(id)sender;
- (IBAction)finished:(id)sender;
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;
- (void)viewDidDisappear:(BOOL)animated;

@end

@interface GLLevelController : UIViewController {
    IBOutlet UILabel *levelName;
    IBOutlet UIButton *levelLeft, *levelRight;
}

- (void) viewDidLoad;
- (void) viewWillAppear:(BOOL)animated;
- (IBAction)finished:(id)sender;
@end

@interface GLWithSettingsController : UIViewController {
    IBOutlet UILabel *l1, *l2, *l3;
    IBOutlet UIButton *b1_1, *b1_2, *b1_3, *b2_1, *b2_2, *b3_1, *b3_2, *b3_3;
}

- (int) adjustButtons:(int) row;
- (void) viewWillAppear:(BOOL)animated;
- (IBAction) radio1:(id)sender;
- (IBAction) radio2:(id)sender;
- (IBAction) radio3:(id)sender;
@end

@interface GLPauseController : GLWithSettingsController {
    IBOutlet UIButton *skipButton;
}

- (void) viewWillAppear:(BOOL)animated;
- (IBAction)finished:(id)sender;
@end

@interface GLOptionsController : GLWithSettingsController {
    IBOutlet CheckButton *colorblindButton, *hintButton, *splatterButton;
}

- (void)viewWillAppear:(BOOL)animated;
- (IBAction)finished:(id)sender;

@end

enum HelpTarget {
    NO_HELP = 0,
    STARTUP_HELP,
    OPTIONS_HELP,
    
    STARTUP_BASE =100,
    MOVE_HELP,
    JUMP_HELP,
    ESC_HELP,
    BUTTON_ESC_HELP,
    ROT_GRAV_HELP,
    ROT_KNOB_HELP
};

@interface GLHelpController : UIViewController {
    HelpTarget whyNow;
}

- (void) viewWillAppear:(BOOL)animated;
- (IBAction)finished:(id)sender;

@end

@interface GLMegaHelpController : UIViewController {
    IBOutlet UIView
        *r1, *r2, *r3, *r4,         // Red -- #4 is png
        *p1, *p2, *p3, *p4, *p5,    // Purple-- #1 is png
        *b1, *b2, *b3, *b4,         // Blue-- #1 is png
        *t1, *t2, *t3,              // Turn (colorless)-- #1 is png
        *g1, *g2, *g3;              // Green -- #1 is png
}

- (IBAction)finished:(id)sender;

@end


@interface GLWebController : GLWithEscController <UIWebViewDelegate> {
    IBOutlet UIWebView *web;
    
    NSURLConnection *dl;
    NSMutableData *tarfile;
}

- (void)tick;

- (void)awakeFromNib; // Redundant?
- (void)viewWillAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (void) screenWithString:(NSString *)format,...;

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response;
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data;
- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite;
- (void)connectionDidFinishLoading:(NSURLConnection *)connection;
- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error;

@end

// Switching mechanics

enum Screen {
    NO_SCREEN = 0,
    DEBUG_SCREEN,
    MAIN_SCREEN,
    OPTIONS_SCREEN,
    PAUSE_SCREEN,
    SKIP_SCREEN,
    SKIP2_SCREEN,
    
    HELP_SCREEN,
    WARP_SCREEN,

    CREATE_SCREEN,
    DIRECTORY_SCREEN,
    PACK_SCREEN,
    INDEX_SCREEN,
    LEVEL_SCREEN,
    EDITOR_SCREEN,
    PALETTE_SCREEN,
    LOPTIONS_SCREEN,
    DELETE_SCREEN,
    DELETE_PACK_SCREEN,
    
    SIGNUP_SCREEN,
    WEB_SCREEN,
    
    AFTER_ENDING_MAIN_SCREEN
};

extern Screen escScreen, winScreen;

enum WebTarget {
    NO_WEB,
    COPYRIGHT_WEB,
    TOS_WEB,
    LOGIN_WEB,
    UPLOAD_WEB,
    DIRECTORY_WEB
};

enum DirectoryTarget {
    NO_DIRECTORY,
    EDIT_DIRECTORY,
    PLAY_DIRECTORY
};

// Transition/direction may only be null when switching to NO_SCREEN
void switchScreen(Screen screen, NSString *transition = nil, NSString *direction = nil, float duration = 0.75);

extern NSString *login_name, *login_pass, *login_email;
extern bool have_saved_login;