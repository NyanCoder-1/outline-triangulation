#include "core.hpp"
#include "application.hpp"

int main()
{
	auto core = Core::Create();
	if (!core)
		return 1;

	core->SetOnInitCallback(Application::OnInitialize);
	core->SetOnDestroyCallback(Application::OnDestroy);
	core->SetOnFrameCallback(Application::OnFrame);

	core->Run();

	return 0;
}