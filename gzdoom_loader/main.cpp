
#include "gzdoom_launcher.h"
#include "zwidget/window/window.h"
#include "zwidget/core/theme.h"
#include <stdexcept>

int main(int argc, char** argv)
{
	try
	{
		// Initialize display backend
		DisplayBackend::Set(DisplayBackend::TryCreateBackend());

		// Set dark theme by default
		WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>());

		// Create and show the launcher window
		auto launcher = new GZDoomLauncher();
		launcher->SetFrameGeometry(100.0, 100.0, 1100.0, 750.0);
		launcher->Show();

		// Run the event loop
		DisplayWindow::RunLoop();

		return 0;
	}
	catch (const std::exception& e)
	{
		printf("Error: %s\n", e.what());
		return 1;
	}
}
