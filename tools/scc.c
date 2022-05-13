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
 * scc.c -- Computation of Strongly Connected Components
 *          Stratifiability check (for rules defining invisible atoms)
 *
 * (c) 2004-2021 Tomi Janhunen
 */

#include <stdlib.h>
#include <stdio.h>

#include "version.h"
#include "symbol.h"
#include "atom.h"
#include "rule.h"
#include "io.h"
#include "scc.h"

void _version_scc_c()
{
  _version("$RCSfile: scc.c,v $",
	   "$Date: 2021/05/27 08:28:26 $",
	   "$Revision: 1.13 $");
}



/* ------- Form the occurrence table corresponding to an atom table  ------- */

OCCURRENCES *find_occurrences(OCCTAB *occtab, int atom)
{
  while(occtab) {
    int count = occtab->count;
    int offset = occtab->offset;

    if(atom > offset && atom <= offset+count)
      return &(occtab->ashead)[atom-offset];

    occtab = occtab->next;
  }
  return NULL;
}

OCCTAB *initialize_occurrences(ATAB *table)
{
  OCCTAB *rvalue = (OCCTAB *)malloc(sizeof(OCCTAB));
  OCCTAB *occtab = rvalue;
  
  while(table) {
    int count = table->count;
    int offset = table->offset;
    SYMBOL **names = table->names;
    int *statuses = table->statuses;
    int *others = table->others;
    OCCURRENCES *ashead = 
      (OCCURRENCES *)malloc(sizeof(OCCURRENCES)*(count+1));
    int i = 0;

    occtab->count = count;
    occtab->offset = offset;
    occtab->ashead = ashead;
    occtab->atoms = table;

    for(i=1; i<=count; i++) {
      OCCURRENCES *h = &ashead[i];

      h->rule_cnt = 0;
      h->rules = NULL;
      h->scc = 0;
      h->scc_size = 0;
      h->visited = 0;
      h->status = (statuses[i] & MARK_INPUT);
      if(names[i])
	h->status |= MARK_VISIBLE;
      if(others)
	h->other = others[i];
      else
	h->other = 0;
    }

    /* Process the next piece (if any) */

    if(table = table->next) {
      occtab->next = (OCCTAB *)malloc(sizeof(OCCTAB));
      occtab = occtab->next;
    } else
      occtab->next = NULL;
  }

  return rvalue;
}

OCCTAB *append_occurrences(OCCTAB *table, OCCTAB *occurrences)
{
  OCCTAB *scan = table;

  if(scan) {
    while(scan->next) scan=scan->next;

    scan->next = occurrences;
  } else
    table = occurrences;

  return table;
}

void compute_occurrences(RULE *program, OCCTAB *occtab, int prune)
{
  RULE *scan = program;
  OCCTAB *pass = occtab;

  /* ------ Count head occurrences of atoms ------ */

  while(scan) {
    int type = scan->type;
    int head = 0;
    
    switch(type) {
    case TYPE_BASIC:
      { BASIC_RULE *basic = scan->data.basic;
        head = basic->head;
      }
      break;

    case TYPE_CONSTRAINT:
      { CONSTRAINT_RULE *constraint = scan->data.constraint;
        head = constraint->head;
      }
      break;

    case TYPE_CHOICE:
      { int *first = NULL;
        int *last = NULL;
        CHOICE_RULE *choice = scan->data.choice;

	for(first = choice->head, last = &first[choice->head_cnt];
	    first != last; first++) {
	  head = *first;
	  OCCURRENCES *h = find_occurrences(occtab, head);

	  if(!(h->status & prune))
	    h->rule_cnt++;
	}
      }
      break;

    case TYPE_WEIGHT:
      { WEIGHT_RULE *weight = scan->data.weight;
        head = weight->head;
      }
      break;

    case TYPE_OPTIMIZE:
      head = 0;
      break;

    case TYPE_DISJUNCTIVE:
      { int *first = NULL;
        int *last = NULL;
        DISJUNCTIVE_RULE *disjunctive = scan->data.disjunctive;

	for(first = disjunctive->head, last = &first[disjunctive->head_cnt];
	    first != last; first++) {
	  head = *first;
	  OCCURRENCES *h = find_occurrences(occtab, head);

	  if(!(h->status & prune))
	    h->rule_cnt++;
	}
      }
      break;

    default:
      fprintf(stderr, "compute_occurrences: unsupported rule type %i!\n",
	      type);
      exit(-1);
    }
    if(head) {
      OCCURRENCES *h = find_occurrences(occtab, head);

      if(!(h->status & prune))
	h->rule_cnt++;
    }

    scan = scan->next;
  }

  /* ------ Allocate memory for pointers referring rules ------ */

  while(pass) {
    int count = pass->count;
    OCCURRENCES *ashead = pass->ashead;
    int i = 0;

    for(i=1; i<=count; i++) {
      OCCURRENCES *h = &ashead[i];

      if(h->rule_cnt) {
	h->rules = (RULE **)malloc(sizeof(RULE *)*(h->rule_cnt));
	h->rule_cnt = 0;
      }
    }

    pass = pass->next;
  }

  /* ----- Collect occurrences of atoms as heads ------ */

  scan = program;

  while(scan) {
    int type = scan->type;
    int head = 0;
    
    switch(type) {
    case TYPE_BASIC:
      { BASIC_RULE *basic = scan->data.basic;
        head = basic->head;
      }
      break;

    case TYPE_CONSTRAINT:
      { CONSTRAINT_RULE *constraint = scan->data.constraint;
        head = constraint->head;
      }
      break;

    case TYPE_CHOICE:
      { int *first = NULL;
        int *last = NULL;
        CHOICE_RULE *choice = scan->data.choice;
      
	for(first = choice->head, last = &first[choice->head_cnt];
	    first != last; first++) {
	  head = *first;
	  OCCURRENCES *h = find_occurrences(occtab, head);

	  if(!(h->status & prune))
	    h->rules[h->rule_cnt++] = scan;
	}
      }
      break;

    case TYPE_WEIGHT:
      { WEIGHT_RULE *weight = scan->data.weight;
        head = weight->head;
      }
      break;

    case TYPE_OPTIMIZE:
      head = 0;
      break;

    case TYPE_DISJUNCTIVE:
      { int *first = NULL;
        int *last = NULL;
        DISJUNCTIVE_RULE *disjunctive = scan->data.disjunctive;
      
	for(first = disjunctive->head, last = &first[disjunctive->head_cnt];
	    first != last; first++) {
	  head = *first;
	  OCCURRENCES *h = find_occurrences(occtab, head);

	  if(!(h->status & prune))
	    h->rules[h->rule_cnt++] = scan;
	}
      }
      break;

    default:
      fprintf(stderr, "compute_occurrences: unsupported rule type %i!\n",
	      type);
      exit(-1);
    }
    if(head) {
      OCCURRENCES *h = find_occurrences(occtab, head);

      if(!(h->status & prune))
	h->rules[h->rule_cnt++] = scan;
    }

    scan = scan->next;
  }

  return;
}

int count_on(ASTACK *stack, int atom)
{
  int rvalue = 0;

  if(stack) {
    int atom2 = stack->atom;

    while(stack && atom != atom2) {
      rvalue++;
      if(stack = stack->under)
	atom2 = stack->atom;
    }
  }

  return rvalue;
}

int different_modules(int atom1, int atom2, ATAB *table)
{
  SYMBOL *symbol1 = find_name(table, atom1);
  SYMBOL *symbol2 = find_name(table, atom2);

  if(symbol1 && symbol2) {
    int module1 = symbol1->info.module;
    int module2 = symbol2->info.module;

    if(module1 && module2 && (module1 != module2))
      return -1;
  }

  return 0;
}

/* ------ Adopting Sedgewick's representation of Tarjan's algorithm: ------- */

int visit(int atom, int *next, int max_atom, ASTACK **stack,
	  OCCTAB *occtab, int control);

void visit_list(int *first, int cnt, int *next, int *min, int max_atom,
		int mark, ASTACK **stack, OCCTAB *occtab, int control)
{
  int i = 0;

  for(i=0; i<cnt; i++) {
    int atom = first[i];
    OCCURRENCES *h = find_occurrences(occtab, atom);
    int rvalue = h->visited;

    /* Visit invisible/all atoms */

    if(!(h->status & (MARK_VISIBLE & control))) {

      h->status |= mark;

      if(rvalue == 0)
	rvalue = visit(atom, next, max_atom, stack, occtab, control);

      if(rvalue < *min) *min = rvalue;
    }
  }

  return;
}

int visit(int atom, int *next, int max_atom, ASTACK **stack,
	  OCCTAB *occtab, int control)
{
  OCCURRENCES *h = find_occurrences(occtab, atom);
  int min = ++(*next);
  int i = 0;

  h->visited = min;

  *stack = push(atom, 0, NULL, *stack);

  /* Traverse atoms which this one depends */

  for(i=0; i < h->rule_cnt; i++) {
    RULE *r = h->rules[i];
    int *first = NULL;
    int cnt = 0;

    /* Positive dependencies */

    if(control & MARK_POSOCC) {
      first = get_pos(r);
      cnt = get_pos_cnt(r);
      if(cnt)
	visit_list(first, cnt, next, &min, max_atom, MARK_POSOCC,
		   stack, occtab, control);
    }

    /* Negative dependencies */

    if(control & MARK_NEGOCC) {
      first = get_neg(r);
      cnt = get_neg_cnt(r);
      if(cnt)
	visit_list(first, cnt, next, &min, max_atom, MARK_NEGOCC,
		   stack, occtab, control);
    }
  }

  /* Unwind a SCC from the stack */

  if(h->visited == min) {
    int size = count_on(*stack, atom)+1;
    int atom2 = 0;

    *stack = pop(&atom2, NULL, NULL, *stack);

    h->scc = min;
    h->scc_size = size;
    h->visited = max_atom+1;

    while(atom2 != atom) {
      OCCURRENCES *h2 = find_occurrences(occtab, atom2);

      h2->scc = min;
      h2->scc_size = size;
      h2->visited = max_atom+1;

      *stack = pop(&atom2, NULL, NULL, *stack);
    }
  }

  return min;
}

void compute_sccs(OCCTAB *occtab, int max_atom, int control)
{
  int next = 0;           /* Next free component number */
  ASTACK *stack = NULL;   /* Global stack to be used by visit */

  /* Visit all atoms found in the reference table */

  while(occtab) {
    int count = occtab->count;
    int offset = occtab->offset;
    int i = 0;

    for(i=1; i<=count; i++) {
      int atom = i+offset;
      OCCURRENCES *h = &(occtab->ashead)[i];

      /* Visit invisible/all atoms */
      
      if(!(h->status & (MARK_VISIBLE & control)) && h->visited == 0)
	visit(atom, &next, max_atom, &stack, occtab, control);
    }

    occtab = occtab->next;
  }

  return;
}

/* ------------- Check stratifiability of the invisible part -------------- */

int in_scc(int scc, int cnt, int *first, OCCTAB *occtab)
{
  int *scan = first;
  int *last = &first[cnt];

  while(scan != last) {
    OCCURRENCES *b = find_occurrences(occtab, *scan);

    if(!(b->status & MARK_VISIBLE)) {

      if(b) {
	if(scc == b->scc)
	  return -1;
      } else {
	fprintf(stderr, "in_scc: missing reference for atom %i!\n", *scan);
	exit(-1);
      }

    }
    scan++;
  }

  return 0;
}

int is_stratifiable(OCCTAB *occtab)
{
  OCCTAB *scan = occtab;
  int rvalue = -1;

  /* Process all atoms one by one */

  while(scan) {
    int count = scan->count;
    int offset = scan->offset;
    int i = 0;

    for(i=1; rvalue && i<=count; i++) {
      int atom = i+offset;
      OCCURRENCES *h = &(scan->ashead)[i];
      int scc = h->scc;
      int scc_size = h->scc_size;
      int j = 0;

      /* Skip all visible atoms */

      if(h->status & MARK_VISIBLE)
	continue;

       /* Process rules that define this invisible atom; check for
          dependencies wrt. the negative literals based on invisible atoms */

      for(j=0; rvalue && j<h->rule_cnt; j++) {
	RULE *r = h->rules[j];

	if(r->type == TYPE_CHOICE)
	  rvalue = 0;
	else
	  if(in_scc(scc, get_neg_cnt(r), get_neg(r), occtab))
	    rvalue = 0;

      }
    }

    scan = scan->next;
  }

  return rvalue;
}

/* ---- Analysis of joint positive dependencies (for module conditions) --- */

int pos_visit(int atom, int *next, int max_atom,
	      ASTACK **stack, OCCTAB *occtab);

void pos_visit_list(int *first, int cnt, int *next,
		    int *min, int max_atom, ASTACK **stack,
		    OCCTAB *occtab)
{
  int i = 0;

  for(i=0; i<cnt; i++) {
    int atom = first[i];
    OCCURRENCES *h = find_occurrences(occtab, atom);
    int rvalue = h->visited;

    if(rvalue == 0)
	rvalue = pos_visit(atom, next, max_atom, stack, occtab);

    if(rvalue < *min) *min = rvalue;
  }

  return;
}

int pos_visit(int atom, int *next, int max_atom,
	      ASTACK **stack, OCCTAB *occtab)
{
  OCCURRENCES *h = find_occurrences(occtab, atom);
  int min = ++(*next);
  int i = 0;
  int fail = 0;            /* A component extends to several modules */
  ASTACK *failing = NULL;  /* Local stack for unwinding */

  h->visited = min;

  *stack = push(atom, 0, NULL, *stack);

  /* Traverse atoms on which this one depends positively */

  for(i=0; i < h->rule_cnt; i++) {
    RULE *r = h->rules[i];
    int *first = NULL;
    int pos_cnt = 0;

    first = get_pos(r);
    pos_cnt = get_pos_cnt(r);
    if(pos_cnt)
      pos_visit_list(first, pos_cnt, next, &min, max_atom, stack, occtab);
  }

  /* Unwind a SCC from the stack */

  if(h->visited == min) {
    int size = count_on(*stack, atom)+1;
    int atom2 = 0;

    *stack = pop(&atom2, NULL, NULL, *stack);
    failing = push(atom2, 0, NULL, failing);

    h->scc = min;
    h->scc_size = size;
    h->visited = max_atom+1;

    while(atom2 != atom) {
      OCCURRENCES *h2 = find_occurrences(occtab, atom2);

      h2->scc = min;
      h2->scc_size = size;
      h2->visited = max_atom+1;

      /* Fail if atoms originate from different modules;
         the atom table is partitioned according to atoms */

      if(different_modules(atom, atom2, occtab->atoms)) fail = -1;

      *stack = pop(&atom2, NULL, NULL, *stack);
      failing = push(atom2, 0, NULL, failing);
    }

    if(fail) {
      fprintf(stderr, "%s: module error: ", program_name);
      fprintf(stderr, "positively interdependent atoms: ");

      while(failing) { /* Unwind and print */
	failing = pop(&atom2, NULL, NULL, failing);
	write_atom(STYLE_READABLE, stderr, atom2, occtab->atoms);
	if(failing)
	  fputc(' ', stderr);
      }
      fprintf(stderr, "!\n");

      exit(-1);

    } else { /* Unwind and forget */
      while(failing)
	failing = pop(&atom2, NULL, NULL, failing);
    }
  }

  return min;
}

void compute_joint_sccs(OCCTAB *occtab, int max_atom)
{
  int next = 0;           /* Next free component number */
  ASTACK *stack = NULL;   /* Global stack to be used by visit */
  OCCTAB *scan = NULL;

  /* Visit all atoms found in the reference table */

  scan = occtab;

  while(scan) {
    int count = scan->count;
    int offset = scan->offset;
    int i = 0;

    for(i=1; i<=count; i++) {
      int atom = i+offset;
      OCCURRENCES *h = &(scan->ashead)[i];

      if(h->visited == 0)
	pos_visit(atom, &next, max_atom, &stack, occtab);
    }

    scan = scan->next;
  }

  return;
}


