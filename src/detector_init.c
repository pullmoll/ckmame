/*
  $NiH$

  detector_init.c -- initialize detector structures
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stddef.h>

#include "detector.h"



void detector_rule_init(detector_rule_t *dr)
{
    dr->start_offset = 0;
    dr->end_offset = 0;
    dr->operation = DETECTOR_OP_NONE;
    dr->tests = array_new(sizeof(detector_test_t));
}



void detector_test_init(detector_test_t *dt)
{
    dt->type = DETECTOR_TEST_DATA;
    dt->offset = 0;
    dt->length = 0;
    dt->value = NULL;
    dt->mask = NULL;
    dt->result = true;
}