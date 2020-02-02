#include "windows.h"
#include <Polycode.h>
#include "PolycodeTemplateApp.h"
#include "PolycodeView.h"

using namespace Polycode;

#define NAME L"Polyethylene"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	PolycodeView *view = new PolycodeView(hInstance, nCmdShow, NAME);
	PolycodeTemplateApp *app = new PolycodeTemplateApp(view);
	
	MSG Msg;
	do {
		if(PeekMessage(&Msg, NULL, 0,0,PM_REMOVE)) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	} while(app->Update());
	return Msg.wParam;
}
