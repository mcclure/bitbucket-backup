/* Emily language prototype -- classes -- header
	(c) Andi McClure, May 2014 */

#import "ast.h"

@interface EmSession : NSObject

@property (nonatomic, retain) NSMutableArray *ops;

- (int)consumeFile:(NSString *)filename;
- (int)consumeString:(NSString *)contents source:(NSString *)label takeOwnership:(BOOL)ownContents;
- (EmBlock *)interpret:(NSArray *)tokens withScope:(EmBlock *)scope;

@end

@interface EmRootScope : EmPseudoBlock
@end