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

#ifndef _XMALLOC_H
#define _XMALLOC_H

#include <stdlib.h>

#include "log.h"

#define check_memory(p) if (p == 0) {\
    fatal("null pointer detected -- exiting.");\
}

void *xmalloc(size_t n);

void *xcalloc(size_t n, size_t s);

void *xrealloc(void *p, size_t n);

char *xstrdup(const char *p);

#endif
