/* Emily language prototype -- driver
    (c) Andi McClure, May 2014 */

#import <Foundation/Foundation.h>
#import "emily.h"
#import "util.h"

int main (int argc, const char * argv[]) {
    NSAutoreleasePool *pool = nil; DRAIN();
	int result = 0;
	EmSession *session = [[EmSession alloc] init];

	NSArray *arguments = [[[NSProcessInfo processInfo] arguments] retain];
	int filecount = 0;
	DRAIN();
	
	for (NSString *filename in arguments) {
		if (filecount++ == 0) continue;
		result = [session consumeFile:filename];
		if (result) break;
	}
	[arguments release];
	
    LASTDRAIN();
    return 0;
}
