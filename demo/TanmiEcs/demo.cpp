#include"../../src/TanmiEcs.hpp"
#include <iostream>
#include <string>

using namespace TanmiEngine;
struct Timer
{
	float time;
};
struct ID
{
	int id;
};
struct Counter
{
	int num;
	float t;
};
struct Time
{
	float t;
};
struct Name
{
	std::string name;
};

void InsSystem(Command& cmd, Resource res)
{
	auto& timer = res.Get<Timer>().time;
	cmd.Spawn(ID{ 0 }, Counter{ 0, timer }, Name{ "David" })
		.Spawn(ID{ 1 }, Time{ 0.0f }, Name{ "Anny" })
		.Spawn(ID{ 2 }, Time{ 0.0f }, Name{ "Bob" })
		.Spawn(ID{ 3 }, Time{ 0.0f }, Name{ "Cannon" });
	std::cout << "--------Instance--------\n";
}
void RunSystem(Command& cmd, Queryer queryer, Resource res, Event& event)
{
	auto& timer = res.Get<Timer>().time;
	Counter* count = nullptr;
	auto player = queryer.GetEntitys<ID, Time>();
	auto judge = queryer.GetEntitys<ID, Counter>();
	for (auto& e : judge)
	{
		timer += 0.5f;
		count = &queryer.GetComponent<Counter>(e);
		count->num++;
		count->t += timer;
	}
	for (auto& e : player)
	{
		queryer.GetComponent<Time>(e).t += timer;
		std::cout << queryer.GetComponent<Name>(e).name << std::endl;
		if (queryer.GetComponent<ID>(e).id == count->num)
		{
			std::cout << "ID:" << queryer.GetComponent<ID>(e).id 
				<< "  Time:" << queryer.GetComponent<Time>(e).t << std::endl;
		}
	}std::cout << "--------Update--------\n";
}

auto main() -> int
{
	Sence sence;
	sence.AddStartSystem(InsSystem)
		.AddUpdateSystem(RunSystem)
		.SetResource(Timer{ 0 });
	sence.Start();
	sence.Update();
	sence.Update();
	sence.Update();
}