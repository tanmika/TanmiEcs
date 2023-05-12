/*****************************************************************//**
 * \file   TanmiEcs.hpp
 * \brief  一个ECS架构的游戏引擎核心
 *
 * \author tanmika
 * \date   May 2023
 *********************************************************************/
#pragma once

#include <assert.h>
#include <unordered_map>
#include <memory>
#include <functional>
#include "TanmiEcsTools.hpp"

#define assertm(exp, msg) assert(((void)msg, exp))
using EntityID = int;
using ComponentID = int;
using SystemID = int;
using SenceID = int;
using createFunc = void* (*)(void);
using destoryFunc = void(*)(void*);

namespace TanmiEngine {
	class Scene;
	class Resource;
	class Command;
	class System;
	class Queryer;
	class Event;
	using UpdateSystem = void (*)(Command&, Queryer, Resource, Event&);
	using StartupSystem = void (*)(Command&, Resource);

	class Sence final
	{
	public:
		friend class Resource;
		friend class Command;
		friend class Queryer;
		Sence()
		{
			_id = IDGenerator<SenceID>::GetID();
		}	///< 构造函数
		Sence(const Sence&) = delete;				///< 禁止赋值构造
		Sence& operator = (const Sence&) = delete;	///< 禁止赋值构造
	public:
		/**
		 * @brief 添加一类资源
		 *
		 * @return 本体引用
		 */
		template<typename T>
		Sence& SetResource(T&& resource)
		{
			Command cmd(*this);
			cmd.SetResource(std::forward<T>(resource));
			return *this;
		}
		/**
		 * @brief 获取一类资源
		 *
		 * @return 资源
		 */
		template<typename T>
		T* GetResource()
		{
			Resource res;
			if (res.Has<T>())
			{
				return &res.Get<T>();
			}
			else
			{
				return nullptr;
			}
		}
		/**
		 * @brief 启用场景
		 */
		void Start()
		{
			std::vector<Command> cmd_list;

			for (auto sys : _startupSystems)
			{
				Command cmd(*this);
				sys(cmd, Resource{*this});
				cmd_list.push_back(cmd);
			}
			for (auto& cmd : cmd_list)
			{
				cmd.Execute();
			}
		}
		/**
		 * @brief 更新场景
		 */
		void Update()
		{
			std::vector<Command> cmd_list;
			for (auto sys : _updateSystems)
			{
				Command cmd(*this);
				/*TODO:update*/
				cmd_list.push_back(cmd);
			}
			for (auto& cmd : cmd_list)
			{
				cmd.Execute();
			}
		}
		/**
		 * @brief 终止场景
		 */
		void ShutDown()
		{
			_entitys.clear();
			_resources.clear();
			_components.clear();
		}
	private:
		/**
		 * @brief 组件索引表, 用于存储组件的内存池以及标识与实体的对应关系
		 */
		struct ComponentInfo
		{
			Pool pool;
			std::unordered_map<EntityID, void*> entity_map;	///< 拥有组件的实体索引
			ComponentInfo(createFunc crt, destoryFunc des) :pool(crt, des)
			{}
			ComponentInfo() :pool(nullptr, nullptr)
			{}
		};
		/**
		 * @brief 资源索引表, 用于资源的生命期管理
		 */
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
		/**
		 * @brief 组件索引map, <组件ID, 组件索引表>
		 */
		using ComponentMap = std::unordered_map<ComponentID, ComponentInfo>;
		ComponentMap _components;	///< 场景容器索引
		/**
		 * @brief 组件容器, <组件ID, 组件对象指针>
		 */
		using ComponentContainer = std::unordered_map<ComponentID, void*>;
		/**
		 * @brief 实体索引map, <实体ID, 组件容器>
		 */
		using EntityMap = std::unordered_map<EntityID, ComponentContainer>;
		EntityMap _entitys;		///< 场景实体索引
		/**
		 * @brief 资源索引map, <组件ID, 资源索引表>
		 */
		using ResouceMap = std::unordered_map<ComponentID, ResourceInfo>;
		ResouceMap _resources;	///< 场景资源索引

		std::vector<StartupSystem> _startupSystems;	///< 场景启用调用系统列表
		std::vector<UpdateSystem> _updateSystems;	///< 场景更新调用系统列表
	};
	/**
	 * @brief 资源类, 用于资源管理
	 */
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
		/**
		 * @brief 添加一组组件以生成一个实体
		 *
		 * @param 组件
		 * @return 本身
		 */
		template<typename ... ComponentTypes>
		Command& Spawn(ComponentTypes&& ... components)
		{
			SpawnAndGet<ComponentTypes ...>(std::forward<ComponentTypes>(components)...);
			return *this;
		}
		/**
		 * @brief 添加一组组件以生成一个实体并返回
		 *
		 * @param 组件
		 * @return 实体ID
		 */
		template<typename ... ComponentTypes>
		EntityID SpawnAndGet(ComponentTypes&& ... components)
		{
			EntitySpawnInfo info;
			info.id = IDGenerator<EntityID>::GetID();
			AddComponent<ComponentTypes ...>(info.components, std::forward<ComponentTypes>(components)...);
			_spawn_entitys.push_back(info);
			return info.id;
		}
		/**
		 * @brief 删除一个实体
		 *
		 * @param id 实体ID
		 * @return 本身
		 */
		Command& Destroy(EntityID id)
		{
			_destroy_entitys.push_back(id);
			return *this;
		}
		/**
		 * @brief 添加一类资源
		 *
		 * @param resource 资源
		 * @return 本身
		 */
		template<typename T>
		Command& SetResource(T&& resource)
		{
			auto index = IndexGenerator::Get<T>();
			if (auto it = _sence._resources.find(index);
				it == _sence._resources.end())
			{
				auto res = _sence._resources.emplace(index, Sence::ResourceInfo(
					[](void* elem)
					{
						delete (T*)elem;
					}
				));
				res.first->second.resource = new T(std::move(std::forward<T>(resource)));
			}
			else
			{
				assertm(it->second.resource, "资源已存在");
				it->second.resource = new T(std::move(std::forward<T>(resource)));
			}
			return *this;
		}
		template<typename T>
		Command& RemoveResource()
		{
			auto index = IndexGenerator::Get<T>();
			_destory_resource.emplace_back(index, [](void* elem)
				{
					delete (T*)elem;
				});
			return *this;
		}
		/**
		 * @brief 完成场景编辑
		 */
		void Execute()
		{
			for (auto& entity : _destroy_entitys)
			{
				DestoryEntity(entity);
			}
			for (auto& resource : _destory_resource)
			{
				removeResource(resource);
			}
			for (auto& entitys : _spawn_entitys)
			{
				auto it =
					_sence._entitys.emplace(entitys.id, Sence::ComponentContainer{});
				auto& container = it.first->second;
				for (auto& componentInfo : entitys.components)
				{
					container[componentInfo.index] = AddComponentToSence(entitys.id, componentInfo);
				}
			}
		}
	private:
		struct ComponentSpawnInfo;
		/**
		 * @brief 为实体添加组件
		 *
		 * @param component_spawn_info 组件信息表
		 * @param component	待添加组件
		 * @param ...remains 剩余组件
		 */
		template<typename T, typename ... Remains>
		void AddComponent(std::vector<ComponentSpawnInfo>& component_spawn_info, T&& component, Remains ... remains)
		{
			ComponentSpawnInfo info;
			info.index = IndexGenerator::Get<T>();
			info.assign = [=](void* elem)
			{
				*static_cast<T*>(elem) = component;
			};
			info.create = []() -> void*
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
		/**
		 * @brief 为实体由场景组件对象池生成并添加组件
		 *
		 * @param entity 实体id
		 * @param info 组件信息
		 * @return 组件
		 */
		void* AddComponentToSence(EntityID entity, ComponentSpawnInfo& info)
		{
			if (auto it = _sence._components.find(info.index);
				it == _sence._components.end())
			{
				_sence._components.emplace(info.index,
					Sence::ComponentInfo(info.create, info.destory));
			}
			auto& componentInfo = _sence._components[info.index];
			void* elem = componentInfo.pool.create();
			info.assign(elem);
			componentInfo.entity_map[entity] = elem;
			return elem;
		}
		/**
		 * @brief 从场景中删除实体
		 *
		 * @param entity 实体
		 */
		void DestoryEntity(EntityID entity)
		{
			if (auto it = _sence._entitys.find(entity);
				it != _sence._entitys.end())
			{
				for (auto& [id, component] : it->second)
				{
					auto& componentInfo = _sence._components[id];
					componentInfo.pool.destory(component);
					componentInfo.entity_map.erase(id);
				}
			}
		}
		/**
		 * @brief  从场景中移除资源
		 *
		 * @param info 资源信息
		 */
		struct ResourceDestoryInfo;
		void removeResource(ResourceDestoryInfo& info)
		{
			if (auto it = _sence._resources.find(info._index);
				it != _sence._resources.end())
			{
				info._destory(it->second.resource);
				it->second.resource = nullptr;
			}
		}
	private:
		using AssignFunc = std::function<void(void*)>;
		/**
		 * @brief 组件信息表
		 */
		struct ComponentSpawnInfo
		{
			AssignFunc assign;	///< 组件赋值函数
			createFunc create;	///< 组件创建函数
			destoryFunc destory;	///< 组件销毁函数
			ComponentID index;	///< 组件索引
		};
		/**
		 * @brief 实体信息表
		 */
		struct EntitySpawnInfo
		{
			std::vector<ComponentSpawnInfo> components;	///< 实体拥有的组件
			EntityID id;	///< 实体ID
		};
		/**
		 * @brief 待摧毁资源索引
		 */
		struct ResourceDestoryInfo
		{
			int _index;	///< 类型索引
			destoryFunc _destory;  ///< 析构函数
			ResourceDestoryInfo(int index, destoryFunc destory) :_index(index), _destory(destory)
			{}
		};
	private:
		Sence& _sence;	///< 对象场景
		std::vector<EntitySpawnInfo> _spawn_entitys;	///< 待生成的实体
		std::vector<EntityID> _destroy_entitys;	///< 待销毁的实体
		std::vector<ResourceDestoryInfo> _destory_resource; ///< 待销毁的资源
	};
	class System
	{
	public:
		System();
		~System();

	private:
	};
	/**
	 * @brief 查询器
	 */
	class Queryer
	{
	public:
		Queryer() = delete;
		Queryer(Sence& _sence) :sence(_sence)
		{}
		~Queryer() = default;
		/**
		 * @brief 获取包含指定组件的实体
		 * 
		 * @tparam 需要查询的组件
		 * @return 实体列表 
		 */
		template<typename ...Components>
		std::vector<EntityID> GetEntitys()const
		{
			std::vector<EntityID> entitys;
			QueryEntitys<Components...>(entitys);
			return entitys;
		}
		/**
		 * @brief 查询实体是否拥有特定组件
		 * 
		 * @tparam Component 组件类型
		 * @param entity 实体
		 * @return 是否包含
		 */
		template<typename Component>
		bool HasComponent(EntityID entity)const
		{
			auto it = sence._entitys.find(entity);
			auto index = IndexGenerator::Get<Component>();
			return(it != sence._entitys.end() && it->second.find(index) != it->second.end());
		}
		/**
		 * @brief 获取实体拥有的组件
		 * 
		 * @tparam Component 组件类型
		 * @param entity 实体
		 * @return 组件引用
		 */
		template<typename Component>
		Component& GetComponent(EntityID entity)
		{
			auto index = IndexGenerator::Get<Component>();
			assertm(sence._entitys[entity][index], "组件不存在");
			return *((Component*)sence._entitys[entity][index]);
		}
	private:
		/**
		 * @brief 对实体进行首次查询并分发
		 * 
		 * @tparam T 首次组件类型
		 * @tparam Remain 剩余组件类型
		 * @param entity_list 查询的实体列表
		 */
		template<typename T, typename ...Remain>
		void QueryEntitys(std::vector<EntityID>& entity_list)const
		{
			auto index = IndexGenerator::Get<T>();
			Sence::ComponentInfo& info = sence._components[index];
			for (auto &e : info.entity_map)
			{
				if constexpr (sizeof...(Remain) != 0)
				{
					QueryEntitysRemain<Remain...>(e.first, entity_list);
				}
				else
				{
					entity_list.push_back(e.first);
				}
			}
		}
		/**
		 * @brief 对符合初次结果的实体的剩余组件进行查询
		 *
		 * @tparam T 组件类型
		 * @tparam Remain 剩余组件类型
		 * @param entity_list 查询的实体列表
		 */
		template<typename T, typename ...Remain>
		void QueryEntitysRemain(EntityID entity, std::vector<EntityID>& entity_list)const
		{
			auto index = IndexGenerator::Get<T>();
			auto& container = sence._entitys[entity];
			if (auto it = container.find(index);
				it == container.end())
			{
				return;
			}
			if constexpr (sizeof...(Remain) != 0)
			{
				QueryEntitysRemain<Remain...>(entity, entity_list);
			}
			else
			{
				entity_list.push_back(entity);
			}
		}
	private:
		Sence& sence;
	};
	class Plugin
	{
	public:
		Plugin();
		~Plugin();

	private:
	};
}