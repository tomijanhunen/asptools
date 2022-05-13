/* asptools -- Tool collection for answer set programming

   Copyright (C) 2022 Tomi Janhunen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 * Relocation of logic program modules
 *
 * (c) 2007 Tomi Janhunen
 */

/* Version information */

#define _RELOCATE_H_RCSFILE  "$RCSfile: relocate.h,v $"
#define _RELOCATE_H_DATE     "$Date: 2021/05/27 08:50:18 $"
#define _RELOCATE_H_REVISION "$Revision: 1.2 $"

extern void _version_relocate_c();

/* Utilities */

extern int reloc_symbol_table(ATAB *table, int shift);
extern ATAB *compress_symbol_table(ATAB *table, int size, int shift);
extern void reloc_program(RULE *program, ATAB *table);

