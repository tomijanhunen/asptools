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
 * PLANAR -- Generating random planar graphs using Stanford Graph Base
 *
 * (c) 2002 Tomi Janhunen
 *
 * Driver program
 */

/*
 * NOTE: Stanford Graph Base must be properly installed on your system
 *       (Package sgb in the Debian Linux distribution)!
 */

#include <gb_graph.h>
#include <gb_plane.h>
#include <gb_save.h>

void print_vertex(Vertex *v)
{
  Arc *a = v->arcs;

  v->u.I = atoi(v->name);

  while(a) {
    Vertex *v2 = a->tip;
    printf("arc(%s,%s).\n", v->name, v2->name);
    a = a->next;
  }

  a = v->arcs;

  while(a) {
    Vertex *v2 = a->tip;
    if(v2->u.I != atoi(v2->name))
      print_vertex(v2);
    a = a->next;
  }

  return;
}

int main(int argc, char **argv)
{
  Graph *g = NULL;
  int n = 0;
  int s = 0;

  if(argc == 3) {
    n = atoi(argv[1]);
    s = atoi(argv[2]);
    if(n<2) {
       fprintf(stderr, "%s: the number of vertices must exceed 2!\n", argv[0]);
      exit(-1);
    }
    g = plane(n, 0, 0, 0, 0, s);    
    
    if(g) {
      int j = 0;
      for(j=0; j<n; j++)
	printf("vertex(%i).\n", j);
      print_vertex(g->vertices);
    }

    exit(0);
  } else {
    fprintf(stderr, "usage: %s: <number of vertices> <seed>\n", argv[0]);
    exit(-1);
  }
}

