#pragma once

#include <assert.h>
#include <unordered_map>
#include <memory>
#include <functional>

#define assertm(exp, msg) assert(((void)msg, exp))
using EntityID = int;
using ComponentID = int;
using SystemID = int;
using SenceID = int;
using createFunc = void* (*)(void);
using destoryFunc = void(*)(void*);

namespace TanmiEngine {
	class Command;
	class System;
	class Scene;
	class Resource;

	class IndexGenerator final
	{
	public:
		template<typename T>
		static int Get()
		{
			static int id = _idx++;
			return id;
		}
	private:
		static int _idx;
	};

	template<typename T, typename = std::enable_if<std::is_integral_v<T>>>
	class IDGenerator final
	{
	public:
		static T GetID()
		{
			return _id++;
		}
	private:
		inline static T _id = {};
	};

	class Sence final
	{
	public:
		Sence()
		{
			_id = IDGenerator<SenceID>::GetID();
		}
	private:
		struct Pool final
		{
			std::vector<void*> instances;
			std::vector<void*> cache;

			createFunc create;
			destoryFunc destory;

			void* create()
			{
				if (cache.empty())
				{
					instances.push_back(create());
				}
				else
				{
					instances.push_back(cache.back());
					cache.pop_back();
				}
				return instances.back();
			}

			void destory(void* elem)
			{
				if (auto it = std::find(instances.begin(), instances.end(), elem);
					it != instances.end())
				{
					std::swap(*it, instances.back());
					cache.push_back(instances.back());
					instances.pop_back();
				}
				else
				{
					assertm("element is not found", false);
				}
			}

			Pool(createFunc crt, destoryFunc des) :create(crt), destory(des)
			{
				assertm("create function cann't be empty", create);
				assertm("destory function cann't be empty", destory);
			}
		};

		struct ComponentInfo
		{
			Pool pool;
			std::unordered_map<EntityID, void*> entity_map;
			ComponentInfo(createFunc crt, destoryFunc des) :pool(crt, des)
			{}
			ComponentInfo() :pool(nullptr, nullptr)
			{}
		};

		struct ResourceInfo
		{
			void* resource = nullptr;
			destoryFunc destory;
			ResourceInfo() = default;
			ResourceInfo(destoryFunc des) :destory(des)
			{
				assertm("destory function cann't be empty", destory);
			}
			~ResourceInfo()
			{
				destory(resource);
			}
		};
	private:
		SenceID _id;
		using ComponentMap = std::unordered_map<ComponentID, ComponentInfo>;
		ComponentMap _components;
		using ComponentContainer = std::unordered_map<ComponentID, void*>;
		using EntityMap = std::unordered_map<EntityID, ComponentContainer>;
		EntityMap _entitys;
		using ResouceMap = std::unordered_map<ComponentID, ResourceInfo>;
		ResouceMap _resources;
	};
	class Resource final
	{
	public:
		Resource() = delete;
		Resource(Sence& sence) :_sence(sence)
		{}
		template<typename T>
		bool Has()const
		{
			int index = IndexGenerator::Get<T>();
			auto it = _sence._resources.find(index);
			return it != _sence._resources.end() && it->second.resource != nullptr;;
		}
		template<typename T>
		T& Get()
		{
			int index = IndexGenerator::Get<T>();
			assertm("resource is empty", _sence._resources[index].resource);
			return *static_cast<T*>(_sence._resources[index].resource);
		}
	private:
		Sence& _sence;
	};

	class Command final
	{
	public:
		Command() = delete;
		Command(Sence& sence) :_sence(sence)
		{}
		template<typename ... ComponentTypes>
		Command& Spawn(ComponentTypes&& ... components)
		{
			SpawnAndGet<ComponentTypes ...>(std::forward<ComponentTypes>(components)...);
			return *this;
		}

		template<typename ... ComponentTypes>
		EntityID SpawnAndGet(ComponentTypes&& ... components)
		{
			EntitySpawnInfo info;
			info.id = IDGenerator<EntityID>::GetID();
			AddComponent<ComponentTypes ...>(info.components, std::forward<ComponentTypes>(components)...);
			_spawn_entitys.push_back(info);
			return info.id;
		}
	private:
		struct ComponentSpawnInfo;
		template<typename T, typename ... Remains>
		void AddComponent(std::vector<ComponentSpawnInfo>& component_spawn_info, T&& component, Remains ... remains)
		{
			ComponentSpawnInfo info;
			info.index = IndexGenerator::Get<T>();
			info.assign = [](void* elem)
			{
				*static_cast<T*>(elem) = component;
			};
			info.create = []()
			{
				return new T();
			};
			info.destory = [](void* elem)
			{
				delete static_cast<T*>(elem);
			};
			component_spawn_info.push_back(info);
			if constexpr (sizeof ...(Remains) != 0)
			{
				AddComponent<Remains ...>(component_spawn_info, std::forward<Remains>(remains)...);
			}
		}
	private:
		Sence& _sence;

		using AssignFunc = std::function<void(void*)>;
		struct ComponentSpawnInfo
		{
			AssignFunc assign;
			createFunc create;
			destoryFunc destory;
			ComponentID index;
		};

		struct EntitySpawnInfo
		{
			std::vector<ComponentSpawnInfo> components;
			EntityID id;
		};

		std::vector<EntitySpawnInfo> _spawn_entitys;
	};
}