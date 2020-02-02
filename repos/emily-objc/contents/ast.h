/* Emily language prototype -- syntax tree and values -- header
	(c) Andi McClure, May 2014 */

// AST/intermediate structures

@interface EmTokenList : NSObject
@property (nonatomic, retain) NSArray *tokens; // Of EmTokens
@end

@class EmBlock;
@class EmSession;

@interface EmAst : NSObject // Desirable?
- (EmBlock *)evaluateWithScope:(EmBlock *)scope session:(EmSession *)session;
@end

@interface EmToken : EmAst
@property (nonatomic, retain) NSString *data;
- initWithString:(NSString *)data;
@end

@interface EmTokenSymbol : EmToken
@end

@interface EmTokenWord : EmToken
@end

@interface EmGroup : EmAst
@property (nonatomic, retain) NSString *grouper;
@property (nonatomic, retain) EmTokenList *body;
- initWithGrouper:(NSString *)data;
@end

// Raw values

@interface EmBlock : EmAst // A thing fitting the interaction requirements for a block.
- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session;
- (BOOL)isTrue;
@end

@interface EmPseudoBlock : EmBlock // A interpreter construct approximating a block
+ (NSDictionary *)methods; // Map atoms to block "methods"
@end

@interface EmValue : EmPseudoBlock
@end

@interface EmValueNull : EmValue
+ (EmValueNull *)null;
@end

@interface EmValueStringBased : EmValue
@property (nonatomic, retain) NSString *data;
- initWithString:(NSString *)data;
@end

@interface EmValueString : EmValueStringBased
@end

@interface EmValueAtom : EmValueStringBased
@end

@interface EmValueNum : EmValue
@property (nonatomic, assign) double data;
- initWithString:(NSString *)data;
@end

// Blocks

@interface EmStorageBlock : EmBlock // A thing the user perceives as a block
@property (nonatomic, retain) NSMutableArray *pairs;
@property (nonatomic, retain) EmBlock *ditch;
+ (EmStorageBlock *)newWithDitch:(EmBlock *)ditch;
- (EmBlock *)setAtom:(EmValueAtom *)atom value:(EmBlock *)value;
- (EmBlock *)setAtomString:(NSString *)str value:(EmBlock *)value;
+ (EmBlock *)newWithBinding:(NSString *)binding value:(EmBlock *)value scope:(EmBlock *)scope;
@end

@interface EmUserBlock : EmStorageBlock
@end

@interface EmClosure : EmBlock
@property (nonatomic, retain) EmBlock *scope;
@property (nonatomic, retain) NSString *argument;
@property (nonatomic, retain) EmGroup *group;
- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session;
@end

// At most one of "pattern" and "key" may be non-nil.
// Exactly one of "value" and "invoke" may be non-nil.

@interface EmPair : NSObject
@property (nonatomic, retain) EmBlock *pattern;
@property (nonatomic, retain) EmBlock *key; // Eventually should this be an EmValue?
@property (nonatomic, retain) EmBlock *value;
@property (nonatomic, retain) EmClosure *invoke;
- (BOOL)match:(EmBlock *)key session:(EmSession *)session;
- (EmBlock *)apply:(EmBlock *)argument session:(EmSession *)session;
@end

// Common atoms

EmValueAtom *atom_set();
EmValueAtom *atom_this();