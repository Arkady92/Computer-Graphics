#include "gk2_tessellation.h"
#include "gk2_window.h"
#include "gk2_exceptions.h"

using namespace std;
using namespace gk2;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	UNREFERENCED_PARAMETER(prevInstance);
	UNREFERENCED_PARAMETER(cmdLine);
	shared_ptr<ApplicationBase> app;
	shared_ptr<Window> w;
	int exitCode = 0;
	try
	{
		app.reset(new Tessellation(hInstance));
		w.reset(new Window(hInstance, 800, 800, L"Teselacja"));
		exitCode = app->Run(w.get(), cmdShow);
	}
	catch (Exception& e)
	{
		MessageBoxW(NULL, e.getMessage().c_str(), L"B��d", MB_OK);
		exitCode = e.getExitCode();
	}
	catch (std::exception& e)
	{
		string s(e.what());
		MessageBoxW(NULL, wstring(s.begin(), s.end()).c_str(), L"B��d!", MB_OK);
	}
	catch(...)
	{
		MessageBoxW(NULL, L"Nieznany B��d", L"B��d", MB_OK);
		exitCode = -1;
	}
	w.reset();
	app.reset();
	return exitCode;
}