//
//  iJumpmanAppDelegate.h
//  iJumpman
//
//  Created by mcc on 3/7/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class JumpView;

@interface iJumpmanAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@end

