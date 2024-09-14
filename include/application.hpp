#pragma once

#include "my_types.hpp"

namespace Application {
	bool OnInitialize(const CorePtr core);
	void OnDestroy(const CorePtr core);
	bool OnFrame(const CorePtr core);
}
