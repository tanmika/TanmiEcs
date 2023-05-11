/*****************************************************************//**
 * \file   TanmiEcsTools.hpp
 * \brief  ������ظ����������ṹ
 * 
 * \author tanmika
 * \date   May 2023
 *********************************************************************/
#pragma once

#include <assert.h>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace TanmiEngine{
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
}
