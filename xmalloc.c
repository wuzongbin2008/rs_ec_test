/*
  Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
  This file is part of Rozofs.

  Rozofs is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation, version 2.

  Rozofs is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "xmalloc.h"
#include "log.h"

void *xmalloc(size_t n) {
    void *p = 0;

    p = malloc(n);
    check_memory(p);
    return p;
}

void *xcalloc(size_t n, size_t s) {
    void *p = 0;

    p = calloc(n, s);
    check_memory(p);
    return p;
}

void *xrealloc(void *p, size_t n) {
    p = realloc(p, n);
    check_memory(p);
    return p;
}

char *xstrdup(const char *str) {
    char *p;

    p = strdup(str);
    check_memory(p);
    return p;
}
