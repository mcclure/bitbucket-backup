//
//  angelsAppDelegate.m
//  angels
//
//  Created by Andi McClure on 3/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "angelsAppDelegate.h"
#import "EAGLView.h"

@implementation angelsAppDelegate

@synthesize window;
@synthesize glView;

- (void) applicationDidFinishLaunching:(UIApplication *)application
{
	[glView startAnimation];
}

- (void) applicationWillResignActive:(UIApplication *)application
{
	[glView stopAnimation];
}

- (void) applicationDidBecomeActive:(UIApplication *)application
{
	[glView startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[glView stopAnimation];
}

- (void) dealloc
{
	[window release];
	[glView release];
	
	[super dealloc];
}

@end
