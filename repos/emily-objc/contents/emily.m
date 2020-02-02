/* Emily language prototype -- classes -- implementation
	(c) Andi McClure, May 2014 */

#import "emily.h"
#import "util.h"
#import "ast.h"
#import "pseudo.h"
#import "macro.h"

#include <ctype.h>

@interface EmSession ()

@property (nonatomic, retain) NSMutableSet *knownSymbols;
@property (nonatomic, retain) NSMutableSet *separators;
@property (nonatomic, retain) NSMutableSet *rparens;
@property (nonatomic, retain) NSMutableDictionary *parens;

// Utility functions
- (NSArray *)firstTokenization:(NSString *)input;

@end

@implementation EmSession

// Paperwork: If I switch to Clang maybe I can remove these.

@synthesize separators = _separators;
@synthesize parens = _parens;
@synthesize rparens = _rparens;
@synthesize knownSymbols = _knownSymbols;
@synthesize ops = _ops;

// Implementations

- (id) init
{
    if (self = [super init]) { // LOL OBJC
		// Set up the "profile minimal" language
		self.separators = [NSMutableSet set];
		[_separators addObject:@";"];
		[_separators addObject:@"\n"];
		
		self.parens = [NSMutableDictionary dictionary];
		[_parens setObject:@")" forKey: @"("];
		[_parens setObject:@"]" forKey: @"["];
		[_parens setObject:@"}" forKey: @"{"];
	
		NSArray *rparens = [_parens allValues];
		
		self.rparens = [NSSet setWithArray:rparens];
		
		self.knownSymbols = [NSMutableSet set];
		[_knownSymbols addObjectsFromArray:[_separators allObjects]];
		[_knownSymbols addObjectsFromArray:[_parens allKeys]];
		[_knownSymbols addObjectsFromArray:rparens];
		[_knownSymbols addObject:@"^"]; // TODO: Interrogate from _ops
		
		self.ops = [NSMutableArray array];
		[_ops addObject:[[EmMacroClosure new] autorelease]];
    }
    return self;
}

- (int)consumeFile:(NSString *)filename {
	NSError *error = nil;
	NSString *contents = [[NSString alloc] initWithContentsOfFile:filename encoding:NSUTF8StringEncoding error:&error];
	
	if (!contents) {
		Err(@"Could not read from file: %@\n%@\n", filename, [error localizedDescription]);
		return 1; // Fail
	}
	
	return [self consumeString:contents source:filename takeOwnership:YES];
}

- (int)consumeString:(NSString *)contents source:(NSString *)label takeOwnership:(BOOL)ownContents {
	NSAutoreleasePool *pool = nil; DRAIN();
	int result = 1;
	
	NSArray *rawTokens = [[self firstTokenization:contents] retain];
	if (ownContents)
		[contents release];
	DRAIN();
	
	@try {
		[rawTokens autorelease];
		EmBlock *scope = [EmUserBlock newWithDitch:[[EmRootScope new] autorelease]];
		[self interpret:rawTokens withScope:scope];
		[scope release];
		result = 0;
	}
	@catch (NSException *e) {
		Err(@"Executing %@ failed:\n%@\n", label, e);
	}
	
	LASTDRAIN();
	
	return result;
}

- (EmBlock *)execute:(NSArray *)_line withScope:(EmBlock *)scope {
	NSArray *line = [_line retain];
	@try {
		for (EmMacro *macro in _ops) {
			NSArray *newline = [macro newBasedOn:line scope:scope];
			[line release];
			line = newline;
		}

		// Now that we have a final line, apply.
		EmBlock *value = nil;
		for(EmAst *ast in line) {
			EmBlock *argument = [ast evaluateWithScope:scope session:self];
#if TRACE
			Err(@"Applying %@ to %@\n", value, argument);
#endif
			value = value ? [value apply:argument session:self] : argument;
		}
		return value;
	}
	@finally {
		[line release];
	}
}

#define REARRAY() DRAIN(); line = [NSMutableArray array];
#define RPARENS(s) [NSException raise:@"EmilyException" format:@"Unexpected right parenthesis: %@", s];

- (EmBlock *)interpret:(NSArray *)tokens withScope:(EmBlock *)scope {
	NSAutoreleasePool *pool = nil;
	NSMutableArray *line; REARRAY();
	NSString *symbol = nil;
	EmBlock *last = nil;
	
	int tokenCount = [tokens count];
	
	@try {
	
		for(int c = 0;;c++) { // "Second tokenization" occurs line by line here.
			EmAst *ast = c < tokenCount ? [tokens objectAtIndex:c] : nil;
			bool astIsSymbol = [ast isKindOfClass:[EmTokenSymbol class]];
			bool foundSeparator = false;
			
			if (symbol) {
				bool matched = false;
				if (astIsSymbol) {
					EmTokenSymbol *token = (EmTokenSymbol *)ast;
					NSString *nextSymbol = [symbol stringByAppendingString: [token data] ];
					if ([_knownSymbols containsObject:nextSymbol]) {
						symbol = nextSymbol;
						matched = true;
					}
				}
				if (!matched) {
					// We have a "complete symbol" and now need to do something with it.
					// This is actually the only place this function does anything "real"
					
					// Built a symbol, but it's unrecognized.
					if (![_knownSymbols containsObject:symbol]) {
						[NSException raise:@"EmilyException" format:@"Encountered unknown symbol '%@'\n", symbol];
					}
					
					// Built a symbol and it's a separator. We can now execute a line.
					else if ([_separators containsObject:symbol]) {
						foundSeparator = true;
					}
					
					// Built a symbol and it opens a parenthesis.
					else if ([_parens objectForKey:symbol]) {
						NSMutableArray *groupTokens = [NSMutableArray array];
						NSMutableArray *parenStack = [NSMutableArray array];
						[parenStack addObject:[_parens objectForKey:symbol]];
						int d = c;
						
						// Scan to closing parenthesis.
						for(;;d++) { // Remember, we're dealing with the symbol of the *previous* token
							if (d >= tokenCount) {
								[NSException raise:@"EmilyException" format:@"Could not find matching right parenthesis for %@", symbol];
							}
							
							EmAst *innerAst = [tokens objectAtIndex:d];
							bool innerAstIsSymbol = [innerAst isKindOfClass:[EmTokenSymbol class]];
							
							if (innerAstIsSymbol) {
								EmTokenSymbol *innerToken = (EmTokenSymbol *)innerAst;
								NSString *innerSymbol = innerToken.data;

								if ([_parens objectForKey:innerSymbol]) {
									[parenStack addObject:[_parens objectForKey:innerSymbol]];
								} else if ([_rparens containsObject:innerSymbol]) {
									if ([innerSymbol isEqualToString: [parenStack lastObject]]) {
										[parenStack removeLastObject];
										if ([parenStack count] == 0) { // Success!
											break; // Success!
										}
									} else {
										RPARENS(innerSymbol);
									}
								}
							}
							
							[groupTokens addObject:innerAst];
						}
						
						EmGroup *group = [[EmGroup alloc] initWithGrouper:symbol];
						group.body.tokens = groupTokens;
						[line addObject:group];
						[group release];
						
						c = d; // Step forward overall parser
						symbol = nil;
						continue; // And halt processing on this symbol (since the group ate it)
					}
					
					else if ([_rparens containsObject:symbol]) {
						RPARENS(symbol);
					}
					
					// Built a symbol and it's just plain a symbol.
					else {
						EmTokenSymbol *astSymbol = [[EmTokenSymbol alloc] initWithString:symbol];
						[line addObject:astSymbol];
						[astSymbol release];
					}
					
					symbol = nil;
				}
			}
			
			if (foundSeparator || !ast) {
				EmBlock *result = [self execute:line withScope:scope];
				if (result) {
					EmBlock *oldLast = last;
					last = [result retain];
					[oldLast release];
				}
					
				[symbol retain];
				REARRAY();
				[symbol autorelease];
			}
			
			if (!ast)
				break;				

			if (symbol) {
				// Skip
			} else if (astIsSymbol) {
				symbol = ((EmTokenSymbol *)ast).data;
			} else {
				[line addObject:ast];
			}
		}
		
		LASTDRAIN(); // Warning: Not safe inside finally
	}
	@finally {
		[last autorelease];
	}
	
	return last;
}

typedef enum {
	TOKENING_NOTHING,
	TOKENING_SWALLOW, // Ignore this character.Doesn't matter why.
	TOKENING_HAVEDOT, // Could become atom or number
	TOKENING_ISCOMMENT,
	TOKENING_ISNEWLINE,
	TOKENING_ISNUM,
	TOKENING_ISATOM,
	TOKENING_ESCAPE,
	TOKENING_ISSTRING,
	TOKENING_ISWORD, // No IsSymbol
} TokenizeMode;

#define ADDCH(x) if (!building) building = [NSMutableString string]; [building appendString:x];
#define CH(x) [x characterAtIndex:0]
#define DONE(x) [result addObject:x]; building = nil; mode = TOKENING_NOTHING;

// Generally works, but not ideal: currently allows spaces between symbols, which I don't usually think I want.
- (NSArray *)firstTokenization:(NSString *)input {
	TokenizeMode mode = TOKENING_NOTHING;
	
	NSMutableArray *result = [NSMutableArray array];
	NSMutableString *building = nil;
	
	int inputLength = [input length];
	for(int c = 0;;c++) {
		NSString *str = c < inputLength ? [input substringWithRange:NSMakeRange(c, 1)] : nil;
		unichar ch = [str characterAtIndex:0];
		
		// Clear out effects of previous pass.
		switch(mode) {
			case TOKENING_HAVEDOT: {
				if (isalpha(ch)) {
					mode = TOKENING_ISATOM; 
				} else if (isdigit(ch)) {
					mode = TOKENING_ISNUM;
				} else if (!isblank(ch)) { // Dot was followed by... nothing I understand? Treat as symbol
					DONE( [[EmTokenSymbol alloc] initWithString:@"."] );
				}
			} break;
			case TOKENING_ISCOMMENT: {
				if (ch == '\r' || ch == '\n') {
					mode = TOKENING_NOTHING;
				}
			} break;
			case TOKENING_ISNEWLINE: {
				if (ch == '\n') {
					mode = TOKENING_SWALLOW;
				} else {
					mode = TOKENING_NOTHING;
				}
			} break;
			case TOKENING_ISNUM: {
				if (!(isalpha(ch) || isdigit(ch) || ch == '.')) {
					DONE( [[EmValueNum alloc] initWithString:building] );
				}
			} break;
			case TOKENING_ISATOM: case TOKENING_ISWORD: {
				if (!isalpha(ch)) {
					DONE( [ [(mode == TOKENING_ISATOM ? [EmValueAtom class] : [EmTokenWord class]) alloc] initWithString:building] );
				}
			} break;
			case TOKENING_ISSTRING: {
				if (ch == '"') {
					DONE( [[EmValueString alloc] initWithString:building] );
					mode = TOKENING_SWALLOW;
				}
			} break;
			default:break;
		};
		
		// Now that previous content is out of the way, check if we need to terminate.
		if (!ch)
			break;
			
		// Continue building string.
		switch(mode) {
			case TOKENING_NOTHING: {
				if (ch == '.') {
					mode = TOKENING_HAVEDOT;
				} else if (isdigit(ch) || ch == '~') {
					mode = TOKENING_ISNUM;
					ADDCH(str);
				} else if (ch == '#') {
					mode = TOKENING_ISCOMMENT;
				} else if (ch == '\r' || ch == '\n') {
					DONE( [[EmTokenSymbol alloc] initWithString:@"\n"] );
					if (ch == '\r')
						mode = TOKENING_ISNEWLINE;
				} else if (ch == '"') {
					mode = TOKENING_ISSTRING;
				} else if (isalpha(ch)) {
					mode = TOKENING_ISWORD;
					ADDCH(str);
				} else if (!isblank(ch)) { // Unknown symbol
					DONE( [[EmTokenSymbol alloc] initWithString:str] );
				}
			} break;
			case TOKENING_SWALLOW: {
				// If we get here this is something like a \n following \r or close quote, just drop it.
				mode = TOKENING_NOTHING;
			} break;
			case TOKENING_ESCAPE: {
				switch (ch) {
					case 'n': ADDCH(@"\n"); break;
					case 't': ADDCH(@"\t"); break;
					case '\\': case '"': ADDCH(str); break;
					default: [NSException raise:@"EmilyException" format:@"Unknown escape code: \\%@", str];
				}
				mode = TOKENING_ISSTRING;
			} break;
			case TOKENING_ISSTRING:
				if (ch == '\\') {
					mode = TOKENING_ESCAPE;
					break;
				}
			case TOKENING_ISNUM: case TOKENING_ISATOM: case TOKENING_ISWORD: {
				ADDCH(str);
			} break;
		}
	}
	return result;
}

@end

@implementation EmRootScope

+ (NSDictionary *)methods { // Map atoms to block "methods"
	static NSDictionary *__singleton = nil;
	if (!__singleton) {
		__singleton = [[NSDictionary alloc] initWithObjectsAndKeys:
			[EmSpecialPrint new], @"print",
			[EmSpecialTern new],  @"tern",
			[EmSpecialLoop new],  @"loop",
			[EmValueNull null],   @"null",
			nil
		];
	}
	return __singleton;
}

@end