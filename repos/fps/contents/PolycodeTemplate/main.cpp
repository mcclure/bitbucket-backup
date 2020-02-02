#include "PolyCore.h"
#include "PolycodeTemplateApp.h"

// main() for Windows and Linux.

int main(int argc, char *argv[]) { 
	PolycodeView *view = new PolycodeView("7DRL"); // TODO: Don't hardcode name here
	PolycodeTemplateApp *app = new PolycodeTemplateApp(view);

	while(app->Update());
	return 0;
} 