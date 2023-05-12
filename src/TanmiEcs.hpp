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
		}	///< ���캯��
		Sence(const Sence&) = delete;				///< ��ֹ��ֵ����
		Sence& operator = (const Sence&) = delete;	///< ��ֹ��ֵ����
	public:
		/**
		 * @brief ���һ����Դ
		 *
		 * @return ��������
		 */
		template<typename T>
		Sence& SetResource(T&& resource)
		{
			Command cmd(*this);
			cmd.SetResource(std::forward<T>(resource));
			return *this;
		}
		/**
		 * @brief ��ȡһ����Դ
		 *
		 * @return ��Դ
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
		 * @brief ���ó���
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
		 * @brief ���³���
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

		std::vector<StartupSystem> _startupSystems;	///< �������õ���ϵͳ�б�
		std::vector<UpdateSystem> _updateSystems;	///< �������µ���ϵͳ�б�
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
		/**
		 * @brief ɾ��һ��ʵ��
		 *
		 * @param id ʵ��ID
		 * @return ����
		 */
		Command& Destroy(EntityID id)
		{
			_destroy_entitys.push_back(id);
			return *this;
		}
		/**
		 * @brief ���һ����Դ
		 *
		 * @param resource ��Դ
		 * @return ����
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
				assertm(it->second.resource, "��Դ�Ѵ���");
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
		 * @brief ��ɳ����༭
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
		/**
		 * @brief Ϊʵ���ɳ��������������ɲ�������
		 *
		 * @param entity ʵ��id
		 * @param info �����Ϣ
		 * @return ���
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
		 * @brief �ӳ�����ɾ��ʵ��
		 *
		 * @param entity ʵ��
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
		 * @brief  �ӳ������Ƴ���Դ
		 *
		 * @param info ��Դ��Ϣ
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
		/**
		 * @brief ���ݻ���Դ����
		 */
		struct ResourceDestoryInfo
		{
			int _index;	///< ��������
			destoryFunc _destory;  ///< ��������
			ResourceDestoryInfo(int index, destoryFunc destory) :_index(index), _destory(destory)
			{}
		};
	private:
		Sence& _sence;	///< ���󳡾�
		std::vector<EntitySpawnInfo> _spawn_entitys;	///< �����ɵ�ʵ��
		std::vector<EntityID> _destroy_entitys;	///< �����ٵ�ʵ��
		std::vector<ResourceDestoryInfo> _destory_resource; ///< �����ٵ���Դ
	};
	class System
	{
	public:
		System();
		~System();

	private:
	};
	/**
	 * @brief ��ѯ��
	 */
	class Queryer
	{
	public:
		Queryer() = delete;
		Queryer(Sence& _sence) :sence(_sence)
		{}
		~Queryer() = default;
		/**
		 * @brief ��ȡ����ָ�������ʵ��
		 * 
		 * @tparam ��Ҫ��ѯ�����
		 * @return ʵ���б� 
		 */
		template<typename ...Components>
		std::vector<EntityID> GetEntitys()const
		{
			std::vector<EntityID> entitys;
			QueryEntitys<Components...>(entitys);
			return entitys;
		}
		/**
		 * @brief ��ѯʵ���Ƿ�ӵ���ض����
		 * 
		 * @tparam Component �������
		 * @param entity ʵ��
		 * @return �Ƿ����
		 */
		template<typename Component>
		bool HasComponent(EntityID entity)const
		{
			auto it = sence._entitys.find(entity);
			auto index = IndexGenerator::Get<Component>();
			return(it != sence._entitys.end() && it->second.find(index) != it->second.end());
		}
		/**
		 * @brief ��ȡʵ��ӵ�е����
		 * 
		 * @tparam Component �������
		 * @param entity ʵ��
		 * @return �������
		 */
		template<typename Component>
		Component& GetComponent(EntityID entity)
		{
			auto index = IndexGenerator::Get<Component>();
			assertm(sence._entitys[entity][index], "���������");
			return *((Component*)sence._entitys[entity][index]);
		}
	private:
		/**
		 * @brief ��ʵ������״β�ѯ���ַ�
		 * 
		 * @tparam T �״��������
		 * @tparam Remain ʣ���������
		 * @param entity_list ��ѯ��ʵ���б�
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
		 * @brief �Է��ϳ��ν����ʵ���ʣ��������в�ѯ
		 *
		 * @tparam T �������
		 * @tparam Remain ʣ���������
		 * @param entity_list ��ѯ��ʵ���б�
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