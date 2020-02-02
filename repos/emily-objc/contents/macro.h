/* Emily language prototype -- macros -- header
	(c) Andi McClure, May 2014 */

#include "emily.h"

@interface EmMacro : NSObject
- (NSArray *)newBasedOn:(NSArray *)input scope:(EmBlock *)scope; // Darn it NARC
@end

@interface EmMacroClosure : EmMacro
@end