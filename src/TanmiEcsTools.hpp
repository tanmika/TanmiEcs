/*****************************************************************//**
 * \file   TanmiEcsTools.hpp
 * \brief  ������ظ����������ṹ
 *
 * \author tanmika
 * \date   May 2023
 *********************************************************************/
#pragma once

#include <assert.h>
#include <optional>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace TanmiEngine {
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
	* @brief �����
	*/
	using createFunc = void* (*)(void);
	using destoryFunc = void(*)(void*);
	struct Pool final
	{
		std::vector<void*> instances;	///< ������еĿ��ö���
		std::vector<void*> cache;		///< ������еĻ������

		createFunc create_f;			///< ��������ĺ���
		destoryFunc destory_f;			///< ���ٶ���ĺ���
		/**
		 * @brief �������󲢷���
		 *
		 * @return �¶���
		 */
		void* create()
		{
			if (cache.empty())
			{
				instances.push_back(create_f());
			}
			else
			{
				instances.push_back(cache.back());
				cache.pop_back();
			}
			return instances.back();
		}
		/**
		 * @brief ɾ��ָ������
		 *
		 * @param ����
		 */
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

		Pool(createFunc crt, destoryFunc des) :create_f(crt), destory_f(des)
		{
			assertm("create function cann't be empty", create_f);
			assertm("destory function cann't be empty", destory_f);
		}
	};
	template<typename T>
	class EventMessage final
	{
	public:
		static void Set(const T& msg)
		{
			_msg_next = msg;
		};
		static void Set(T&& msg)
		{
			_msg_next = std::move(msg);
		};
		static std::optional<T>& Get()
		{
			return _msg;
		};
		static bool Has()
		{
			return _msg.has_value();
		};
		static void Clear()
		{
			_msg = std::nullopt;
			_msg_next = std::nullopt;
		}
		static void Update()
		{
			if (_msg_next.has_value())
			{
				_msg = _msg_next;
				_msg_next = std::nullopt;
			}
		};
	private:
		inline static std::optional<T> _msg = std::nullopt;
		inline static std::optional<T> _msg_next = std::nullopt;
	};
}