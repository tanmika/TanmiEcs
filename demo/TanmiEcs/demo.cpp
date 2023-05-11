#include"../../src/TanmiEcs.hpp"
#include <iostream>

using namespace TanmiEngine;
auto main() -> int
{
	Sence sence;
	Command cmd(sence);
	cmd.Spawn(new int(1), new float(0.5f))
		.Spawn(new double(0.3f), new int(2));

	std::cout << std::endl;
}
