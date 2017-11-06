#pragma once
#include <cstdint>
#include <cassert>

namespace vpl {
	//simple map
	//unsorted, seperate lists for keyes and values, great for cache coherency, 
	//linear search is good enough for small data collections
	template<typename Key, typename Value, uint32_t SIZE>
	class Map
	{
	public:
		bool GetValue(const Key& key, Value& out_result) const
		{
			int32_t index = -1;
			for (uint32_t i = 0; i < num_items; ++i)
			{
				if (keyes[i] == key)
				{
					index = (int32_t)i;
					break;
				}
			}
			if (index != -1)
			{
				out_result = values[index];
				return true;
			}
			else
			{
				return false;
			}
		}
		bool Exists(const Key& key) const
		{
			int32_t index = -1;
			for (uint32_t i = 0; i < num_items; ++i)
			{
				if (keyes[i] == key)
				{
					index = (int32_t)i;
					break;
				}
			}
			return index != -1;
		}
		void Add(const Key& key, const Value& value)//force copy
		{
			assert(num_items < SIZE);
			if (num_items == 0 || !Exists(key))
			{
				keyes[num_items] = key;
				values[num_items] = value;
				++num_items;
			}
		}
		void Remove(const Key& key)
		{
			for (uint32_t i = 0; i < num_items; ++i)
			{
				if (keyes[i] == key)
				{//overwrite the 'pair' we want to delete with the last 'pair'
					keyes[i] = keyes[num_items - 1];
					values[i] = values[num_items - 1];
					--num_items;
				}
			}
		}

	private:
		Key keyes[SIZE];
		Value values[SIZE];
		uint32_t num_items;
	};
}