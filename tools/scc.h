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
 * Definitions related to strongly connected components (SCCs)
 *
 * (c) 2004-2005 Tomi Janhunen
 */

#define _SCC_H_RCSFILE  "$RCSfile: scc.h,v $"
#define _SCC_H_DATE     "$Date: 2021/05/27 08:23:45 $"
#define _SCC_H_REVISION "$Revision: 1.5 $"

extern void _version_scc_c();

/* Defining rules */

typedef struct occurrences {
  int rule_cnt;         /* Number of rules */
  RULE **rules;         /* First rule */
  int scc;              /* Number of the strongly connected component */
  int scc_size;         /* Size of the srongly connected component */
  int visited;          /* For Tarjan's algorithm */
  int status;           /* Status bits */
  int other;            /* Corresponding atom in the other program */
} OCCURRENCES;

/* Occurrence tables (analogous to atom tables) */

typedef struct occtab {
  int count;                /* Number of atoms */
  int offset;               /* Index = atom number - offset */
  OCCURRENCES *ashead;      /* Rules having this atom as head */
  struct occtab *next;      /* Next piece (if any) */
  ATAB *atoms;              /* Respective atom table */
} OCCTAB;

extern OCCTAB *initialize_occurrences(ATAB *table);
extern OCCTAB *append_occurrences(OCCTAB *table, OCCTAB *occurrences);
extern void compute_occurrences(RULE *program, OCCTAB *occtab, int prune);
extern OCCURRENCES *find_occurrences(OCCTAB *occtab, int atom);
extern void compute_sccs(OCCTAB *occtable, int max_atom, int control);
extern void compute_joint_sccs(OCCTAB *occtab, int max_atom);
extern int is_stratifiable(OCCTAB *occtab);
