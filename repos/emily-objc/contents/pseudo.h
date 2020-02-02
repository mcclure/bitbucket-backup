/* Emily language prototype -- pseudoblocks -- header
	(c) Andi McClure, May 2014 */

@interface EmSpecialPrint : EmBlock
@end

@interface EmSpecialLoop : EmBlock
@end

@interface EmCurriedNum : EmBlock // This might be redundant
@property (nonatomic, assign) double curry;
- initWithCurry:(EmBlock *)argument;
@end

@interface EmNumAdd : EmCurriedNum
@end

@interface EmNumSub : EmCurriedNum
@end

@interface EmCurriedBlock : EmBlock // This might be redundant
@property (nonatomic, retain) EmBlock *curry;
- initWithCurry:(EmBlock *)obj;
@end

@interface EmNumGt  : EmCurriedBlock
@end

@interface EmNumLt  : EmCurriedBlock
@end

@interface EmNumEq  : EmCurriedBlock
@end

@interface EmSetTarget : EmCurriedBlock
@end

@interface EmSetTargetKey : EmBlock
@property (nonatomic, retain) EmStorageBlock *target;
@property (nonatomic, retain) EmBlock *key;
- initWithTarget:(EmBlock *)argument key:(EmBlock *)key;
@end

@interface EmSpecialTern : EmBlock
@property (nonatomic, retain) EmBlock *condition;
@property (nonatomic, retain) EmBlock *a;
- initWithCondition:(EmBlock *)condition a:(EmBlock *)a;
@end
