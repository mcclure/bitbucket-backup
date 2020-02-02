/* General utilities not specifically related to Emily -- implementation
	(c) Andi McClure, May 2014 */

#import "util.h"

void OutStr(NSString *value) {
    [(NSFileHandle*)[NSFileHandle fileHandleWithStandardOutput]
        writeData: [value dataUsingEncoding:NSUTF8StringEncoding]];
}

void ErrStr(NSString *value) {
    [(NSFileHandle*)[NSFileHandle fileHandleWithStandardError]
        writeData: [value dataUsingEncoding:NSUTF8StringEncoding]];
}

void Out(NSString *format, ...) {
    va_list args;
    va_start(args, format);
	NSString *formattedString = [[NSString alloc] initWithFormat: format arguments: args];
    va_end(args);
	
	OutStr(formattedString);
	
    [formattedString release];
}

void Err(NSString *format, ...) {
    va_list args;
    va_start(args, format);
	NSString *formattedString = [[NSString alloc] initWithFormat: format arguments: args];
    va_end(args);
	
	ErrStr(formattedString);
	
    [formattedString release];
}