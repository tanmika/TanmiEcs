#include"../../src/TanmiEcs.hpp"
#include <iostream>

using namespace TanmiEngine;
auto main() -> int
{
	struct Timer
	{
		int time;
	};
	Sence sence;
	Command cmd(sence);
	cmd.Spawn(new int(1), new float(0.5f))
		.Spawn(new double(0.3f), new int(2))
		.Spawn(Timer{ 1 }, new int(3))
		.Execute();
	sence.SetResource(Timer{ 1 });

	Queryer stage(sence);
	auto entity_has_int = stage.GetEntitys<int*>();
	for (auto e : entity_has_int)
	{
		if (stage.HasComponent<float*>(e))
		{
			std::cout << "the entity has float id is " << *(stage.GetComponent<int*>(e)) << std::endl;
		}
		if (stage.HasComponent<double*>(e))
		{
			std::cout <<"the entity has double id is "<< *(stage.GetComponent<int*> (e))<<std::endl;
		}
		if (stage.HasComponent<Timer>(e))
		{
			std::cout << "the entity has Timer id is " << *(stage.GetComponent<int*> (e)) << std::endl;
		}
	}
	auto entity_has_int_and_timer = stage.GetEntitys<int*, Timer>();
	for (auto e : entity_has_int_and_timer)
	{
		std::cout << "the entity has Timer id is " << *(stage.GetComponent<int*> (e)) << std::endl;
	}
	std::cout << std::endl;
}
