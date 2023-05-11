/*****************************************************************//**
 * \file   TanmiEcs.hpp
 * \brief  һ��ECS�ܹ�����Ϸ�������
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
	 * @brief �������ɲ�ͬ����Ψһ������
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
	 * @brief ��������ͬ���Ͳ�ͬ����Ψһ��ID
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
	 * @brief �����࣬���ڹ���һ���ֲ���Ϸ����
	 */
	class Sence final
	{
	public:
		friend class Resource;
		friend class Command;
		Sence()
		{
			_id = IDGenerator<SenceID>::GetID();
		}	///< ���캯��
		Sence(const Sence & ) = delete;				///< ��ֹ��ֵ����
		Sence& operator = (const Sence&) = delete;	///< ��ֹ��ֵ����
	public:
		/**
		 * @brief ���һ����Դ
		 * 
		 * @return ��������
		 */
		template<typename T>
		Sence& SetResource(T&& resource);
		/**
		 * @brief ��ȡһ����Դ
		 * 
		 * @return ��Դ
		 */
		template<typename T>
		T* GetResource();
		/**
		 * @brief ���ó���
		 */
		void Start();
		/**
		 * @brief ���³���
		 */
		void Update();
		/**
		 * @brief ��ֹ����
		 */
		void ShutDown()
		{
			_entitys.clear();
			_resources.clear();
			_components.clear();
		}
	private:
		/**
		 * @brief ���������, ���ڴ洢������ڴ���Լ���ʶ��ʵ��Ķ�Ӧ��ϵ
		 */
		struct ComponentInfo
		{
			Pool pool;
			std::unordered_map<EntityID, void*> entity_map;	///< ӵ�������ʵ������
			ComponentInfo(createFunc crt, destoryFunc des) :pool(crt, des)
			{}
			ComponentInfo() :pool(nullptr, nullptr)
			{}
		};
		/**
		 * @brief ��Դ������, ������Դ�������ڹ���
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
		 * @brief �������map, <���ID, ���������>
		 */
		using ComponentMap = std::unordered_map<ComponentID, ComponentInfo>;
		ComponentMap _components;	///< ������������
		/**
		 * @brief �������, <���ID, �������ָ��>
		 */
		using ComponentContainer = std::unordered_map<ComponentID, void*>;
		/**
		 * @brief ʵ������map, <ʵ��ID, �������>
		 */
		using EntityMap = std::unordered_map<EntityID, ComponentContainer>;
		EntityMap _entitys;		///< ����ʵ������
		/**
		 * @brief ��Դ����map, <���ID, ��Դ������>
		 */
		using ResouceMap = std::unordered_map<ComponentID, ResourceInfo>;
		ResouceMap _resources;	///< ������Դ����
	};
	/**
	 * @brief ��Դ��, ������Դ����
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
		 * @brief ���һ�����������һ��ʵ��
		 * 
		 * @param ���
		 * @return ����
		 */
		template<typename ... ComponentTypes>
		Command& Spawn(ComponentTypes&& ... components)
		{
			SpawnAndGet<ComponentTypes ...>(std::forward<ComponentTypes>(components)...);
			return *this;
		}
		/**
		 * @brief ���һ�����������һ��ʵ�岢����
		 * 
		 * @param ���
		 * @return ʵ��ID
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
		 * @brief Ϊʵ��������
		 * 
		 * @param component_spawn_info �����Ϣ��
		 * @param component	��������
		 * @param ...remains ʣ�����
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
		 * @brief �����Ϣ��
		 */
		struct ComponentSpawnInfo
		{
			AssignFunc assign;	///< �����ֵ����
			createFunc create;	///< �����������
			destoryFunc destory;	///< ������ٺ���
			ComponentID index;	///< �������
		};
		/**
		 * @brief ʵ����Ϣ��
		 */
		struct EntitySpawnInfo
		{
			std::vector<ComponentSpawnInfo> components;	///< ʵ��ӵ�е����
			EntityID id;	///< ʵ��ID
		};
	private:
		Sence& _sence;
		std::vector<EntitySpawnInfo> _spawn_entitys;	///< �����ɵ�ʵ��
	};
}