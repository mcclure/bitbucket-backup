/* Emily language prototype -- syntax tree and values -- implementation
	(c) Andi McClure, May 2014 */

#import "util.h"
#import "emily.h"
#import "ast.h"
#import "pseudo.h"

EmValueAtom *atom_set() {
	static EmValueAtom *__atom = NULL;
	if (!__atom) {
		__atom = [[EmValueAtom alloc] initWithString:@"set"];
	}
	return __atom;
}

EmValueAtom *atom_this() {
	static EmValueAtom *__atom = NULL;
	if (!__atom) {
		__atom = [[EmValueAtom alloc] initWithString:@"this"];
	}
	return __atom;
}

@implementation EmTokenList
@synthesize tokens = _tokens; // Of EmTokens
@end

@implementation EmAst
- (EmBlock *)evaluateWithScope:(EmBlock *)scope session:(EmSession *)session {
	[NSException raise:@"EmilyException" format:@"Internal error: %@ somehow escaped the parse stage", self];
	return nil;
}
@end

@implementation EmToken
@synthesize data = _data;

- initWithString:(NSString *)data {
    if (self = [super init]) {
		self.data = data;
	}
	return self;
}

- description {
	return [NSString stringWithFormat:@"<%@ '%@'>", NSStringFromClass([self class]), _data];
}

@end

@implementation EmTokenSymbol
@end

@implementation EmTokenWord
- (EmBlock *)evaluateWithScope:(EmBlock *)scope session:(EmSession *)session {
	EmValueAtom *atom = [[EmValueAtom alloc] initWithString:self.data];
	@try {
#if TRACE
		Err(@"Evaluate %@\n", atom);
#endif
		return [scope apply:atom session:session];
	}
	@finally {
		[atom release];
	}
}
@end

// Raw values

@implementation EmBlock

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	[NSException raise:@"EmilyException" format:@"Failed application: %@ can't handle argument %@", self, argument];
	return nil;
}

// Nothing to evaluate. You are already a block. Be proud.
- (EmBlock *)evaluateWithScope:(EmBlock *)scope session:(EmSession *)session {
	return self;
}

- (BOOL)isTrue {
	return YES;
}

@end

@implementation EmPseudoBlock // A interpreter construct approximating a block

+ (NSDictionary *)methods { // Map atoms to block "methods"
	return nil;
}

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueAtom class]]) {
		EmValueAtom *atom = (EmValueAtom *)argument;
		EmBlock *method = [[[self class] methods] objectForKey:atom.data];
		if ([method isKindOfClass:[EmBlock class]]) {
			return method;
		} else if (method) { // Just assume it's a class
			Class methodClass = (Class)method;
			return [[methodClass alloc] initWithCurry:self];
		}
	}
	return [super apply:argument session:session];
}
@end

// TODO: All EmValue subclasses need isEqual: implementations

@implementation EmValue
@end

@implementation EmValueNull

+ (EmValueNull *)null {
	static EmValueNull *__singleton = nil;
	if (!__singleton) {
		__singleton = [[EmValueNull alloc] init];
	}
	return __singleton;
}

- (BOOL)isTrue {
	return NO;
}

- (BOOL)isEqual:(id)other { // Assume null is a singleton
	return other == self;
}

@end

@implementation EmValueStringBased
@synthesize data = _data;

- initWithString:(NSString *)data {
    if (self = [super init]) {
		self.data = data;
	}
	return self;
}

- description {
	return [NSString stringWithFormat:@"<%@ '%@'>", NSStringFromClass([self class]), _data];
}

- (BOOL)isEqual:(id)other { // Assume null is a singleton
	if ([other isKindOfClass:[EmValueStringBased class]]) {
		EmValueStringBased *stringOther = (EmValueStringBased *)other;
		return [self.data isEqualToString:stringOther.data];
	}
	return NO;
}

@end

@implementation EmValueString
@end

@implementation EmValueAtom
@end

#define ADDCH(ch) [building appendFormat:@"%C",ch];

@implementation EmValueNum : EmValue
@synthesize data = _data;

// Ugh
- initWithString:(NSString *)data {
    if (self = [super init]) {
		bool haveDot = false;
		NSMutableString *building = [NSMutableString string];
		int datalength = [data length];
		for(int c = 0; c < datalength; c++) {
			unichar ch = [data characterAtIndex:c];
			if (ch == '~') {
				ADDCH(ch);
			} else if (ch == '.') {
				if (!haveDot) {
					ADDCH(ch);
					haveDot = true;
				}
			} else {
				ADDCH(ch);
			}
		}
		self.data = [building doubleValue];
	}
	return self;
}

- description {
	return [NSString stringWithFormat:@"<%@ %lf>", NSStringFromClass([self class]), _data];
}

- (BOOL)isEqual:(id)other { // Assume null is a singleton
	if ([other isKindOfClass:[EmValueNum class]]) {
		EmValueNum *numOther = (EmValueNum *)other;
		return self.data == numOther.data;
	}
	return NO;
}

+ (NSDictionary *)methods { // Map atoms to block "methods"
	static NSDictionary *__singleton = nil;
	if (!__singleton) {
		__singleton = [[NSDictionary alloc] initWithObjectsAndKeys:
			[EmNumAdd class], @"plus",
			[EmNumSub class], @"minus",
			[EmNumGt class],  @"gt",
			[EmNumLt class],  @"lt",
			[EmNumEq class],  @"eq",
			nil
		];
	}
	return __singleton;
}

@end

@implementation EmGroup
@synthesize grouper = _grouper;
@synthesize body = _body;

- initWithGrouper:(NSString *)grouper {
    if (self = [super init]) {
		self.grouper = grouper;
		self.body = [EmTokenList new];
		[_body release];
	}
	return self;
}

- description {
	return [NSString stringWithFormat:@"<%@ '%@' (%d tokens)>", NSStringFromClass([self class]), _grouper, (int)_body.tokens.count];
}

// The primary actual use of "evaluate", 
- (EmBlock *)evaluateWithScope:(EmBlock *)scope session:(EmSession *)session {
		// TODO: Deal with ^, [], {} specials
	EmStorageBlock *newScope = nil;
	EmBlock *forceReturn = nil;
	@try {
		if ([self.grouper isEqualToString: @"{"]) {
			newScope = [EmUserBlock newWithDitch:scope];
			scope = newScope;
		} else if ([self.grouper isEqualToString: @"["]) {
			EmBlock *object = [[EmUserBlock new] autorelease];
			newScope = [EmStorageBlock newWithDitch:scope];
			[newScope setAtom:atom_this() value: object];
			[newScope setAtom:atom_set()  value: [object apply:atom_set() session:session]];
			scope = newScope;
			forceReturn = object;
		}
	
		EmBlock *result = [session interpret:self.body.tokens withScope:scope];
		
		return forceReturn ? forceReturn : result;
	}
	@finally {
		[newScope release];
	}
}

@end

// Blocks

@implementation EmStorageBlock
@synthesize pairs = _pairs;
@synthesize ditch = _ditch;

- init {
    if (self = [super init]) {
		_pairs = [[NSMutableArray alloc] init];
	}
	return self;
}

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	// Pass 2: k/v check (Pass 1 is in EmUserBlock)
	NSMutableArray *pairs = _pairs;
	NSInteger pairCount = [pairs count];
	for(int c = 0; c < pairCount; c++) {
		EmPair *pair = [pairs objectAtIndex:c];
		if ([pair match:argument session:session]) {
			return [pair apply:argument session:session];
		}
	}
	
	// Pass 3: Fall through to prototype
	if (_ditch) {
#if TRACE
		Err(@"Falling through to %@\n", _ditch);
#endif
		return [_ditch apply:argument session:session];
	}
	
	return [super apply:argument session:session]; // Throw error
}

- (EmBlock *)setAtom:(EmValueAtom *)atom value:(EmBlock *)value {
	EmPair *pair = [EmPair new];
	pair.key = atom;
	pair.value = value;
	[self.pairs addObject:pair];
	[pair release];
	return [EmValueNull null];
}

- (EmBlock *)setAtomString:(NSString *)str value:(EmBlock *)value {
	EmValueAtom *atom = [EmValueAtom new];
	atom.data = str;
	@try {
		return [self setAtom:atom value:value];
	}
	@finally {
		[atom release];
	}
}

+ (EmStorageBlock *)newWithDitch:(EmBlock *)ditch {
	EmStorageBlock *block = [[self class] new];
	block.ditch = ditch;
	return block;
}

+ (EmBlock *)newWithBinding:(NSString *)binding value:(EmBlock *)value scope:(EmBlock *)scope {
	if (!binding) {
		return [scope retain];
	}
	EmStorageBlock *block = [EmStorageBlock newWithDitch:scope];
	[block setAtomString:binding value: value];
	return block;
}

@end

@implementation EmUserBlock

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	// Pass 1: Special "set" behavior
	if ([atom_set() isEqual:argument]) {
		return [[[EmSetTarget alloc] initWithCurry:self] autorelease];
	}

	return [super apply:argument session:session]; // k/v
}

@end


@implementation EmClosure
@synthesize scope = _scope;
@synthesize argument = _argument;
@synthesize group = _group;

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	EmBlock *modifiedScope = [EmStorageBlock newWithBinding:_argument value:argument scope:_scope];
	@try {
		return [_group evaluateWithScope:modifiedScope session:session];
	}
	@finally {
		[modifiedScope release];
	}
}

@end

// At most one of "pattern" and "key" may be non-nil.
// Exactly one of "value" and "invoke" may be non-nil.
// Non-nil value + non-nil invoke is nonsense.

@implementation EmPair
@synthesize pattern = _pattern;
@synthesize key = _key;
@synthesize value = _value;
@synthesize invoke = _invoke;

- (BOOL)match:(EmBlock *)key session:(EmSession *)session {
	if (_key)
		return [_key isEqual:key];
	else
		return [[_pattern apply:key session:session] isTrue];
}

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if (_value)
		return _value;
	return [_invoke apply:argument session:session];
}
@end
