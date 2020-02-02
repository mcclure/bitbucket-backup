//
// Polycode template. Write your code here.
//

#include "Polycode.h"

#ifdef __APPLE__
#import "PolycodeView.h"
#else
//typedef bool BOOL;
#include "PolycodeView.h"
#ifdef _WINDOWS
#include "PolyWinCore.h"
#else
#include "PolySDLCore.h" 
#endif
#endif

// Used for special debug builds with the "DOS box" to capture stderr.
#define WIN_CONSOLE 0

using namespace Polycode;

class PolycodeTemplateApp {
public:
    PolycodeTemplateApp(PolycodeView *view);
    ~PolycodeTemplateApp();
    
    bool Update();
    
private:
    Core *core;
};
