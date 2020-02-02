#import <UIKit/UIKit.h>
#include <string>

// #define MUTILS_USING_3 TARGET_IPHONE_SIMULATOR
#define MUTILS_USING_3 1

// Utility classes:

@interface CheckButton : UIButton {
	BOOL checked;
}
@property (setter=checkedset:) BOOL checked;

- (id)initWithFrame:(CGRect)aRect;
- (void)awakeFromNib;
- (void)checkInit;
- (void)checkedset:(BOOL)_checked; // Would prefer property
- (IBAction)clicked:(id)sender;

@end

// Conversion functions:

inline NSString * toNs(const std::string &s) {
    return [[[NSString alloc] initWithBytes:s.c_str() length:s.size() encoding: NSUTF8StringEncoding] autorelease];
}

inline NSString * toNs(const char *s) {
    return s?[[[NSString alloc] initWithBytes:s length:strlen(s) encoding: NSUTF8StringEncoding] autorelease]:[NSString string];
}

inline const char *toString(NSString *s) { return s?[s UTF8String]:""; }

// Workarounds for Apple's sloppy deprecation:

inline void setText(UITableViewCell *cell, NSString *s) {
#if MUTILS_USING_3
    cell.textLabel.text = s; // TODO: Gate on 3.0, not simulator
#else
    cell.text = s;
#endif
}

inline void setFont(UIButton *button, UIFont *font) {
#if MUTILS_USING_3
    button.titleLabel.font = font; // TODO: Gate on 3.0, not simulator
#else
    button.font = font;
#endif
}

NSString *_(NSString *in);