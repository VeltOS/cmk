/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_SEPARATOR_H__
#define __CMK_SEPARATOR_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_SEPARATOR cmk_separator_get_type()
G_DECLARE_FINAL_TYPE(CmkSeparator, cmk_separator, CMK, SEPARATOR, CmkWidget);

/**
 * SECTION:cmk-separator
 * @TITLE: CmkSeparator
 * @SHORT_DESCRIPTION: A separator widget
 */

/**
 * cmk_separator_new_h:
 *
 * New horizontal separator.
 */
CmkWidget * cmk_separator_new_h();

/**
 * cmk_separator_new_v:
 *
 * New vertical separator.
 */
CmkWidget * cmk_separator_new_v();

G_END_DECLS

#endif
