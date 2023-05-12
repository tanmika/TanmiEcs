/*****************************************************************//**
 * \file   TanmiEcsTools.hpp
 * \brief  引擎相关辅助函数、结构
 * 
 * \author tanmika
 * \date   May 2023
 *********************************************************************/
#pragma once

#include <assert.h>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace TanmiEngine{
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
	/**
	* @brief 对象池
	*/
	using createFunc = void* (*)(void);
	using destoryFunc = void(*)(void*);
	struct Pool final
	{
		std::vector<void*> instances;	///< 对象池中的可用对象
		std::vector<void*> cache;		///< 对象池中的缓存对象

		createFunc create_f;			///< 创建对象的函数
		destoryFunc destory_f;			///< 销毁对象的函数
		/**
		 * @brief 创建对象并返回
		 *
		 * @return 新对象
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
		 * @brief 删除指定对象
		 *
		 * @param 对象
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
}
