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
 * (c) 2007-2010 Tomi Janhunen
 */

#include <stdlib.h>
#include <stdio.h>

#include "version.h"
#include "symbol.h"
#include "atom.h"
#include "rule.h"
#include "io.h"
#include "scc.h"
#include "relocate.h"

void _version_relocate_h()
{
  _version(_RELOCATE_H_RCSFILE, _RELOCATE_H_DATE, _RELOCATE_H_REVISION);
}

void _version_relocate_c()
{
  _version_relocate_h();
  _version("$RCSfile: relocate.c,v $",
	   "$Date: 2021/05/27 09:53:56 $",
	   "$Revision: 1.5 $");
}

/* ------------------ Relocate and compress atoms tables ------------------- */

int reloc_symbol_table(ATAB *table, int shift)
{
  int new = shift;
  int count = table->count;
  int *others = table->others;
  int i = 0;

  /* Renumber all local atoms, starting from shift+1,
     and store new atom numbers as others[i] */

  if(!table || table->next) {
    fprintf(stderr, "relocation error: contiguous symbol table expected!\n");
    exit(-1);
  }

  if(others)
    for(i=1; i<=count; i++)
      if(others[i]) { /* The atom appears in the previous modules */

	if(others[i]>shift) {
	  fprintf(stderr, "relocation error: too big cross-reference!\n");
	  exit(-1);
	}

      } else { /* Relocate */
	int status = table->statuses[i];
	
	if(status & (MARK_POSOCC_OR_NEGOCC | MARK_HEADOCC | MARK_VISIBLE))
	  others[i] = ++new;
      }

  return new;
}

ATAB *compress_symbol_table(ATAB *table, int size, int shift)
{
  ATAB *new = new_table(size, shift);
  int i = 1;

  /* This is a destructive operation that will create a new table */

  while(table) {
    ATAB *next = table->next;
    int count = table->count;
    int offset = table->offset;
    int j = 0;

    /* Go through atoms in this slice */

    for(j=1; j<=count; j++) { 
      int other = table->others[j];
      int atom = j+offset;

      if(!other) continue;  /* Deletes unused invisible atoms */

      if(other>shift) {

	/* Copy a local atom that has been relocated */

	new->names[i] = table->names[j];
	new->statuses[i] = table->statuses[j];
	if(i+shift != table->others[j]) {
	  fprintf(stderr, "%s: relocation error for _%i\n",
		  program_name, atom);
	  exit(-1);
	} else
	  i++;

      }
    }

    free(table->names);
    free(table->statuses);
    free(table->others);
    free(table);

    table = next;
  }

  return new;
}

/* ---------------------------- Relocate atoms ---------------------------- */

int reloc_atom(int atom, ATAB *table)
{
  int *others = table->others;
  int offset = table->offset;

  return others[atom-offset];
}

void reloc_atom_list(int cnt, int *atoms, ATAB *table)
{
  int i = 0;

  for(i = 0; i<cnt; i++)
    atoms[i] = reloc_atom(atoms[i], table);

  return;
}

/* ---------------------- Relocate rules and programs ---------------------- */

void reloc_basic(RULE *rule, ATAB *table)
{
  int cnt = 0;

  BASIC_RULE *basic = rule->data.basic;

  basic->head = reloc_atom(basic->head, table);

  if(cnt = basic->pos_cnt) reloc_atom_list(cnt, basic->pos, table);
  if(cnt = basic->neg_cnt) reloc_atom_list(cnt, basic->neg, table);

  return;
}

void reloc_constraint(RULE *rule, ATAB *table)
{
  int cnt = 0;

  CONSTRAINT_RULE *constraint = rule->data.constraint;

  constraint->head = reloc_atom(constraint->head, table);

  if(cnt = constraint->pos_cnt) reloc_atom_list(cnt, constraint->pos, table);
  if(cnt = constraint->neg_cnt) reloc_atom_list(cnt, constraint->neg, table);

  return;
}

void reloc_integrity(RULE *rule, ATAB *table)
{
  int cnt = 0;

  INTEGRITY_RULE *integrity = rule->data.integrity;

  if(cnt = integrity->pos_cnt) reloc_atom_list(cnt, integrity->pos, table);
  if(cnt = integrity->neg_cnt) reloc_atom_list(cnt, integrity->neg, table);

  return;
}

void reloc_choice(RULE *rule, ATAB *table)
{
  int cnt = 0;

  CHOICE_RULE *choice = rule->data.choice;

  if(cnt = choice->head_cnt) reloc_atom_list(cnt, choice->head, table);
  if(cnt = choice->pos_cnt) reloc_atom_list(cnt, choice->pos, table);
  if(cnt = choice->neg_cnt) reloc_atom_list(cnt, choice->neg, table);

  return;
}

void reloc_weight(RULE *rule, ATAB *table)
{
  int cnt = 0;

  WEIGHT_RULE *weight = rule->data.weight;

  weight->head = reloc_atom(weight->head, table);

  if(cnt = weight->pos_cnt) reloc_atom_list(cnt, weight->pos, table);
  if(cnt = weight->neg_cnt) reloc_atom_list(cnt, weight->neg, table);

  return;
}

void reloc_optimize(RULE *rule, ATAB *table)
{
  int cnt = 0;

  OPTIMIZE_RULE *optimize = rule->data.optimize;

  if(cnt = optimize->pos_cnt) reloc_atom_list(cnt, optimize->pos, table);
  if(cnt = optimize->neg_cnt) reloc_atom_list(cnt, optimize->neg, table);

  return;
}

void reloc_disjunctive(RULE *rule, ATAB *table)
{
  int cnt = 0;

  DISJUNCTIVE_RULE *disjunctive = rule->data.disjunctive;

  if(cnt = disjunctive->head_cnt)
    reloc_atom_list(cnt, disjunctive->head, table);
  if(cnt = disjunctive->pos_cnt) reloc_atom_list(cnt, disjunctive->pos, table);
  if(cnt = disjunctive->neg_cnt) reloc_atom_list(cnt, disjunctive->neg, table);

  return;
}

void reloc_clause(RULE *rule, ATAB *table)
{
  int cnt = 0;

  CLAUSE *clause = rule->data.clause;

  if(cnt = clause->pos_cnt) reloc_atom_list(cnt, clause->pos, table);
  if(cnt = clause->neg_cnt) reloc_atom_list(cnt, clause->neg, table);

  return;
}

void reloc_rule(RULE *rule, ATAB *table)
{
  switch(rule->type) {
  case TYPE_BASIC:
    reloc_basic(rule, table);
    break;

  case TYPE_CONSTRAINT:
    reloc_constraint(rule, table);
    break;

  case TYPE_INTEGRITY:
    reloc_integrity(rule, table);
    break;

  case TYPE_CHOICE:
    reloc_choice(rule, table);
    break;

  case TYPE_WEIGHT:
    reloc_weight(rule, table);
    break;

  case TYPE_OPTIMIZE:
    reloc_optimize(rule, table);
    break;

  case TYPE_DISJUNCTIVE:
    reloc_disjunctive(rule, table);
    break;

  case TYPE_CLAUSE:
    reloc_clause(rule, table);
    break;

  default:
    error("unknown rule type");
  }
}

void reloc_program(RULE *rule, ATAB *table)
{
  if(!table || table->next) {
    fprintf(stderr, "relocation error: contiguous symbol table expected!\n");
    exit(-1);
  }

  while(rule) {
    reloc_rule(rule, table);
    rule = rule->next;
  }
  return;
}
