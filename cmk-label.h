/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 *
 * Just a ClutterText but bold and using CmkWidget-style font colors.
 */

#ifndef __CMK_LABEL_H__
#define __CMK_LABEL_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_LABEL cmk_label_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkLabel, cmk_label, CMK, LABEL, CmkWidget);

typedef struct _CmkLabelClass CmkLabelClass;

struct _CmkLabelClass
{
	CmkWidgetClass parentClass;
};

CmkLabel * cmk_label_new(void);
CmkLabel * cmk_label_new_with_text(const gchar *text);

void cmk_label_set_text(CmkLabel *label, const gchar *text);
const gchar * cmk_label_get_text(CmkLabel *label);

G_END_DECLS

#endif
