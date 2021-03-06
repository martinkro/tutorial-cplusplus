// BitMappedMemoryPool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


// https://www.ibm.com/developerworks/aix/tutorials/au-memorymanager/index.html
// g++ -std=c++11 mempool2.cpp
#include "performance.h"
#include <time.h>
#include <sys/types.h>
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
using namespace std;

class Complex
{
public:
	Complex(double a, double b) :r(a), c(b) {}
private:
	double r;
	double c;
};

void test_Normal()
{
	Performance p("test_Normal");
	Complex* array[1000];
	for (int i = 0; i < 5000; i++)
	{
		for (int j = 0; j < 1000; j++)
		{
			array[j] = new Complex(i, j);
		}
		for (int j = 0; j < 1000; j++)
		{
			delete array[j];
		}
	}
}

// mem pool
class Complex2
{
public:
	Complex2(double a, double b) :r(a), c(b) {}
	void* operator new(size_t);
	void operator delete(void* pointerToDeleted);

private:
	double r;
	double c;
};

class IMemoryManager
{
public:
	virtual void* allocate(size_t) = 0;
	virtual void free(void*) = 0;
};


#define POOLSIZE 1024
class MemoryManager :public IMemoryManager
{
	struct FreeStore
	{
		FreeStore* next;
	};
	void expandPoolSize()
	{
		size_t size = sizeof(Complex2) > sizeof(FreeStore*) ? sizeof(Complex2) : sizeof(FreeStore);
		FreeStore* head = reinterpret_cast<FreeStore*>(new char[size]);
		freeStoreHead = head;
		for (int i = 0; i < POOLSIZE; i++)
		{
			head->next = reinterpret_cast<FreeStore*>(new char[size]);
			head = head->next;
		}
		head->next = 0;
	}
	void cleanUp()
	{
		FreeStore* nextPtr = freeStoreHead;
		for (; nextPtr; nextPtr = freeStoreHead)
		{
			freeStoreHead = freeStoreHead->next;
			delete[] nextPtr; // this was a char array
		}
	}
	FreeStore* freeStoreHead;
public:
	MemoryManager()
	{
		freeStoreHead = 0;
		expandPoolSize();
	}
	virtual ~MemoryManager()
	{
		cleanUp();
	}
	virtual void* allocate(size_t size)
	{
		if (0 == freeStoreHead) expandPoolSize();
		FreeStore* head = freeStoreHead;
		freeStoreHead = head->next;
		return head;
	}
	virtual void free(void* deleted)
	{
		FreeStore* head = static_cast<FreeStore*>(deleted);
		head->next = freeStoreHead;
		freeStoreHead = head;
	}
};

MemoryManager gMemoryManager;
void* Complex2::operator new(size_t size)
{
	return gMemoryManager.allocate(size);
}
void Complex2::operator delete(void* pointerToDeleted)
{
	gMemoryManager.free(pointerToDeleted);
}


void test_MemPool()
{
	Performance p("test_MemPool");
	Complex2* array[1000];
	for (int i = 0; i < 5000; i++)
	{
		for (int j = 0; j < 1000; j++)
		{
			array[j] = new Complex2(i, j);
		}
		for (int j = 0; j < 1000; j++)
		{
			delete array[j];
		}
	}
}

// Bitmapped memory manager
// support new []
const int BIT_MAP_SIZE = 1024;
const int INT_SIZE = sizeof(int) * 8;
const int BIT_MAP_ELEMENTS = BIT_MAP_SIZE / INT_SIZE;
//Memory Allocation Pattern
//11111111 11111111 11111111
//11111110 11111111 11111111
//11111100 11111111 11111111
//if all bits for 1st section become 0 proceed to next section

//...
//00000000 11111111 11111111
//00000000 11111110 11111111
//00000000 11111100 11111111
//00000000 11111000 11111111

//The reason for this strategy is that lookup becomes O(1) inside the map 
//for the first available free block
class Complex3
{
public:
	Complex3() :r(0), c(0) {}
	Complex3(double a, double b) :r(a), c(b) {}
	void* operator new(size_t size);
	void operator delete(void* object);
	void* operator new[](size_t size);
	void operator delete[](void* object);
private:
	double r;
	double c;
};
typedef struct ArrayInfo
{
	size_t MemPoolListIndex;   // 对象数组 存储在哪个内存池
	size_t StartPosition;      // 对象数组的开始位置
	size_t Size;               // 数组元素个数
}ArrayMemoryInfo;

typedef struct BitMapEntry
{
	int Index;    // 内存池列表索引 O(1)找到相应的内存池
	int BlocksAvailable;   // 该内存池可用块个数
	int BitMap[BIT_MAP_ELEMENTS];
	void* head;
public:
	BitMapEntry() :BlocksAvailable(BIT_MAP_SIZE) {
		// initially all blocks are free and bit value 1 in the map denotes 
		// available block
		// BIT_MAP_SIZE/sizeof(char) bytes ??
		memset(BitMap, 0xff, sizeof(BitMap));
	}
	void SetBit(size_t position, bool flag)
	{
		BlocksAvailable += flag ? 1 : -1;
		size_t elementNo = position / INT_SIZE;   // 8*4 
		size_t bitNo = position % INT_SIZE;
		if (flag)
		{
			BitMap[elementNo] = BitMap[elementNo] | (1 << bitNo);
		}
		else
		{
			BitMap[elementNo] = BitMap[elementNo] & ~(1 << bitNo);
		}
	}
	void SetMultipleBits(size_t position, bool flag, int count)
	{
		BlocksAvailable += flag ? count : -count;
		size_t elementNo = position / INT_SIZE;
		size_t bitNo = position % INT_SIZE;

		int bitSize = (count <= INT_SIZE - bitNo) ? count : INT_SIZE - bitNo;
		SetRangeOfInt(&BitMap[elementNo], bitNo + bitSize - 1, bitNo, flag);
		count -= bitSize;
		if (!count) return;

		size_t  i = ++elementNo;
		while (count >= 0)
		{
			if (count <= INT_SIZE)
			{
				SetRangeOfInt(&BitMap[i], count - 1, 0, flag);
				return;
			}
			else
				BitMap[i] = flag ? unsigned(-1) : 0;
			count -= 32;
			i++;
		}
	}
	void SetRangeOfInt(int* element, int msb, int lsb, bool flag)
	{
		if (flag)
		{
			int mask = (unsigned(-1) << lsb) & (unsigned(-1) >> INT_SIZE - msb - 1);
			*element |= mask;
		}
		else
		{
			int mask = (unsigned(-1) << lsb) & (unsigned(-1) >> INT_SIZE - msb - 1);
			*element &= ~mask;
		}
	}
	Complex3* FirstFreeBlock(size_t size)
	{
		for (int i = 0; i < BIT_MAP_ELEMENTS; ++i)
		{
			if (BitMap[i] == 0) continue;

			int result = BitMap[i] & -(BitMap[i]);
			void* address = 0;
			int basePos = (INT_SIZE*i);
			switch (result)
			{
				//make the corresponding bit 0 meaning block is no longer free
			case 0x00000001: return ComplexObjectAddress(basePos + 0);
			case 0x00000002: return ComplexObjectAddress(basePos + 1);
			case 0x00000004: return ComplexObjectAddress(basePos + 2);
			case 0x00000008: return ComplexObjectAddress(basePos + 3);
			case 0x00000010: return ComplexObjectAddress(basePos + 4);
			case 0x00000020: return ComplexObjectAddress(basePos + 5);
			case 0x00000040: return ComplexObjectAddress(basePos + 6);
			case 0x00000080: return ComplexObjectAddress(basePos + 7);
			case 0x00000100: return ComplexObjectAddress(basePos + 8);
			case 0x00000200: return ComplexObjectAddress(basePos + 9);
			case 0x00000400: return ComplexObjectAddress(basePos + 10);
			case 0x00000800: return ComplexObjectAddress(basePos + 11);
			case 0x00001000: return ComplexObjectAddress(basePos + 12);
			case 0x00002000: return ComplexObjectAddress(basePos + 13);
			case 0x00004000: return ComplexObjectAddress(basePos + 14);
			case 0x00008000: return ComplexObjectAddress(basePos + 15);
			case 0x00010000: return ComplexObjectAddress(basePos + 16);
			case 0x00020000: return ComplexObjectAddress(basePos + 17);
			case 0x00040000: return ComplexObjectAddress(basePos + 18);
			case 0x00080000: return ComplexObjectAddress(basePos + 19);
			case 0x00100000: return ComplexObjectAddress(basePos + 20);
			case 0x00200000: return ComplexObjectAddress(basePos + 21);
			case 0x00400000: return ComplexObjectAddress(basePos + 22);
			case 0x00800000: return ComplexObjectAddress(basePos + 23);
			case 0x01000000: return ComplexObjectAddress(basePos + 24);
			case 0x02000000: return ComplexObjectAddress(basePos + 25);
			case 0x04000000: return ComplexObjectAddress(basePos + 26);
			case 0x08000000: return ComplexObjectAddress(basePos + 27);
			case 0x10000000: return ComplexObjectAddress(basePos + 28);
			case 0x20000000: return ComplexObjectAddress(basePos + 29);
			case 0x40000000: return ComplexObjectAddress(basePos + 30);
			case 0x80000000: return ComplexObjectAddress(basePos + 31);
			default: break;
			}
		}
		return 0;
	}
	Complex3* ComplexObjectAddress(int pos)
	{
		SetBit(pos, false);
		return &((static_cast<Complex3*>(Head()) + (pos / INT_SIZE))[INT_SIZE - ((pos %  INT_SIZE) + 1)]);
	}
	void* Head()
	{
		return head;
	}
}BitMapEntry;

class BitMappedMemoryManager :public IMemoryManager
{
public:
	BitMappedMemoryManager() {}
	virtual ~BitMappedMemoryManager() {}
	virtual void* allocate(size_t size)
	{
		if (size == sizeof(Complex3))    // mon-array version
		{
			std::set<BitMapEntry*>::iterator freeMapI = FreeMapEntries.begin();
			if (!FreeMapEntries.empty())
			{
				BitMapEntry* mapEntry = *freeMapI;
				return mapEntry->FirstFreeBlock(size);
			}
			else
			{
				AllocateChunkAndInitBitMap();
				FreeMapEntries.insert(&(BitMapEntryList[BitMapEntryList.size() - 1]));
				return BitMapEntryList[BitMapEntryList.size() - 1].FirstFreeBlock(size);
			}
		}
		else  // array version
		{
			if (ArrayMemoryList.empty())
			{
				return AllocateArrayMemory(size);
			}
			else
			{
				std::map<void*, ArrayMemoryInfo>::iterator infoI = ArrayMemoryList.begin();
				std::map<void*, ArrayMemoryInfo>::iterator infoEndI =
					ArrayMemoryList.end();
				while (infoI != infoEndI)
				{
					ArrayMemoryInfo info = (*infoI).second;
					if (info.StartPosition != 0)  // search only in those mem blocks  
						continue;             // where allocation is done from first byte
					else
					{
						BitMapEntry* entry = &BitMapEntryList[info.MemPoolListIndex];
						if (entry->BlocksAvailable < (size / sizeof(Complex3)))
							return AllocateArrayMemory(size);
						else
						{
							info.StartPosition = BIT_MAP_SIZE - entry->BlocksAvailable;
							info.Size = size / sizeof(Complex3);
							Complex3* baseAddress = static_cast<Complex3*>(
								MemoryPoolList[info.MemPoolListIndex]) + info.StartPosition;

							ArrayMemoryList[baseAddress] = info;
							SetMultipleBlockBits(&info, false);

							return baseAddress;
						}
					}
				}
			}
		}
		return 0;
	}
	virtual void free(void* object)
	{
		if (ArrayMemoryList.empty() || (ArrayMemoryList.find(object) == ArrayMemoryList.end()))
		{
			SetBlockBit(object, true); // simple block deletion
		}
		else
		{
			// 应该从ArraryMemoryList移除吗?
			ArrayMemoryInfo* info = &ArrayMemoryList[object];
			SetMultipleBlockBits(info, true);
		}
	}
	vector<void*> GetMemoryPoolList()
	{
		return MemoryPoolList;
	}
private:
	void* AllocateArrayMemory(size_t size)
	{
		void* chunkAddress = AllocateChunkAndInitBitMap();
		ArrayMemoryInfo info;
		info.MemPoolListIndex = MemoryPoolList.size() - 1;
		info.StartPosition = 0;
		info.Size = size / sizeof(Complex);
		ArrayMemoryList[chunkAddress] = info;
		SetMultipleBlockBits(&info, false);
		return chunkAddress;
	}
	void* AllocateChunkAndInitBitMap()
	{
		BitMapEntry mapEntry;
		Complex3* memoryBeiginAddress = reinterpret_cast<Complex3*>(new char[sizeof(Complex3)*BIT_MAP_SIZE]);

		MemoryPoolList.push_back(memoryBeiginAddress);

		mapEntry.Index = MemoryPoolList.size() - 1;
		mapEntry.head = memoryBeiginAddress;
		BitMapEntryList.push_back(mapEntry);

		return memoryBeiginAddress;
	}
	void SetBlockBit(void* object, bool flag)
	{
		int i = BitMapEntryList.size() - 1;
		for (; i >= 0; i--)
		{
			BitMapEntry* bitMap = &BitMapEntryList[i];
			if ((bitMap->Head() <= object)
				&& (&(static_cast<Complex3*>(bitMap->Head()))[BIT_MAP_SIZE - 1] >= object))
			{
				// 找到对应的内存池
				size_t position = static_cast<Complex3*>(object) - static_cast<Complex3*>(bitMap->Head());
				position = (position / INT_SIZE) * INT_SIZE + (INT_SIZE - (position % INT_SIZE + 1));
				bitMap->SetBit(position, flag);
				//flag ? bitMap->BlocksAvailable++ : bitMap->BlocksAvailable--;
			}
		}
	}
	void SetMultipleBlockBits(ArrayMemoryInfo* info, bool flag)
	{
		BitMapEntry* mapEntry = &BitMapEntryList[info->MemPoolListIndex];
		mapEntry->SetMultipleBits(info->StartPosition, flag, info->Size);
	}

private:
	vector<void*> MemoryPoolList; // 内存池列表 每个内存池可存储1024个object
	vector<BitMapEntry> BitMapEntryList;  // 位图列表，每个位图项描述一个内存池
	set<BitMapEntry*> FreeMapEntries;
	map<void*, ArrayMemoryInfo> ArrayMemoryList; // 每个数组对象的信息
};

BitMappedMemoryManager gBitMappedMemoryManager;
void* Complex3::operator new(size_t size)
{
	return gBitMappedMemoryManager.allocate(size);
}

void Complex3::operator delete(void* object)
{
	gBitMappedMemoryManager.free(object);
}

void* Complex3::operator new[](size_t size)
{
	return gBitMappedMemoryManager.allocate(size);
}

void Complex3::operator delete[](void* object)
{
	gBitMappedMemoryManager.free(object);
}

void test_BitMappedMemPool()
{
	Performance p("test_BitMappedMemPool");
	Complex3* a1 = new Complex3;
	//Complex3* b = new Complex3[10];
	//Complex3* c = new Complex3[20];
	delete a1;
	//delete[] b;
	//delete[] c;
	Complex3* array[1000];
	for (int i = 0; i < 5000; i++)
	{
		for (int j = 0; j < 1000; j++)
		{
			array[j] = new Complex3(i, j);
		}
		for (int j = 0; j < 1000; j++)
		{
			delete array[j];
		}
	}
}

// Create a general-purpose memory manager for variable-size allocation
int main()
{
	test_Normal();
	test_MemPool();
	test_BitMappedMemPool();
	return 0;
}

