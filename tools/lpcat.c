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
 * LPCAT -- Linker for ground logic programs in SMODELS format
 *
 * (c) 2002-2010 Tomi Janhunen
 *
 * Main function for LPCAT
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "version.h"
#include "symbol.h"
#include "atom.h"
#include "rule.h"
#include "io.h"
#include "scc.h"
#include "relocate.h"

void _version_lpcat_c()
{
  fprintf(stderr, "%s: version information:\n", program_name);
  _version("$RCSfile: lpcat.c,v $",
	   "$Date: 2021/05/27 08:50:44 $",
	   "$Revision: 1.27 $");
  _version_symbol_c();
  _version_atom_c();
  _version_rule_c();
  _version_input_c();
  _version_output_c();
  _version_scc_c();
  _version_relocate_c();
}

void usage()
{
  fprintf(stderr, "\nusage:");
  fprintf(stderr, "   lpcat <options> [-f <file>] <file> ... \n\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "   -h or --help -- print help message\n");
  fprintf(stderr, "   --version -- print version information\n");
  fprintf(stderr, "   -v -- verbose mode (human readable)\n");
  fprintf(stderr, "   -c -- collect the entire program in memory\n");
  fprintf(stderr, "   -f -- read file names from a file\n");
  fprintf(stderr, "   -r -- read modules recursively until EOF\n");
  fprintf(stderr, "   -m -- check module conditions\n");
  fprintf(stderr, "         (also SCCs are checked if -c is given)\n");
  fprintf(stderr, "   -i -- mark input atoms (having no defining rules)\n");
  fprintf(stderr, "   -a=<number>\n");
  fprintf(stderr, "      -- set the first possible atom number\n");
  fprintf(stderr, "   -s=<symbol file>\n");
  fprintf(stderr, "      -- print a dummy program with symbol names\n");
  fprintf(stderr, "\n");

  return;
}

void spit_program(int style, FILE *out, RULE *program, ATAB *table);
void transfer_status_bits(ATAB *table1, ATAB *table2);
void reset_input_atoms(ATAB *table);

int main(int argc, char **argv)
{
  char **files = (char **)malloc(argc*sizeof(char *));
  int *ismeta = (int *)malloc(argc*sizeof(int));
  int fcnt = 0, i = 0;
  FILE *in = NULL;

  RULE *program1 = NULL;
  ATAB *table1 = NULL;
  int size1 = 0;
  int number1 = 0;
  int module = 0;

  RULE *program2 = NULL;
  ATAB *table2 = NULL;
  int size2 = 0;
  OCCTAB *occtab2 = NULL;
  int number2 = 1;

  char *file = NULL;
  char *metafile = NULL;
  char *symfile = NULL;

  FILE *meta = NULL;
  FILE *sym = NULL;
  FILE *out = stdout;

  int doubly_defined = 0;

  int option_help = 0;
  int option_version = 0;
  int option_verbose = 0;
  int option_collect = 0;
  int option_recursive = 0;
  int option_modular = 0;
  int option_mark_input = 0;
  int option_symbols = 0;

  char *arg = NULL;
  int which = 0;
  int error = 0;

  program_name = argv[0];

  /* Process options */

  for(which=1; which<argc; which++) {
    arg = argv[which];

    if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
      option_help = -1;
    else if(strcmp(arg, "--version") == 0)
      option_version = -1;
    else if(strcmp(arg, "-v") == 0)
      option_verbose = -1;
    else if(strcmp(arg, "-c") == 0)
      option_collect = -1;
    else if(strcmp(arg, "-r") == 0)
      option_recursive = -1;
    else if(strcmp(arg, "-f") == 0) {
      which++;
      if(which<argc) {
	arg = argv[which];
	files[fcnt] = arg;
	ismeta[fcnt] = -1;
	fcnt++;
      } else {
        fprintf(stderr, "%s: missing file name for -f\n", program_name);
        error = -1;
      }
    }
    else if(strcmp(arg, "-m") == 0)
      option_modular = -1;
    else if(strcmp(arg, "-i") == 0)
      option_mark_input = -1;
    else if(strncmp(arg, "-a=", 3) == 0) {
      size2 = atoi(&arg[3]) - 1;          /* The corresponding offset */
      if(size2 < 0) {
        fprintf(stderr, "%s: the first atom number should be positive\n",
		program_name);
	error = -1;
      }
    } else if(strncmp(arg, "-s=", 3) == 0) {
      option_symbols = -1;
      symfile = &arg[3];
    } else if(strncmp(arg, "-", 1) == 0 && strlen(arg)>1) {
      fprintf(stderr, "%s: unknown option %s\n", program_name, arg);
      error = -1;
    } else {
      files[fcnt] = arg;
      ismeta[fcnt] = 0;
      fcnt++;
    }
  }

  if(option_help) usage();
  if(option_version) _version_lpcat_c();

  if(option_help || option_version)
    exit(0);

  /* Check compatibility of options */

  if(option_mark_input && !option_modular) {
    fprintf(stderr, "%s: option -i presumes option -m!\n", program_name);
    exit(-1);
  }

  if(fcnt == 0) {
    files[fcnt] = "-";
    ismeta[fcnt] = 0;
    fcnt++;
  }

  if(error) {
    usage();
    exit(-1);
  }

  if(option_symbols) {
    if((sym = fopen(symfile, "w")) == NULL) {
      fprintf(stderr, "%s: cannot open file %s for writing symbols\n",
	      program_name, symfile);
      exit(-1);
    }
  }

  if(option_verbose && !option_collect) {
    fprintf(out, "%% Rules:\n");
    fprintf(out, "\n");
  }

  /* Read in logic programs or modules one by one as program1;
     the result of the concatenation accumulates as program2 */

  while(i<fcnt) {

    if(!option_recursive || in == NULL) {

      if(ismeta[i]) {
	if(!meta) {
	  metafile = files[i];

	  if(strcmp("-", metafile) == 0)
	    meta = stdin;
	  else if((meta = fopen(metafile, "r")) == NULL) {
	    fprintf(stderr, "%s: cannot open file %s\n",
		    program_name, metafile);
	    exit(-1);
	  }
	}
	if((file = read_string(meta)) == NULL) {
	  fprintf(stderr, "%s: no filename/newline found\n", program_name);
	  exit(-1);
	} else {
	  fscanf(meta, "\n");
	}
	if(option_verbose)
	  fprintf(out, "%% consulting file '%s'\n", file);

	if(feof(meta)) {
	  meta = NULL;
	  metafile = NULL;
	}
      } else
	file = files[i];

      if(strcmp("-", file) == 0)
	in = stdin;
      else if((in = fopen(file, "r")) == NULL) {
	fprintf(stderr, "%s: cannot open file %s\n", program_name, file);
	exit(-1);
      }
    }
    program1 = read_program(in);
    table1 = read_symbols(in);
    number1 = read_compute_statement(in, table1);

    /* Close the input file for not to have too many open files */

    if(!option_recursive && in != stdin) {
      fclose(in); in = NULL;
    }

   if(option_mark_input)
      /* Atoms having no defining atoms are marked as input atoms */
     mark_io_atoms(program1, table1, ++module);

    /* Calculate cross-references from table1 to table2 */

    initialize_other_tables(table1, table2);
    doubly_defined = combine_atom_tables(table1, table2, 0, 0, option_modular);

    if(doubly_defined) {
      if(!option_verbose) {
	out = stderr;
	fprintf(out, "%s: module error: ", program_name);
      } else {
	fprintf(out, "%s: warning: ", program_name);
      }
      write_atom(STYLE_READABLE, out, doubly_defined, table1);
      fprintf(out, " is defined by several modules!\n");
      /* The given programs do not form proper modules */
      if(!option_verbose)
	exit(-1);
    }

    /* Relocate/compress the symbol table and relocate rules */

    if(table1 && table1->next)
      table1 = make_contiguous(table1);  /* Assumed by relocation procedures */

    mark_visible(table1);
    mark_occurrences(program1, table1);

    size1 = reloc_symbol_table(table1, size2) - size2;

    if(option_collect)
      reloc_program(program1, table1);
    else {
      /* Write rules immediately and free the memory */

      if(option_verbose)
	spit_program(STYLE_READABLE, out, program1, table1);
      else
	spit_program(STYLE_SMODELS, out, program1, table1);

      free_program(program1);
      program1 = NULL;
    }

    transfer_status_bits(table1, table2); /* MARK_TRUE/FALSE/HEADOCC */

    if(size1>0) {
      /* Append table1 after table2 */

      table1 = compress_symbol_table(table1, size1, size2);
      attach_atoms_to_names(table1);
      table2 = append_table(table2, table1);
      table1 = NULL;

      size2 += size1;

    } else {
      /* Get disposed of table1 */

      free(table1->names);
      free(table1->statuses);
      free(table1->others);
      free(table1);

      table1 = NULL;
    }

    size1 = 0;

    if(option_collect) {
      program2 = append_rules(program2, program1);
      program1 = NULL;
    }

    number2 *= number1;
    number1 = 0;

    /* Proceed to the next program/module */

    if(option_recursive) {
      if(feof(in)) {
	fclose(in);
	in = NULL;
	if(!ismeta[i] || !meta)
	  i++;
      }
    } else if(!ismeta[i] || !meta)
      i++;
  }

  /* Check module conditions */

  if(option_modular && option_collect) {
    /* Form the dependency graph */
    occtab2 = initialize_occurrences(table2);
    compute_occurrences(program2, occtab2, 0);

    /* Calculate strongly connected components and check module conditions */
    compute_joint_sccs(occtab2, size2);
  }

  /* Print the result of concatenation */

  if(option_verbose) {

    if(option_collect) {
      fprintf(out, "\n%% Rules:\n");
      fprintf(out, "\n");
      if(table2 && table2->next)
	table2 = make_contiguous(table2);
      write_program(STYLE_READABLE, out, program2, table2);
    }
    fprintf(out, "\n");

    fprintf(out, "compute { ");
    write_compute_statement(STYLE_READABLE, out, table2, MARK_TRUE_OR_FALSE);
    fprintf(out, " }.\n\n");

    write_input(STYLE_READABLE, out, table2);

    fprintf(out, "%% Symbols:\n\n");
    write_symbols(STYLE_READABLE, out, table2);
    fprintf(out, "\n");

  } else { /* !option_verbose */

    if(option_collect) {
      if(table2 && table2->next)
	table2 = make_contiguous(table2);
      write_program(STYLE_SMODELS, out, program2, table2);
    }
    fprintf(out, "0\n");

    write_symbols(STYLE_SMODELS, out, table2);
    fprintf(out, "0\n");

    fprintf(out, "B+\n");
    write_compute_statement(STYLE_SMODELS, out, table2, MARK_TRUE);
    fprintf(out, "0\n");

    fprintf(out, "B-\n");
    write_compute_statement(STYLE_SMODELS, out, table2, MARK_FALSE);
    fprintf(out, "0\n");

    if(!option_mark_input)
      reset_input_atoms(table2);
    fprintf(out, "E\n");
    write_compute_statement(STYLE_SMODELS, out, table2, MARK_INPUT);
    fprintf(out, "0\n");

    fprintf(out, "%i\n", number2);

    if(option_symbols) {
      /* Create a dummy program containing only symbol names */

      fprintf(sym, "0\n");
      write_symbols(STYLE_SMODELS, sym, table2);
      fprintf(sym, "0\n");
      fprintf(sym, "B+\n");
      fprintf(sym, "0\n");
      fprintf(sym, "B-\n");
      fprintf(sym, "0\n");
      fprintf(sym, "0\n");
    }
  }

  exit(0);
}

/* ------------------------- Local output routines ------------------------- */

void spit_atom(int style, FILE *out, int atom, ATAB *table)
{
  int offset = table->offset;
  int count = table->count;
  int shift = table->shift;
  int *others = table->others;
  int atom2 = others[atom-offset];

  if(style == STYLE_SMODELS)
    fprintf(out, " %i", atom2+shift);
  else {
    SYMBOL **symbols = table->names;
    SYMBOL *sym = symbols[atom-offset];

    if(sym)
      write_name(out, sym, table->prefix, table->postfix);
    else
      fprintf(out, "_%i", atom2+shift);
  }

  return;
}

void spit_literal_list(int style, FILE *out, char *separator,
		       int pos_cnt, int *pos,
		       int neg_cnt, int *neg,
		       int *weight, ATAB *table)
{
  int *scan = NULL;
  int *last = NULL;
  int *wscan = weight;
  int *wlast = &weight[pos_cnt+neg_cnt];

  for(scan = neg, last = &neg[neg_cnt];
      scan != last; ) {
    if(style == STYLE_READABLE)
      fprintf(out, "not ");
    spit_atom(style, out, *scan, table);
    if(wscan && (style == STYLE_READABLE))
      fprintf(out, "=%i", *(wscan++));
    scan++;
    if(style == STYLE_READABLE)
      if(scan != last || pos_cnt)
	fprintf(out, "%s", separator);
  }

  for(scan = pos, last = &pos[pos_cnt];
      scan != last; ) {
    spit_atom(style, out, *scan, table);
    if(wscan && (style == STYLE_READABLE))
      fprintf(out, "=%i", *(wscan++));
    scan++;
    if(style == STYLE_READABLE)
      if(scan != last)
	fprintf(out, "%s", separator);
  }

  if(wscan && (style == STYLE_SMODELS))
    while(wscan != wlast)
      fprintf(out, " %i", *(wscan++));

  return;
}

void spit_basic(int style, FILE *out, RULE *rule, ATAB *table)
{
  int *scan = NULL;
  int *last = NULL;
  int pos_cnt = 0;
  int neg_cnt = 0;

  BASIC_RULE *basic = rule->data.basic;

  if(style == STYLE_SMODELS)
    fprintf(out, "1");

  spit_atom(style, out, basic->head, table);

  pos_cnt = basic->pos_cnt;
  neg_cnt = basic->neg_cnt;

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i", (pos_cnt+neg_cnt), neg_cnt);

  if(pos_cnt || neg_cnt) {
    if(style == STYLE_READABLE)
      fprintf(out, " :- ");

    spit_literal_list(style, out, ", ",
		       pos_cnt, basic->pos,
		       neg_cnt, basic->neg,
		       NULL, table);
  }

  if(style == STYLE_READABLE)
    fprintf(out, ".");
  fprintf(out, "\n");

  return;
}

void spit_constraint(int style, FILE *out, RULE *rule, ATAB *table)
{
  int pos_cnt = 0;
  int neg_cnt = 0;
  int bound = 0;

  CONSTRAINT_RULE *constraint = rule->data.constraint;

  if(style == STYLE_SMODELS)
    fprintf(out, "2");

  spit_atom(style, out, constraint->head, table);

  pos_cnt = constraint->pos_cnt;
  neg_cnt = constraint->neg_cnt;
  bound = constraint->bound;

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i %i", (pos_cnt+neg_cnt), neg_cnt, bound);

  if(style == STYLE_READABLE)
    fprintf(out, " :- %i {", bound);

  if(pos_cnt || neg_cnt)
    spit_literal_list(style, out, ", ",
		       pos_cnt, constraint->pos,
		       neg_cnt, constraint->neg,
		       NULL, table);

  if(style == STYLE_READABLE)
    fprintf(out, "}.");

  fprintf(out, "\n");

  return;
}

void spit_choice(int style, FILE *out, RULE *rule, ATAB *table)
{
  int head_cnt = 0;
  int pos_cnt = 0;
  int neg_cnt = 0;
  char *separator = ", ";

  CHOICE_RULE *choice = rule->data.choice;
  head_cnt = choice->head_cnt;
  pos_cnt = choice->pos_cnt;
  neg_cnt = choice->neg_cnt;

  if(style == STYLE_SMODELS)
    fprintf(out, "3 %i", head_cnt);
  else if(style == STYLE_READABLE)
    fprintf(out, "{");

  spit_literal_list(style, out, separator,
		     head_cnt, choice->head,
		     0, NULL,
		     NULL, table);

  if(style == STYLE_READABLE)
    fprintf(out, "}");

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i", (pos_cnt+neg_cnt), neg_cnt);

  if(pos_cnt || neg_cnt) {
    if(style == STYLE_READABLE)
      fprintf(out, " :- ");

    spit_literal_list(style, out, ", ",
		       pos_cnt, choice->pos,
		       neg_cnt, choice->neg,
		       NULL, table);
  }

  if(style == STYLE_READABLE)
    fprintf(out, ".");
  fprintf(out, "\n");

  return;
}

void spit_integrity(int style, FILE *out, RULE *rule, ATAB *table)
{
  int *scan = NULL;
  int *last = NULL;
  int pos_cnt = 0;
  int neg_cnt = 0;

  INTEGRITY_RULE *integrity = rule->data.integrity;

  if(style == STYLE_SMODELS)
    fprintf(out, "4");

  pos_cnt = integrity->pos_cnt;
  neg_cnt = integrity->neg_cnt;

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i", (pos_cnt+neg_cnt), neg_cnt);

  if(pos_cnt || neg_cnt) {
    if(style == STYLE_READABLE)
      fprintf(out, " :- ");

    spit_literal_list(style, out, ", ",
		       pos_cnt, integrity->pos,
		       neg_cnt, integrity->neg,
		       NULL, table);
  }

  if(style == STYLE_READABLE)
    fprintf(out, ".");
  fprintf(out, "\n");

  return;
}

void spit_weight(int style, FILE *out, RULE *rule, ATAB *table)
{
  int pos_cnt = 0;
  int neg_cnt = 0;
  int bound = 0;

  WEIGHT_RULE *weight = rule->data.weight;

  if(style == STYLE_SMODELS)
    fprintf(out, "5");

  spit_atom(style, out, weight->head, table);

  pos_cnt = weight->pos_cnt;
  neg_cnt = weight->neg_cnt;
  bound = weight->bound;

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i %i", bound, (pos_cnt+neg_cnt), neg_cnt);

  if(style == STYLE_READABLE)
    fprintf(out, " :- %i [", bound);

  if(pos_cnt || neg_cnt)
    spit_literal_list(style, out, ", ",
		       pos_cnt, weight->pos,
		       neg_cnt, weight->neg,
		       weight->weight, table);

  if(style == STYLE_READABLE)
    fprintf(out, "].");
  fprintf(out, "\n");

  return;
}

void spit_optimize(int style, FILE *out, RULE *rule, ATAB *table)
{
  int pos_cnt = 0;
  int neg_cnt = 0;

  OPTIMIZE_RULE *optimize = rule->data.optimize;

  if(style == STYLE_SMODELS)
    fprintf(out, "6 0");

  pos_cnt = optimize->pos_cnt;
  neg_cnt = optimize->neg_cnt;

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i", (pos_cnt+neg_cnt), neg_cnt);

  if(pos_cnt || neg_cnt) {
    if(style == STYLE_READABLE)
      fprintf(out, "minimize [");

    spit_literal_list(style, out, ", ",
		       pos_cnt, optimize->pos,
		       neg_cnt, optimize->neg,
		       optimize->weight, table);
  }

  if(style == STYLE_READABLE)
    fprintf(out, "].");
  fprintf(out, "\n");

  return;
}

void spit_disjunctive(int style, FILE *out, RULE *rule, ATAB *table)
{
  int head_cnt = 0;
  int pos_cnt = 0;
  int neg_cnt = 0;
  char *separator = ", ";

  DISJUNCTIVE_RULE *disjunctive = rule->data.disjunctive;
  head_cnt = disjunctive->head_cnt;
  pos_cnt = disjunctive->pos_cnt;
  neg_cnt = disjunctive->neg_cnt;

  if(style == STYLE_SMODELS)
    fprintf(out, "8 %i", head_cnt);
  else if(style == STYLE_READABLE)
    fprintf(out, "{");

  spit_literal_list(style, out, separator,
		     head_cnt, disjunctive->head,
		     0, NULL,
		     NULL, table);

  if(style == STYLE_READABLE)
    fprintf(out, "}");

  if(style == STYLE_SMODELS)
    fprintf(out, " %i %i", (pos_cnt+neg_cnt), neg_cnt);

  if(pos_cnt || neg_cnt) {
    if(style == STYLE_READABLE)
      fprintf(out, " :- ");

    spit_literal_list(style, out, ", ",
		       pos_cnt, disjunctive->pos,
		       neg_cnt, disjunctive->neg,
		       NULL, table);
  }

  if(style == STYLE_READABLE)
    fprintf(out, ".");
  fprintf(out, "\n");

  return;
}

void spit_rule(int style, FILE *out, RULE *rule, ATAB *table)
{
  switch(rule->type) {
  case TYPE_BASIC:
    spit_basic(style, out, rule, table);
    break;

  case TYPE_CONSTRAINT:
    spit_constraint(style, out, rule, table);
    break;

  case TYPE_CHOICE:
    spit_choice(style, out, rule, table);
    break;

  case TYPE_INTEGRITY:
    spit_integrity(style, out, rule, table);
    break;

  case TYPE_WEIGHT:
    spit_weight(style, out, rule, table);
    break;

  case TYPE_OPTIMIZE:
    spit_optimize(style, out, rule, table);
    break;

  case TYPE_DISJUNCTIVE:
    spit_disjunctive(style, out, rule, table);
    break;

  default:
    error("unknown rule type");
  }
}

void spit_program(int style, FILE *out, RULE *rule, ATAB *table)
{
  if(style != STYLE_READABLE && style != STYLE_SMODELS) {
    fprintf(stderr, "%s: unknown style %i for spit_rule\n",
	    program_name, style);
    exit(-1);
  }

  if(!table || table->next) {
    fprintf(stderr,
	    "spit_rule: the first symbol table should be contiguous!\n");
    exit(-1);
  }

  while(rule) {
    spit_rule(style, out, rule, table);
    rule = rule->next;
  }
  return;
}

void transfer_status_bits(ATAB *table1, ATAB *table2)
{
  ATAB *scan = table1;
  int i = 0;

  /* Presumes a previous call to attach_atoms_to_names(table2) */

  while(scan) {
    int count = scan->count;
    int offset = scan->offset;
    SYMBOL **names = scan->names;

    for(i=1; i<=count; i++) {
      int atom = i+offset;
      SYMBOL *sym = names[i];

      if(sym) {   /* The atom has a symbolic name */

	ATAB *other = sym->info.table;    /* Relevant piece */
	int atom2 = sym->info.atom;
	
	if(atom2 && other) {
	  int j = atom2 - other->offset;  /* Calculate index */

	  (other->statuses)[j] |=
	    ((scan->statuses)[i] & (MARK_TRUE_OR_FALSE|MARK_HEADOCC));

	}
      }
    }
    scan = scan->next;
  }

  return;
}

void reset_input_atoms(ATAB *table)
{
  while(table) {
    int count = table->count;
    SYMBOL **names = table->names;
    int *statuses = table->statuses;
    int i = 0;

    for(i = 1; i <= count; i++)

      /* Reset the input status if defining rules exist */
      if(names[i] && (statuses[i] & MARK_HEADOCC))
	statuses[i] &= ~MARK_INPUT;

    table = table->next;
  }

  return;
}
