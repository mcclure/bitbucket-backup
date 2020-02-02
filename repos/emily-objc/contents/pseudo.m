/* Emily language prototype -- pseudoblocks -- implementation
	(c) Andi McClure, May 2014 */

#import "util.h"
#import "emily.h"
#import "ast.h"
#import "pseudo.h"

@implementation EmSpecialPrint

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueStringBased class]]) {
		EmValueStringBased *obj = (EmValueStringBased *)argument;
		Out(@"%@", obj.data);
	} else if ([argument isKindOfClass:[EmValueNum class]]) {
		EmValueNum *obj = (EmValueNum *)argument;
		Out(@"%lf", obj.data);
	} else if ([argument isKindOfClass:[EmValueNull class]]) {
		Out(@"[null]");
	} else {
		return [super apply:argument session:session];
	}
	return [EmValueNull null];
}

@end

@implementation EmSpecialLoop

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	do {} while ([[argument apply:[EmValueNull null] session:session] isTrue]);

	return [EmValueNull null];
}

@end

@implementation EmCurriedNum // This might be redundant
@synthesize curry = _curry;

- initWithCurry:(EmBlock *)argument {
    if (self = [super init]) {
		EmValueNum *obj = (EmValueNum *)argument;
		self.curry = obj.data;
	}
	return self;
}

- description {
	return [NSString stringWithFormat:@"<%@ %lf>", NSStringFromClass([self class]), _curry];
}
@end

@implementation EmNumAdd

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueNum class]]) {
		EmValueNum *obj = (EmValueNum *)argument;
		EmValueNum *result = [EmValueNum new];
		result.data = self.curry + obj.data;
		return [result autorelease];
	} else {
		return [super apply:argument session:session];
	}
}

@end

@implementation EmNumSub

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueNum class]]) {
		EmValueNum *obj = (EmValueNum *)argument;
		EmValueNum *result = [EmValueNum new];
		result.data = self.curry - obj.data;
		return [result autorelease];
	} else {
		return [super apply:argument session:session];
	}
}

@end

@implementation EmNumGt

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueNum class]]) {
		EmValueNum *this = (EmValueNum *)self.curry;
		EmValueNum *obj =  (EmValueNum *)argument;
		return this.data > obj.data ? (EmBlock *)this : (EmBlock *)[EmValueNull null];
	} else {
		return [super apply:argument session:session];
	}
}

@end

@implementation EmNumLt

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueNum class]]) {
		EmValueNum *this = (EmValueNum *)self.curry;
		EmValueNum *obj =  (EmValueNum *)argument;
		return this.data < obj.data ? (EmBlock *)this : (EmBlock *)[EmValueNull null];
	} else {
		return [super apply:argument session:session];
	}
}

@end

@implementation EmNumEq

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if ([argument isKindOfClass:[EmValueNum class]]) {
		EmValueNum *this = (EmValueNum *)self.curry;
		EmValueNum *obj =  (EmValueNum *)argument;
		return this.data == obj.data ? (EmBlock *)this : (EmBlock *)[EmValueNull null];
	} else {
		return [super apply:argument session:session];
	}
}

@end

@implementation EmCurriedBlock // This might be redundant
@synthesize curry = _curry;

- initWithCurry:(EmBlock *)obj {
    if (self = [super init]) {
		self.curry = obj;
	}
	return self;
}

- description {
	return [NSString stringWithFormat:@"<%@ %@>", NSStringFromClass([self class]), _curry];
}
@end

@implementation EmSetTarget

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	return [[[EmSetTargetKey alloc] initWithTarget:self.curry key:argument] autorelease];
}

@end

@implementation EmSetTargetKey
@synthesize target = _target;
@synthesize key = _key;

- initWithTarget:(EmBlock *)target key:(EmBlock *)key {
	if (self = [super init]) {
		self.target = (EmStorageBlock *)target; // Users can't create these so we assume they work
		self.key = key;
	}
	return self;
}

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	EmPair *pair = [EmPair new];
	pair.key = _key;
	pair.value = argument;
	
	NSMutableArray *pairs = _target.pairs;
	NSInteger found = -1;
	NSInteger targetPairs = [pairs count];
	for(int c = 0; c < targetPairs; c++) {
		if ([[pairs objectAtIndex:c] match:_key session:session]) {
			found = c;
			break;
		}
	}
	if (found >= 0) { // TODO: Insert null = delete?
		[pairs replaceObjectAtIndex:found withObject:pair];
	} else {
		[pairs addObject:pair];
	}
	[pair release];
	return [EmValueNull null];
}

@end

@implementation EmSpecialTern
@synthesize condition = _target;
@synthesize a = _a;

- initWithCondition:(EmBlock *)condition a:(EmBlock *)a {
	if (self = [super init]) {
		self.condition = condition; // Users can't create these so we assume they work
		self.a = a;
	}
	return self;
}

- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session {
	if (!self.condition) {
		return [[EmSpecialTern alloc] initWithCondition:argument a:nil];
	}
	if (!self.a) {
		return [[EmSpecialTern alloc] initWithCondition:self.condition a:argument];
	}
	
	if ([[self.condition apply:[EmValueNull null] session:session] isTrue]) {
		return [self.a apply:[EmValueNull null] session:session];
	} else {
		return [argument apply:[EmValueNull null] session:session];
	}
}

@end
