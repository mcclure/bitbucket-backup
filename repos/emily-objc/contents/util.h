/* General utilities not specifically related to Emily -- header
	(c) Andi McClure, May 2014 */

void OutStr(NSString *value);
void ErrStr(NSString *value);
void Out(NSString *format, ...);
void Err(NSString *format, ...);

// TODO: Switch to Clang, use @autorelease {}
#define LASTDRAIN() [pool drain]; pool = nil;
#define DRAIN() [pool drain]; pool = [[NSAutoreleasePool alloc] init];

#define TRACE 0