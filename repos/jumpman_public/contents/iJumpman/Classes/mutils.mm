#import "mutils.h"

@implementation CheckButton

@synthesize checked;

- (id)initWithFrame:(CGRect)aRect {
    self = [super initWithFrame:aRect];
    if (self) {
        [self checkInit];
    }
    return self;
}

- (void)awakeFromNib {
    [super awakeFromNib];
    [self checkInit];
}

- (void)checkInit {
   [self addTarget: self action: @selector(clicked:) forControlEvents: UIControlEventTouchUpInside];
   self.checked = false;
}
   
- (void)checkedset:(BOOL)_checked {
    checked = _checked;
    UIImage *checkedImage = [UIImage imageNamed:@"ibchecked.png"],
    *uncheckedImage = [UIImage imageNamed:@"ibunchecked.png"],
    *inCheckedImage = [UIImage imageNamed:@"ibinchecked.png"],
    *inUncheckedImage = [UIImage imageNamed:@"ibinunchecked.png"];
    [self setImage:checked?inCheckedImage:inUncheckedImage forState:UIControlStateNormal];
    [self setImage:checked?uncheckedImage:checkedImage forState:UIControlStateHighlighted];
}

- (IBAction)clicked:(id)sender {
    self.checked = !checked;
    [self sendActionsForControlEvents: UIControlEventValueChanged];
}

@end