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
	class Command;
	class System;
	class Scene;
	class Resource;
	/**
	 * @brief 用于生成不同类型唯一的索引
	 */
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
		inline static int _idx = 0;
	};
	/**
	 * @brief 用于生成同类型不同对象唯一的ID
	 */
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
	/**
	 * @brief 场景类，用于构建一个局部游戏场景
	 */
	class Sence final
	{
	public:
		friend class Resource;
		friend class Command;
		Sence()
		{
			_id = IDGenerator<SenceID>::GetID();
		}	///< 构造函数
		Sence(const Sence & ) = delete;				///< 禁止赋值构造
		Sence& operator = (const Sence&) = delete;	///< 禁止赋值构造
	public:
		/**
		 * @brief 添加一类资源
		 * 
		 * @return 本体引用
		 */
		template<typename T>
		Sence& SetResource(T&& resource);
		/**
		 * @brief 获取一类资源
		 * 
		 * @return 资源
		 */
		template<typename T>
		T* GetResource();
		/**
		 * @brief 启用场景
		 */
		void Start();
		/**
		 * @brief 更新场景
		 */
		void Update();
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
	private:
		Sence& _sence;
		std::vector<EntitySpawnInfo> _spawn_entitys;	///< 待生成的实体
	};
}