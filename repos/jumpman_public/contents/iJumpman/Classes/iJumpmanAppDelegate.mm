//
//  iJumpmanAppDelegate.m
//  iJumpman
//
//  Created by mcc on 3/7/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "iJumpmanAppDelegate.h"
#import "EAGLView.h"

@implementation iJumpmanAppDelegate

@synthesize window;

void PhoneQuittingNow();
void PhoneSleepingNow();
void PhoneWakingNow();

extern JumpView *lastGl;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    globalWindow = window;
    lastGl = [[JumpView alloc] initWithFrame: [window frame]];
    [globalWindow addSubview:lastGl];
    [lastGl release];
	[lastGl startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application {
	PhoneQuittingNow();
}

- (void)applicationWillResignActive:(UIApplication *)application {
	PhoneSleepingNow();
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
//	glView.animationInterval = 1.0 / 60.0; // Why do anything?
    PhoneWakingNow();
}

- (void)dealloc {
	[window release];
//	[lastGl release];
	[super dealloc];
}

@end
