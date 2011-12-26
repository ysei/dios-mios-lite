/*

SNEEK - SD-NAND/ES emulation kit for Nintendo Wii

Copyright (C) 2009-2010  crediar

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "alloc.h"
#include "vsprintf.h"

u8 *RAM;
HeapInfoEntry *HeapInfoEntries=NULL;

extern u32 DRAMRead( u32 a );
extern void DRAMWrite( u32 a, u32 b );

void HeapInit( void )
{
	RAM = (u8*)0xFFFFA000;
	HeapInfoEntries = (HeapInfoEntry*)(RAM+_AHEAP_SIZE);
	memset32( HeapInfoEntries, 0, _AHEAP_INFO_SIZE );

	while( HeapInfoEntries[0].Offset != 0 )
	{
		dbgprintf("Failed to clear memory!");
		Shutdown();
	}

	dbgprintf("Cleared 0x%04X bytes Space for %d allocs\n", _AHEAP_INFO_SIZE, _AHEAP_INFO_SIZE / 8 );	
}
void *malloc( u32 size )
{
	if( size == 0 )
		return NULL;
	if( size > _AHEAP_SIZE )
		return NULL;

	//align size to 32, easy cheat toallow all allocs to be aligned easily
	size = (size+0x1F) & (~0x1F);

	//find a free entry to be used
	u32 entry = 0xdeadbeef;
	u32 i;

	for( i=0; i < _AHEAP_INFO_SIZE / sizeof(HeapInfoEntry); ++i )
	{
		if( HeapInfoEntries[i].Offset == 0 )
		{
			entry = i;
			break;
		}
	}
	if( entry == 0xdeadbeef )
	{
		dbgprintf("run out of entries!\n");
		return NULL;
	}

	dbgprintf("Using entry:%d to alloc %d bytes...\n", entry, size );

	//Now we search a used entry
	u32 used_entry = 0xdeadbeef;

	for( i=0; i < _AHEAP_INFO_SIZE / sizeof(HeapInfoEntry); ++i )
	{
		if( HeapInfoEntries[i].Offset == 0 )
			continue;

		used_entry = i;
		break;
	}

	if( used_entry == 0xdeadbeef )
	{
		//dbgprintf("There are no other entries used atm\n");
		HeapInfoEntries[entry].Offset = RAM;
		HeapInfoEntries[entry].Size   = size;
		//dbgprintf("alloc1: ptr:%p size:%08X Entry:%d\n", HeapInfoEntries[entry].Offset, HeapInfoEntries[entry].Size, entry );
		return HeapInfoEntries[entry].Offset;
	}

find_space:
;
	//dbgprintf("[%02d]Offset:%08X Size:%08X\n", used_entry, HeapInfoEntries[used_entry].Offset, HeapInfoEntries[used_entry].Size );

	//now we search for the next closest and the previous closest entry
	u32 next	= 0xdeadbeef;
	u32 prev	= 0xdeadbeef;

	for( i=0; i < _AHEAP_INFO_SIZE / sizeof(HeapInfoEntry); ++i )
	{
		if( HeapInfoEntries[i].Offset == 0 )
			continue;
		if( used_entry == i )
			continue;

		if( next == 0xdeadbeef )
		{
			if( HeapInfoEntries[i].Offset > HeapInfoEntries[used_entry].Offset )
				next = i;
		} else {
			if( HeapInfoEntries[i].Offset < HeapInfoEntries[next].Offset && HeapInfoEntries[i].Offset > HeapInfoEntries[used_entry].Offset )
				next = i;
		}

		if( prev == 0xdeadbeef )
		{
			if( HeapInfoEntries[i].Offset < HeapInfoEntries[used_entry].Offset )
				prev = i;
		} else {
			if( HeapInfoEntries[i].Offset > HeapInfoEntries[prev].Offset && HeapInfoEntries[i].Offset < HeapInfoEntries[used_entry].Offset )
				prev = i;
		}
	}

	if( next == 0xdeadbeef )
	{
		//dbgprintf("This is the last entry\n");
	
		//check if there is engough space left for our alloc

		if( (u32)(HeapInfoEntries[used_entry].Offset-RAM) + HeapInfoEntries[used_entry].Size + size <= _AHEAP_SIZE )
		{
			HeapInfoEntries[entry].Offset = HeapInfoEntries[used_entry].Offset + HeapInfoEntries[used_entry].Size;
			HeapInfoEntries[entry].Size   = size;
			//dbgprintf("alloc2: ptr:%p size:%08X Entry:%d\n", HeapInfoEntries[entry].Offset, HeapInfoEntries[entry].Size, entry );
			return HeapInfoEntries[entry].Offset;
		}
		;//dbgprintf("2Not enough space left only had:%d\n", HEAP_SIZE - ((u32)(HeapInfoEntries[used_entry].Offset-RAM) + HeapInfoEntries[used_entry].Size) );
	} else if( (u32)(HeapInfoEntries[used_entry].Offset) + HeapInfoEntries[used_entry].Size + size < (u32)(HeapInfoEntries[next].Offset) )
	{
		HeapInfoEntries[entry].Offset = HeapInfoEntries[used_entry].Offset + HeapInfoEntries[used_entry].Size;
		HeapInfoEntries[entry].Size   = size;
		//dbgprintf("alloc4: ptr:%p size:%08X Entry:%d\n", HeapInfoEntries[entry].Offset, HeapInfoEntries[entry].Size, entry );
		return HeapInfoEntries[entry].Offset;
	} else {
		;//dbgprintf("4Not enough space left only had:%d %d:%d\n", (u32)( HeapInfoEntries[next].Offset - HeapInfoEntries[used_entry].Offset ) - HeapInfoEntries[used_entry].Size, next, used_entry );
	}

	if( prev == 0xdeadbeef )
	{
		//dbgprintf("This is the first entry\n");
		if( (u32)(HeapInfoEntries[used_entry].Offset-RAM) >= size )
		{
			HeapInfoEntries[entry].Offset = HeapInfoEntries[used_entry].Offset - size;
			HeapInfoEntries[entry].Size   = size;
			//dbgprintf("alloc3: ptr:%p size:%08X Entry:%d\n", HeapInfoEntries[entry].Offset, HeapInfoEntries[entry].Size, entry );
			return HeapInfoEntries[entry].Offset;
		}
		;//dbgprintf("3Not enough space left only had:%d\n", (u32)(HeapInfoEntries[used_entry].Offset-RAM) );
	} else if( (u32)(HeapInfoEntries[prev].Offset) + HeapInfoEntries[prev].Size + size < (u32)(HeapInfoEntries[used_entry].Offset) )
	{
		HeapInfoEntries[entry].Offset = HeapInfoEntries[prev].Offset + HeapInfoEntries[prev].Size;
		HeapInfoEntries[entry].Size   = size;
		//dbgprintf("alloc5: ptr:%p size:%08X Entry:%d\n", HeapInfoEntries[entry].Offset, HeapInfoEntries[entry].Size, entry );
		return HeapInfoEntries[entry].Offset;
	} else {
		;//dbgprintf("5Not enough space left only had:%d\n", (u32)(HeapInfoEntries[used_entry].Offset-HeapInfoEntries[prev].Offset) - HeapInfoEntries[prev].Size );
	}

	//if we land here we have to go to the next entry
	u32 temp = used_entry + 1;
	used_entry = 0xdeadbeef;

	for( i=temp; i < _AHEAP_INFO_SIZE / sizeof(HeapInfoEntry); ++i )
	{
		if( HeapInfoEntries[i].Offset == 0 )
			continue;

		used_entry = i;
		break;
	}

	if( used_entry != 0xdeadbeef )
		goto find_space;

	dbgprintf("failed to alloc %d bytes\n", size  );

	return NULL;
}
void *malloca( u32 size, u32 align )
{
	return malloc( size );
}
void free( void *ptr )
{
	if( ptr == NULL )
		return;

	u32 i;
	for( i=0; i < _AHEAP_INFO_SIZE / sizeof(HeapInfoEntry); ++i )
	{
		if( HeapInfoEntries[i].Offset == ptr )
		{
			dbgprintf("free:   ptr:%p size:%08X Entry:%d\n", HeapInfoEntries[i].Offset, HeapInfoEntries[i].Size, i );
			HeapInfoEntries[i].Offset = NULL;
			HeapInfoEntries[i].Size = 0;
			ptr = NULL;
			return;
		}
	}
}