#include "PolyCore.h"
#include "PolycodeTemplateApp.h"

int main(int argc, char *argv[]) { 
	Base *view = new Base(); 
	PolycodeTemplateApp *app = new PolycodeTemplateApp(view);

	while(app->Update());
	return 0;
} 
