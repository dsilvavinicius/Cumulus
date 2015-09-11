#ifndef BITMAP_MEMORY_POOL_H
#define BITMAP_MEMORY_POOL_H

#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <bitset>

using namespace std;

namespace model
{
	const int BIT_MAP_SIZE = 1024;
	const int INT_SIZE = sizeof( int ) * 8;
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

	class BitMapMemoryManager;
	
	template< typename T >
	class BitMapMemoryPool;
	
	template< typename T >
	class BitMapEntry
	{
	public:
		BitMapEntry()
		: BlocksAvailable( BIT_MAP_SIZE )
		{
			memset( BitMap, 0xff, BIT_MAP_SIZE / sizeof( char ) ); 
			// initially all blocks are free and bit value 1 in the map denotes 
			// available block
		}
		void SetBit( int position, bool flag );
		void SetMultipleBits( int position, bool flag, int count );
		void SetRangeOfInt( int* element, int msb, int lsb, bool flag );
		T* FirstFreeBlock( size_t size );
		T* objectAddress( int pos );
		void* Head();
	
		int Index;
		int BlocksAvailable;
		int BitMap[ BIT_MAP_ELEMENTS ];
	};

	typedef struct ArrayInfo
	{
		int   MemPoolListIndex;
		int   StartPosition;
		int   Size;
	}
	ArrayMemoryInfo;
	
	template< typename T >
	void BitMapEntry< T >::SetBit( int position, bool flag )
	{
		BlocksAvailable += flag ? 1 : -1;
		int elementNo = position / INT_SIZE;
		int bitNo = position % INT_SIZE;
		if( flag )
			BitMap[ elementNo ] = BitMap[ elementNo ] | ( 1 << bitNo );
		else
			BitMap[ elementNo ] = BitMap[ elementNo ] & ~( 1 << bitNo ); 
	}

	template< typename T >
	void BitMapEntry< T >::SetMultipleBits( int position, bool flag, int count )
	{
		BlocksAvailable += flag ? count : -count;
		int elementNo = position / INT_SIZE;
		int bitNo = position % INT_SIZE;

		int bitSize = ( count <= INT_SIZE - bitNo ) ? count : INT_SIZE - bitNo;  
		SetRangeOfInt( &BitMap[ elementNo ], bitNo + bitSize - 1, bitNo, flag );
		count -= bitSize;
		if( !count ) return;
		
		int i = ++elementNo;
		while( count >= 0 )
		{
			if( count <= INT_SIZE )
			{
				SetRangeOfInt( &BitMap[ i ], count - 1, 0, flag );
				return;
			}
			else 
				BitMap[ i ] = flag ? unsigned ( -1 ) : 0;
			count -= 32; 
			i++;
		}
	}

	template< typename T >
	void BitMapEntry< T >::SetRangeOfInt( int* element, int msb, int lsb, bool flag )
	{
		if( flag )
		{
			int mask = ( unsigned( -1 ) << lsb ) & ( unsigned( -1 ) >> INT_SIZE - msb - 1 );
			*element |= mask;
		}
		else 
		{
			int mask = ( unsigned( -1 ) << lsb ) & ( unsigned( -1 ) >> INT_SIZE - msb - 1 );
			*element &= ~mask;
		}
	}

	template< typename T >
	T* BitMapEntry< T >::FirstFreeBlock( size_t size )
	{
		for( int i = 0 ; i < BIT_MAP_ELEMENTS; ++i )
		{
			if( BitMap[ i ] == 0 )
				continue;            // no free bit was found 
			
			int result = BitMap[ i ] & -( BitMap[ i ] ); // this expression yields the first 
			// bit position which is 1 in an int from right.
			void* address = 0;
			int basePos = ( INT_SIZE * i );
			switch( result )
			{
				//make the corresponding bit 0 meaning block is no longer free
				case 0x00000001: return objectAddress( basePos + 0 );
				case 0x00000002: return objectAddress( basePos + 1 );
				case 0x00000004: return objectAddress( basePos + 2 );
				case 0x00000008: return objectAddress( basePos + 3 );
				case 0x00000010: return objectAddress( basePos + 4 );
				case 0x00000020: return objectAddress( basePos + 5 );
				case 0x00000040: return objectAddress( basePos + 6 );
				case 0x00000080: return objectAddress( basePos + 7 );
				case 0x00000100: return objectAddress( basePos + 8 );
				case 0x00000200: return objectAddress( basePos + 9 );
				case 0x00000400: return objectAddress( basePos + 10 );
				case 0x00000800: return objectAddress( basePos + 11 );
				case 0x00001000: return objectAddress( basePos + 12 );
				case 0x00002000: return objectAddress( basePos + 13 );
				case 0x00004000: return objectAddress( basePos + 14 );
				case 0x00008000: return objectAddress( basePos + 15 );
				case 0x00010000: return objectAddress( basePos + 16 );
				case 0x00020000: return objectAddress( basePos + 17 );
				case 0x00040000: return objectAddress( basePos + 18 );
				case 0x00080000: return objectAddress( basePos + 19 );
				case 0x00100000: return objectAddress( basePos + 20 );
				case 0x00200000: return objectAddress( basePos + 21 );
				case 0x00400000: return objectAddress( basePos + 22 );
				case 0x00800000: return objectAddress( basePos + 23 );
				case 0x01000000: return objectAddress( basePos + 24 );
				case 0x02000000: return objectAddress( basePos + 25 );
				case 0x04000000: return objectAddress( basePos + 26 );
				case 0x08000000: return objectAddress( basePos + 27 );
				case 0x10000000: return objectAddress( basePos + 28 );
				case 0x20000000: return objectAddress( basePos + 29 );
				case 0x40000000: return objectAddress( basePos + 30 );
				case 0x80000000: return objectAddress( basePos + 31 );
				default : break;      
			}
		}
		return 0;
	}

	template< typename T >
	T* BitMapEntry< T >::objectAddress( int pos )
	{
		SetBit( pos, false ); 
		return &( ( static_cast< T* >( Head() ) + ( pos / INT_SIZE ) )[ INT_SIZE - ( ( pos % INT_SIZE ) + 1 ) ] );
	} 

	template< typename T >
	void* BitMapEntry< T >::Head()
	{
		return static_cast< BitMapMemoryManager& >( BitMapMemoryManager::instance() ).getPool< T >()
				.GetMemoryPoolList()[ Index ];
	}
	
	template< typename T >
	class BitMapMemoryPool
	{
	public: 
		BitMapMemoryPool() {}
		~BitMapMemoryPool();
		void* allocate();
		void* allocateArray( const size_t& size );
		bool deallocate( void* p );
		vector< void* >& GetMemoryPoolList();
		
		/** Calculates how much memory blocks are currently used. */
		size_t usedBlocks() const;
		
		/** Calculates how much memory is currently used in this pool in bytes. */
		size_t memoryUsage() const;
		
	private:
		void* AllocateArrayMemory( size_t size );
		void* AllocateChunkAndInitBitMap();
		bool SetBlockBit( void* object, bool flag );
		void SetMultipleBlockBits( ArrayMemoryInfo* info, bool flag );
		
		vector< void* > MemoryPoolList;
		vector< BitMapEntry > BitMapEntryList;
		//the above two lists will maintain one-to-one correpondence and hence 
		//should be of same  size.
		set< BitMapEntry* > FreeMapEntries;
		map< void*, ArrayMemoryInfo > ArrayMemoryList;
	};
	
	template< typename T >
	BitMapMemoryPool< T >::~BitMapMemoryPool()
	{
		for( int i = 0; i < MemoryPoolList.size(); ++i )
		{
			delete MemoryPoolList[ i ];
		}
	}
	
	template< typename T >
	void* BitMapMemoryPool< T >::allocate()
	{
		size_t size = sizeof( T );
		std::set< BitMapEntry* >::iterator freeMapI = FreeMapEntries.begin();
		if( freeMapI != FreeMapEntries.end() )
		{
			BitMapEntry* mapEntry = *freeMapI;
			return mapEntry->FirstFreeBlock( size );
		}
		else
		{
			AllocateChunkAndInitBitMap();
			FreeMapEntries.insert( &( BitMapEntryList[ BitMapEntryList.size() - 1 ] ) );
			return BitMapEntryList[ BitMapEntryList.size() - 1 ].FirstFreeBlock( size );
		}
	}

	template< typename T >
	void* BitMapMemoryPool< T >::allocateArray( const size_t& size )
	{
		if( ArrayMemoryList.empty() )
		{
			return AllocateArrayMemory( size );
		}
		else 
		{
			std::map< void*, ArrayMemoryInfo >::iterator infoI = ArrayMemoryList.begin();
			std::map< void*, ArrayMemoryInfo >::iterator infoEndI = ArrayMemoryList.end();
			while( infoI != infoEndI )
			{
				ArrayMemoryInfo info = ( *infoI ).second;
				if( info.StartPosition != 0 )  // search only in those mem blocks  
					continue;             // where allocation is done from first byte
				else 
				{
					BitMapEntry* entry = &BitMapEntryList[ info.MemPoolListIndex ];
					if( entry->BlocksAvailable < ( size / sizeof( T ) ) ) 
						return AllocateArrayMemory( size );
					else 
					{
						info.StartPosition = BIT_MAP_SIZE - entry->BlocksAvailable;
						info.Size = size / sizeof( T );
						T* baseAddress = static_cast< T* >( MemoryPoolList[ info.MemPoolListIndex ] ) +
											info.StartPosition;

						ArrayMemoryList[ baseAddress ] = info;
						SetMultipleBlockBits( &info, false );
			
						return baseAddress;
					} 
				}
			}
		}
	}
	
	template< typename T >
	void* BitMapMemoryPool< T >::AllocateArrayMemory( size_t size )
	{
		void* chunkAddress = AllocateChunkAndInitBitMap();
		ArrayMemoryInfo info;
		info.MemPoolListIndex = MemoryPoolList.size() - 1;
		info.StartPosition = 0;
		info.Size = size / sizeof( T );
		ArrayMemoryList[ chunkAddress ] = info;
		SetMultipleBlockBits( &info, false );
		return chunkAddress;
	}

	template< typename T >
	void BitMapMemoryPool< T >::deallocate( void* object )
	{
		if( ArrayMemoryList.find( object ) == ArrayMemoryList.end() )
			SetBlockBit( object, true );         // simple block deletion 
		else 
		{//memory block deletion 
			ArrayMemoryInfo *info = &ArrayMemoryList[ object ];
			SetMultipleBlockBits( info, true );
		}
	}
	
	template< typename T >
	std::vector< void* >& BitMapMemoryPool< T >::GetMemoryPoolList()
	{ 
		return MemoryPoolList;
	}
	
	template< typename T >
	size_t BitMapMemoryPool< T >::usedBlocks()
	{
		size_t nUsedBlocks = ( BitMapEntryList.size() - FreeMapEntries.size() ) * BIT_MAP_SIZE;
		for( BitMapEntry* entry : FreeMapEntries )
		{
			nUsedBlocks += BIT_MAP_SIZE - entry->BlocksAvailable;
		}
		
		return nUsedBlocks;
	}
	
	template< typename T >
	size_t BitMapMemoryPool< T >::memoryUsage()
	{
		return usedBlocks() * sizeof( T );
	}
	
	template< typename T >
	void* BitMapMemoryPool< T >::AllocateChunkAndInitBitMap()
	{
		BitMapEntry mapEntry;
		T* memoryBeginAddress = reinterpret_cast< T* >( new char [ sizeof( T ) * BIT_MAP_SIZE ] );
		MemoryPoolList.push_back( memoryBeginAddress );
		mapEntry.Index = MemoryPoolList.size() - 1;
		BitMapEntryList.push_back( mapEntry );
		return memoryBeginAddress;
	}

	template< typename T >
	void BitMapMemoryPool< T >::SetBlockBit( void* object, bool flag )
	{
		int i = BitMapEntryList.size() - 1;
		for( ; i >= 0 ; i-- )
		{
			BitMapEntry* bitMap = &BitMapEntryList[ i ];
			if( ( bitMap->Head() <= object ) && 
				( &( static_cast< T* >( bitMap->Head() ) )[ BIT_MAP_SIZE-1 ] >= object ) )
			{
				int position = static_cast< T* >( object ) - static_cast< T* >( bitMap->Head() );
				bitMap->SetBit( position, flag );
				flag ? bitMap->BlocksAvailable++ : bitMap->BlocksAvailable--;
			}
		}
	}

	template< typename T >
	void BitMapMemoryPool< T >::SetMultipleBlockBits( ArrayMemoryInfo* info, bool flag )
	{
		BitMapEntry* mapEntry = &BitMapEntryList[ info->MemPoolListIndex ];
		mapEntry->SetMultipleBits( info->StartPosition, flag, info->Size );
	}
}

#endif