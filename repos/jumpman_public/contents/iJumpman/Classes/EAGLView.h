//
//  EAGLView.h
//  iJumpman
//
//  Created by mcc on 3/7/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//


#import <UIKit/UIKit.h>
#import <UIKit/UIAccelerometer.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

// Kinda obnoxious but hash_map is kept in here?
#include "chipmunk.h"
#include "sound.h"

@interface AccelSlurper : NSObject <UIAccelerometerDelegate> {}
- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration;
@end

extern UIView *globalWindow;
extern BOOL transitioning;

// -------- I really don't like having this stuff in the header! -------
// In other words: When hashing UITouches, treat the pointers like ints
namespace __gnu_cxx                                                                                 
{                                                                                             
	template<> struct hash< UITouch * >                                                       
	{                                                                                           
		size_t operator()(  UITouch * const & x ) const                                           
		{                                                                                         
			return hash< unsigned int >()( (unsigned int)x );                                              
		}                                                                                         
	};                                                                                          
}          
inline cpVect cpv(CGPoint p) {
	return cpv(p.x, p.y);
}
extern int ticks;
#define TOUCHTRACK 50
#define TOUCHAVG 4
#define TOUCHTIME (1.0/3)
#define TOUCHTICKS ((int)REAL_FPS*TOUCHTIME)
#define POISONTICKS TOUCHTICKS
enum trace_dir {
	t_center = 0,
	t_left,
	t_right
};
struct touchinfo {
	unsigned int count, top;
	cpVect touches[TOUCHTRACK];
    int touchWhen[TOUCHTRACK];
	cpVect v; cpFloat a;
	trace_dir dir;
	bool isNew, isFresh; // Should these be bitfields?
	bool poisoned, editPoisoned, dragPoisoned; // "poisoned by"
    int swipePoisonedAt, pushPoisonedAt; // poison that wears off
    bool wasEscAttempt; // Like slow-acting poison I guess
	uint8_t poisonException;
    int drewAt;
	
	void push(cpVect p) {
		top++; top %= TOUCHTRACK;
		if (count < TOUCHTRACK) count++;
		touches[top] = p;
        touchWhen[top] = ticks;
	}
	inline void push(CGPoint p) {
		push(cpv(p));
	}
	
	touchinfo() { // TODO: Initializers are more efficient?
		count = 0;
		top = 0;
		v = cpvzero; a = 0;
		dir = t_center;
		isNew = true; isFresh = false;
		poisoned = false; editPoisoned = false; dragPoisoned = false; swipePoisonedAt = 0, pushPoisonedAt = 0; poisonException = 0;
        wasEscAttempt = false;
        drewAt=0;
	}
};
// -------- End unpleasantries -------

/*
This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
The view content is basically an EAGL surface you render your OpenGL scene into.
Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
*/
@interface EAGLView : UIView {
    
@protected
    /* The pixel dimensions of the backbuffer */
    GLint backingWidth;
    GLint backingHeight;    
    
    EAGLContext *context;
    NSTimer *animationTimer;
    NSTimeInterval animationInterval;

    /* OpenGL names for the renderbuffer and framebuffers used to render to this view */
    GLuint viewRenderbuffer, viewFramebuffer;
    
    /* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
    GLuint depthRenderbuffer;    
    
    bool off;
}

@property NSTimeInterval animationInterval;

- (id)initWith:(EAGLView *)source;
- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView;
- (void)bury;

@end

@interface JumpView : EAGLView {	 
    bool haveInitialized;

	AccelSlurper *slurper;
    hash_map<UITouch *, touchinfo> allTouches; 
        	
	// Taken from GLPaint, maybe should be killed:
	CGPoint				location;
	CGPoint				previousLocation;
	Boolean				firstTouch;    
}

- (void)pushAll:(NSSet *)touches;
- (void)popAll:(NSSet *)touches;
- (void)touchArrows;
- (void)drawArrows:(bool)movement;

@end
