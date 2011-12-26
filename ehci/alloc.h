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
#include "utils.h"
#include "memory.h"
#include "HW.h"

#ifndef _ALLOC_
#define _ALLOC_

#define _AHEAP_SIZE_TOTAL	0x2000
#define _AHEAP_INFO_SIZE	0x0100
#define _AHEAP_SIZE		_AHEAP_SIZE_TOTAL-_AHEAP_INFO_SIZE

typedef struct
{
	u8 *Offset;
	u32 Size;
} HeapInfoEntry;

void HeapInit( void );
void *malloc( u32 size );
void *malloca( u32 size, u32 align );
void free( void *ptr );

#endif
