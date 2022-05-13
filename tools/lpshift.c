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
 * LPSHIFT -- Shift disjunctions in a disjunctive logic program
 *
 * (c) 2014 Tomi Janhunen
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "version.h"
#include "symbol.h"
#include "atom.h"
#include "rule.h"
#include "io.h"
#include "scc.h"

void _version_lpshift_c()
{
  fprintf(stderr, "%s: version information:\n", program_name);
  _version("$RCSfile: lpshift.c,v $",
           "$Date: 2021/05/27 09:24:36 $",
           "$Revision: 1.5 $");
  _version_atom_c();
  _version_rule_c();
  _version_input_c();
  _version_output_c();
}

void usage()
{
  fprintf(stderr, "\nusage:");
  fprintf(stderr, "   lpshift <options> <file>\n\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "   -h or --help -- print help message\n");
  fprintf(stderr, "   --version    -- print version information\n");
  fprintf(stderr, "   -f           -- forced shift (SCCs neglected)\n");
  fprintf(stderr, "   --bc         -- force body compression\n");
  fprintf(stderr, "   --nb         -- no body compression\n");
  fprintf(stderr, "   -v           -- verbose (human readable) output\n");
  fprintf(stderr, "\n");

  return;
}

int shift_rule(int style, FILE *out, RULE *rule,
	       int no_bc, int force_bc, int force,
	       ATAB *table, OCCTAB *occtab, int newatom, int verbose);

void transform_into_basic(int style, FILE *out, RULE *rule, ATAB *table);

int main(int argc, char **argv)
{
  char *file = NULL;
  FILE *in = NULL;
  RULE *program = NULL;
  RULE *rule = NULL;
  ATAB *table = NULL;
  OCCTAB *occtab = NULL;
  int newatom = 0;
  int size = 0;

  FILE *out = stdout;

  char *arg = NULL;
  int which = 0; 

  int option_help = 0;
  int option_version = 0;
  int option_force = 0;
  int option_verbose = 0;
  int option_force_bodyc = 0;
  int option_no_bodyc = 0;

  program_name = argv[0];

  for(which=1; which<argc; which++) {
    arg = argv[which];

    if((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
      option_help = -1;
    else if(strcmp(arg, "--version") == 0)
      option_version = 1;
    else if(strcmp(arg, "-f") == 0)
      option_force = 1;
    else if(strcmp(arg, "--bc") == 0)
      option_force_bodyc = 1;
    else if(strcmp(arg, "--nb") == 0)
      option_no_bodyc = 1;
    else if(strcmp(arg, "-v") == 0)
      option_verbose = 1;
    else if(file == NULL)
      file = arg;
    else {
      fprintf(stderr, "%s: unknown argument %s\n", program_name, arg);
      usage();
      exit(-1);
    }
  }

  if(option_help) usage();
  if(option_version) _version_lpshift_c();

  if(option_help || option_version)
    exit(0);

  if(option_no_bodyc && option_force_bodyc) {
    fprintf(stderr, "%s: options --bc and --nb are incompatible!\n",
	    program_name);
    exit(-1);
  }

  if(file == NULL || strcmp("-", file) == 0) {
    in = stdin;
  } else {
    if((in = fopen(file, "r")) == NULL) {
      fprintf(stderr, "%s: cannot open file %s\n", program_name, file);
      exit(-1);
    }
  }
  
  program = read_program(in);
  table = read_symbols(in);
  read_compute_statement(in, table);

  size = table_size(table);
  newatom = size+1;    

  /* Calculate the strongly connected components */

  if(!option_force) {
    occtab = initialize_occurrences(table);
    compute_occurrences(program, occtab, 0);
    compute_sccs(occtab, size, MARK_POSOCC);
  }

  /* Shift atoms from the heads of disjunctive rules as far as possible */

  if(option_verbose) {
    rule = program;

    while(rule) {
      if(rule->type == TYPE_DISJUNCTIVE) {
	int head_cnt = get_head_cnt(rule);

	if(head_cnt>1)
	  newatom =
	    shift_rule(STYLE_READABLE, out, rule,
		       option_no_bodyc, option_force_bodyc, option_force,
		       table, occtab, newatom, option_verbose);
	else
	  transform_into_basic(STYLE_READABLE, out, rule, table);
	  
      } else
        write_rule(STYLE_READABLE, out, rule, table);

      rule = rule->next;
    }
    fprintf(out, "\n");
    fprintf(out, "compute { ");
    write_compute_statement(STYLE_READABLE, out, table, MARK_TRUE|MARK_FALSE);
    fprintf(out, " }.\n\n");

    write_input(STYLE_READABLE, out, table);

  } else { /* !verbose_mode */
    rule = program;

    while(rule) {
      if(rule->type == TYPE_DISJUNCTIVE) {
	int head_cnt = get_head_cnt(rule);
	
	if(head_cnt>1) 
	  newatom =
	    shift_rule(STYLE_SMODELS, out, rule,
		       option_no_bodyc, option_force_bodyc, option_force,
		       table, occtab, newatom, option_verbose);
	else
	  transform_into_basic(STYLE_SMODELS, out, rule, table);
      } else
        write_rule(STYLE_SMODELS, out, rule, table);

      rule = rule->next;
    }
    fprintf(out, "0\n");

    write_symbols(STYLE_SMODELS, out, table);
    fprintf(out, "0\n");

    fprintf(out, "B+\n");
    write_compute_statement(STYLE_SMODELS, out, table, MARK_TRUE);
    fprintf(out, "0\n");

    fprintf(out, "B-\n");
    write_compute_statement(STYLE_SMODELS, out, table, MARK_FALSE);
    fprintf(out, "0\n");

    write_input(STYLE_SMODELS, out, table);

    fprintf(out, "%i\n", 0);
  }

  exit(0);
}

/* --------------------- Local transformation routines --------------------- */

int get_scc(int atom, OCCTAB *occtab)
{
  OCCURRENCES *b = find_occurrences(occtab, atom);

  if(b)
    return b->scc;
  else
    return 0;
}

int partition_head_by_sccs(int cnt, int *heads, OCCTAB *occtab)
{
  int i = 0;
  int scc_cnt = 0;

  for(i=0; i<cnt; i++) {
    int scc = get_scc(heads[i], occtab);
    int j = 0;

    scc_cnt++;

    for(j=i+1; j<cnt; j++)
      if(scc == get_scc(heads[j], occtab)) {
	i++;
	if(j>i) { /* We have to make a real move */
	  int head = heads[j];

	  heads[j] = heads[i];
	  heads[i] = head;
	}
      }
  }

  return scc_cnt;
}

/* Do shifting for rules that have at least two head atoms */

int shift_rule(int style, FILE *out, RULE *rule,
	       int no_bc, int force_bc, int force,
	       ATAB *table, OCCTAB *occtab, int newatom, int verbose)
{
  int *heads = get_heads(rule);
  int head_cnt = get_head_cnt(rule);
  int n = partition_head_by_sccs(head_cnt, heads, occtab);
  int scc = 0;
  int i = 0;
  RULE *shifted = NULL;
  DISJUNCTIVE_RULE *disjunctive = NULL;
  BASIC_RULE *basic = NULL;
  int joint_body = 0;

  if(!force)
    scc = get_scc(heads[0], occtab);

  if(verbose)
    fprintf(out, "%% A head (cnt=%i) shared by %i components:\n", head_cnt, n);

  /* Create a template of the shifted rule */

  shifted = (RULE *)malloc(sizeof(RULE));
  shifted->type = 0;
  shifted->next = NULL;

  disjunctive = (DISJUNCTIVE_RULE *)malloc(sizeof(DISJUNCTIVE_RULE));
  disjunctive-> neg = NULL;
  basic = (BASIC_RULE *)malloc(sizeof(BASIC_RULE));
  basic-> neg = NULL;

  if((!no_bc &&
      (n-1)*(get_pos_cnt(rule)+get_neg_cnt(rule)) > n+3)
     ||
     (force_bc && get_pos_cnt(rule)+get_neg_cnt(rule)>1))
  {
    RULE *jbody = (RULE *)malloc(sizeof(RULE));
    BASIC_RULE *joint = (BASIC_RULE *)malloc(sizeof(BASIC_RULE));

    extend_table(table, 1, newatom-1);
    joint_body = newatom++;

    jbody->type = TYPE_BASIC;
    jbody->data.basic = joint;
    jbody->next = NULL;

    joint->head = joint_body;
    joint->pos_cnt = get_pos_cnt(rule);
    joint->pos = get_pos(rule);
    joint->neg_cnt = get_neg_cnt(rule);
    joint->neg = get_neg(rule);

    write_rule(style, out, jbody, table);

    free(jbody);
    free(joint);
  }

  while(i<head_cnt) {
    int j = i;
    int new_head_cnt = 0;

    if(!force)
      while(j<head_cnt && scc == get_scc(heads[j], occtab)) j++;
    else
      j++;
    
    new_head_cnt = j-i;

    if(new_head_cnt == 1) {
      shifted->type = TYPE_BASIC;
      shifted->data.basic = basic;
      basic->head = heads[i];
    } else {
      shifted->type = TYPE_DISJUNCTIVE;
      shifted->data.disjunctive = disjunctive;
      disjunctive->head_cnt = new_head_cnt;
      disjunctive->head = &heads[i];
    }

    if(joint_body) {
      int new_cnt = head_cnt - new_head_cnt;
      int *new_neg = (int *)malloc(new_cnt*sizeof(int));
      int k = 0, l = 0;

      for(l=0; l<i; l++)
	  new_neg[k++] = heads[l];
      for(l=j; l<head_cnt; l++)
	  new_neg[k++] = heads[l];

      if(new_head_cnt == 1) {
	basic->pos_cnt = 1;
	basic->pos = &joint_body;
	basic->neg_cnt = new_cnt;
	basic->neg = new_neg;
      } else {
	disjunctive->pos_cnt = 1;
	disjunctive->pos = &joint_body;
	disjunctive->neg_cnt = new_cnt;
	disjunctive->neg = new_neg;
      }
    } else {
      int neg_cnt = get_neg_cnt(rule);
      int new_cnt = neg_cnt + head_cnt - (j-i);
      int *neg = get_neg(rule);
      int *new_neg = (int *)malloc(new_cnt*sizeof(int));
      int k = 0, l = 0;

      for(k=0; k<neg_cnt; k++)
	new_neg[k] = neg[k];
      for(l=0; l<i; l++)
	  new_neg[k++] = heads[l];
      for(l=j; l<head_cnt; l++)
	  new_neg[k++] = heads[l];

      if(new_head_cnt == 1) {
	basic->pos_cnt = get_pos_cnt(rule);
	basic->pos = get_pos(rule);
	basic->neg_cnt = new_cnt;
	basic->neg = new_neg;
      } else {
	disjunctive->pos_cnt = get_pos_cnt(rule);
	disjunctive->pos = get_pos(rule);
	disjunctive->neg_cnt = new_cnt;
	disjunctive->neg = new_neg;
      }
    }

    write_rule(style, out, shifted, table);

    if(new_head_cnt == 1)
      free(basic->neg);
    else
      free(disjunctive->neg);

    i=j;
    if(i<head_cnt) scc = get_scc(heads[i], occtab);
  }

  free(disjunctive);
  free(basic);
  free(shifted);

  return newatom;
}

/*
 * transform_into_basic -- Transform a single-headed disjunctive rule
 */

void transform_into_basic(int style, FILE *out, RULE *rule, ATAB *table)
{
  DISJUNCTIVE_RULE *disjunctive = rule->data.disjunctive;
  BASIC_RULE *basic = (BASIC_RULE *)malloc(sizeof(BASIC_RULE));
  RULE *new = (RULE *)malloc(sizeof(RULE));

  new->type = TYPE_BASIC;
  new->data.basic = basic;

  basic->head = disjunctive->head[0];
  basic->pos_cnt = disjunctive->pos_cnt;
  basic->pos = disjunctive->pos;
  basic->neg_cnt = disjunctive->neg_cnt;
  basic->neg = disjunctive->neg;

  write_rule(style, out, new, table);

  free(new);
  free(basic);

  return;
}
