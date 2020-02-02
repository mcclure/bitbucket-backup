//
//  angelsAppDelegate.m
//  angels
//
//  Created by Andi McClure on 3/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "Jumpcore-iphoneAppDelegate.h"
#import "EAGLView.h"

void program_sleep();
void program_wake();

@implementation angelsAppDelegate

@synthesize window;
@synthesize glView;

- (void) applicationDidFinishLaunching:(UIApplication *)application
{
	[glView startAnimation];
}

- (void) applicationWillResignActive:(UIApplication *)application
{
	program_sleep();
	[glView stopAnimation];
}

- (void) applicationDidBecomeActive:(UIApplication *)application
{
	program_wake();
	[glView startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	program_sleep();
	[glView stopAnimation];
}

- (void) dealloc
{
	[window release];
	[glView release];
	
	[super dealloc];
}

@end
