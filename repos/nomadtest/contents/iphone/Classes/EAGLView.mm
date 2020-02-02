// MAIN LOOP -- IPHONE

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2010 Andi McClure
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "kludge.h"
#import "EAGLView.h"
#include <string>
#include <sys/time.h>

#include "chipmunk.h"
#include "program.h"
#include "glCommon.h"

#define FORCE_ES1 0
#define USE_ACCELEROMETER 1

// Jumpcore state
int fullscreenw = 0, fullscreenh = 0;
int surfacew = 0, surfaceh = 0;
int ticks = 0;
double aspect = 0;
bool paused = false;
bool completelyHalted;

// in iSound.cpp
void sound_init();
void AudioHalt();
void AudioResume();

#if 1

// TODO: Move to an mutils or something

inline NSString * toNs(const std::string &s) {
    return [[[NSString alloc] initWithBytes:s.c_str() length:s.size() encoding: NSUTF8StringEncoding] autorelease];
}

inline NSString * toNs(const char *s) {
    return s?[[[NSString alloc] initWithBytes:s length:strlen(s) encoding: NSUTF8StringEncoding] autorelease]:[NSString string];
}

inline cpVect cpv(CGPoint p) {
	return cpv(p.x, p.y);
}

#endif

// Reproduce stuff naturally existing in main on Desktop version
void Quit(int) { completelyHalted = true; }
uint64_t SDL_GetTicks() { 
    timeval tp;
    gettimeofday(&tp, NULL);
    uint64_t r = (uint64_t)(tp.tv_sec)*1000 + (uint64_t)(tp.tv_usec)/1000;
    return r;
}

@implementation AccelSlurper
- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
	float accX = [acceleration x];
	float accY = [acceleration y];
	float accZ = [acceleration z];
    
	program_eventaccel(accX, accY, accZ);
}
@end

@interface JCRenderer : NSObject <ESRenderer>
{
@private
	EAGLContext *context;
	
	// The pixel dimensions of the CAEAGLLayer
	GLint backingWidth;
	GLint backingHeight;
    bool haveDoneInit;
	
	// The OpenGL names for the framebuffer and renderbuffer used to render to this view
	GLuint defaultFramebuffer, colorRenderbuffer;
}

- (void) render;
- (BOOL) resizeFromLayer:(CAEAGLLayer *)layer;

@end

@implementation EAGLView

@synthesize animating;
@dynamic animationFrameInterval;

// You must implement this method
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id) initWithCoder:(NSCoder*)coder
{    
    if ((self = [super initWithCoder:coder]))
	{
        // Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		renderer = [[JCRenderer alloc] init];
        
		animating = FALSE;
		displayLinkSupported = FALSE;
		animationFrameInterval = 1;
		displayLink = nil;
		animationTimer = nil;
		
		// A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
		// class is used as fallback when it isn't available.
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			displayLinkSupported = TRUE;
    }
	
    return self;
}

- (void) drawView:(id)sender
{
    [renderer render];
}

- (void) layoutSubviews
{
	[renderer resizeFromLayer:(CAEAGLLayer*)self.layer];
    [self drawView:nil];
}

- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!animating)
	{
		if (displayLinkSupported)
		{
			// CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
			// if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
			// not be called in system versions earlier than 3.1.

			displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[displayLink setFrameInterval:animationFrameInterval];
			[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			animationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 60.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[displayLink invalidate];
			displayLink = nil;
		}
		else
		{
			[animationTimer invalidate];
			animationTimer = nil;
		}
		
		animating = FALSE;
	}
}

- (void) dealloc
{
    [renderer release];
	
    [super dealloc];
}

- (void) awakeFromNib {
}

list<touch_rec> touchSetToList(NSSet *touches, EAGLView *self) {
    list<touch_rec> values;
    NSEnumerator *enumerator = [touches objectEnumerator];
	UITouch *value;
	while ((value = [enumerator nextObject])) {
        values.push_back( touch_rec(value, cpv([value locationInView: self]) ) );
    }        
    return values;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    program_eventtouch(touchSetToList(touches, self), touch_down);
}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    program_eventtouch(touchSetToList(touches, self), touch_move);
}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    program_eventtouch(touchSetToList(touches, self), touch_up);
}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    program_eventtouch(touchSetToList(touches, self), touch_cancel);
}

@end

@implementation JCRenderer

// Create an ES 2 or 1.1 context
- (id) init
{
	if (self = [super init])
	{
        haveDoneInit = NO;
        
#if !FORCE_ES1
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
#else
        context = NULL;
#endif
        
        if (context) {
            gl2 = true;
            ERR("USING OPENGL ES 2\n");
        } else {
            gl2 = false;
            ERR("USING OPENGL ES 1\n");
            context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        }        
        
        if (!context || ![EAGLContext setCurrentContext:context])
		{
            [self release];
            return nil;
        }
		
		// Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
		glGenFramebuffersOES(1, &defaultFramebuffer);
		glGenRenderbuffersOES(1, &colorRenderbuffer);
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer);
	}
	
	return self;
}

- (void) render
{
    if (completelyHalted) return;
    
    if (!haveDoneInit) { // Perform the following init only once. Wait until now to do it cuz
        if (!gl2)
            glPointSize(3.0);
        
        glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        gl2Basic();
        
        display_init(false);
        
        audio_init();
        sound_init();
        
        program_init();    
        
        program_interface();
		
#if USE_ACCELEROMETER
		AccelSlurper *slurper = [[AccelSlurper alloc] init];
		UIAccelerometer *acc = [UIAccelerometer sharedAccelerometer];
		[acc setUpdateInterval: 1/60.0];
		[acc setDelegate: slurper];		
#endif

        haveDoneInit = true;
    }
    
    // This application only creates a single context which is already set current at this point.
	// This call is redundant, but needed if dealing with multiple contexts.
    [EAGLContext setCurrentContext:context];
    
	// This application only creates a single default framebuffer which is already bound at this point.
	// This call is redundant, but needed if dealing with multiple framebuffers.
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
    glViewport(0, 0, backingWidth, backingHeight);

    display();
    
    // This application only creates a single color renderbuffer which is already bound at this point.
	// This call is redundant, but needed if dealing with multiple renderbuffers.
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];

    program_update();
}

- (BOOL) resizeFromLayer:(CAEAGLLayer *)layer
{	
	// Allocate color buffer backing based on the current layer size
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
    fullscreenw = backingWidth; surfacew = backingWidth;
    surfaceh = backingHeight; surfaceh = backingHeight;
    aspect = double(surfaceh)/surfacew;
    
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
    return YES;
}

- (void) dealloc
{
	// Tear down GL
	if (defaultFramebuffer)
	{
		glDeleteFramebuffersOES(1, &defaultFramebuffer);
		defaultFramebuffer = 0;
	}
    
	if (colorRenderbuffer)
	{
		glDeleteRenderbuffersOES(1, &colorRenderbuffer);
		colorRenderbuffer = 0;
	}
	
	// Tear down context
	if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	
	[context release];
	context = nil;
	
	[super dealloc];
}

@end

// Note: iJumpman BombBox was threadsafe, this isn't
void BombBox(string why) {
	NSLog(@"SUPPOSED-TO-BE-FATAL ERROR: %s\n", why.c_str());
    UIAlertView *someError = [[UIAlertView alloc] initWithTitle: @"Everything broke" message: toNs(why) delegate: nil cancelButtonTitle: nil otherButtonTitles: nil];
    [someError show];
    [someError release]; // Redundant, this will never close?
    completelyHalted = true;
}
