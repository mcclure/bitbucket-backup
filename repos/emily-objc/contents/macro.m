/* Emily language prototype -- macros -- implementation
	(c) Andi McClure, May 2014 */

#import "util.h"
#import "emily.h"
#import "ast.h"
#import "macro.h"

@implementation EmMacro

- (NSArray *)newBasedOn:(NSArray *)input scope:(EmBlock *)scope {
	return [input retain];
}

@end

@implementation EmMacroClosure

- (NSArray *)newBasedOn:(NSArray *)input scope:(EmBlock *)scope {
	NSMutableArray *result = [NSMutableArray new];
	bool caret = false;
	NSString *bindingName = nil;
	for (EmAst *ast in input) {
		if (caret) {
			if ([ast isKindOfClass:[EmGroup class]]) {
				EmClosure *closure = [EmClosure new];
				closure.argument = bindingName;
				closure.group = (EmGroup *)ast;
				closure.scope = scope;
				[result addObject:closure];
				// FIXME: This release on the next line is necessary-- there is an overrelease somewhere else this compensates for
				//[closure release];
				bindingName = nil;
				caret = false;
			} else if ([ast isKindOfClass:[EmTokenWord class]]) {
				if (bindingName)
					[NSException raise:@"EmilyException" format:@"Duplicate binding %@ on closure", self];
					
				bindingName = ((EmTokenWord *)ast).data;
			} else {
				[NSException raise:@"EmilyException" format:@"Unexpectedly found %@ after ^", self];
			}
		} else {
			if ([ast isKindOfClass:[EmTokenSymbol class]] && [((EmTokenSymbol *)ast).data isEqualToString:@"^"]) {
				caret = true;
			} else {
				[result addObject:ast]; // Pass through
			}
		}
	}
	return result;
}

@end