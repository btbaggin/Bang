#include "typedefs.h"

template <typename T>
class FreeList
{
private:
	struct FreeListNode
	{
		T item;
		FreeListNode* next;
	};

	FreeListNode* list = nullptr;

public:
	T Request()
	{
		T result = 0;
		if (list)
		{
			FreeListNode* node = list;
			result = list->item;
			list = list->next;
			delete node;
		}

		return result;
	}

	void Append(T pItem)
	{
		FreeListNode* node = new FreeListNode;
		node->item = pItem;

		node->next = list;
		list = node;
	}

	bool Contains(T pItem)
	{
		for (FreeListNode* node = list; node; node = node->next)
		{
			if (node->item == pItem) return true;
		}
		return false;
	}

	void Clear()
	{
		list = nullptr;
	}
};


template <typename T>
class List
{
public:
	T* items;
	u32 count;

	void AddItem(T item)
	{
		items[count++] = item;
	}
};

template<typename T, unsigned N>
class StaticList
{
public:
	T items[N];
	u32 count;

	void AddItem(T item)
	{
		items[count++] = item;
	}
};