/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-palette
 * @TITLE: CmkPalette
 * @SHORT_DESCRIPTION: Color mangement
 *
 * CmkPalette stores a set of colors that a widget can use
 * to draw itself. There are three main palettes: the
 * base, primary, and secondary.
 *
 * The base palette should be used for "plain" areas of
 * an application. For example, the app's main background
 * color.
 *
 * The primary palette should be used for main widgets
 * that the user might interact with: buttons, switches,
 * etc. Also, it can be used for header / menu bars.
 *
 * The secondary palette should be used for widgets
 * resting on top of primary-colored areas, such as floating
 * circle buttons, or other special widgets which should
 * get extra attention.
 *
 * See the Material design page on Colors for more info:
 * https://material.io/guidelines/style/color.html
 *
 * The cmk_palette_get_* methods support a default palette
 * for each type (using 0 for the type), but also a palette
 * for specific widget classes. These specific versions
 * will inherit any unset colors from the default palette.
 *
 * Widget implementations should select a palette to use
 * automatically, but that choice can be overridden using
 * the cmk_widget_set_palette() method.
 *
 * Any changes to a palette's colors, including changes
 * to inherited values, are notified through the CmkPalette::change
 * signal. CmkWidget automatically listens to this signal
 * and invalidates the widget (and emits its own notify::palette
 * signal) on a change.
 */

#ifndef __CMK_STYLE_H__
#define __CMK_STYLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct
{
	float r, g, b, a;
} CmkColor;

typedef struct
{
	char *name;
	CmkColor color;
} CmkNamedColor;

#define CMK_TYPE_PALETTE cmk_palette_get_type()
G_DECLARE_FINAL_TYPE(CmkPalette, cmk_palette, CMK, PALETTE, GObject);

/**
 * cmk_palette_get_base:
 * @type: The widget class type, or 0 for a default.
 *
 * Gets the base color palette for type. The base
 * palette should be used for standard text on
 * standard background (usually black on white).
 *
 * If type is non-zero, it will inherit from the
 * default base type.
 *
 * Returns: (transfer none): The palette.
 */
CmkPalette * cmk_palette_get_base(GType type);

/**
 * cmk_palette_get_primary:
 * @type: The widget class type, or 0 for a default.
 *
 * Gets the primary color palette for type. The primary
 * palette should be used for headers, most buttons, and
 * other widgets.
 *
 * If type is non-zero, it will inherit from the
 * default base type.
 *
 * Returns: (transfer none): The palette.
 */
CmkPalette * cmk_palette_get_primary(GType type);

/**
 * cmk_palette_get_secondary:
 * @type: The widget class type, or 0 for a default.
 *
 * Gets the secondary color palette for type. The
 * secondary palette should be used for some buttons,
 * namely the raised circle button, and other
 * case-by-case uses.
 *
 * If type is non-zero, it will inherit from the
 * default base type.
 *
 * Returns: (transfer none): The palette.
 */
CmkPalette * cmk_palette_get_secondary(GType type);


/**
 * cmk_palette_new:
 * @inherit: Inherit from this, or NULL.
 *
 * Creates a new palette which optionally inherits
 * from the given palette.
 *
 * Returns: (transfer full): The palette.
 */
CmkPalette * cmk_palette_new(CmkPalette *inherit);

/**
 * cmk_palette_set_color:
 *
 * Sets a named color on the palette. Can set color to
 * NULL to inherit.
 */
void cmk_palette_set_color(CmkPalette *palette, const char *name, const CmkColor *color);

/**
 * cmk_palette_set_colors:
 *
 * Set multiple colors at once. Good for initialization.
 * The color array must end with a NULL-named color.
 */
void cmk_palette_set_colors(CmkPalette *palette, const CmkNamedColor *colors);

/**
 * cmk_palette_get_color:
 *
 * Gets a named color, or NULL if it doesn't exist. The colors
 * "background", "foreground", and "alert" are guaranteed to
 * exist (and set to white, black, and red, respectively, if
 * they are not set otherwise).
 */
const CmkColor * cmk_palette_get_color(CmkPalette *palette, const char *name);

/**
 * cmk_palette_set_shade:
 *
 * Sets the shading multiplier. This is used to created accents
 * of a color in the palette, generally for effects such as
 * mouse-over.
 *
 * For light themes where shading is darker, this should be
 * less than 1. For dark themes where the shading is lighter,
 * this should be greater than 1.
 *
 * Set to a negative value to inherit.
 */
void cmk_palette_set_shade(CmkPalette *palette, float shade);

/**
 * cmk_palette_get_shade:
 *
 * Gets the palette's shade multiplier.
 */
float cmk_palette_get_shade(CmkPalette *palette);

/**
 * cmk_palette_shade:
 *
 * Applies the shade multiplier to the input color, and
 * writes the result to out.
 */
void cmk_palette_shade(CmkPalette *palette, const CmkColor *color, CmkColor *out);

G_END_DECLS

#endif
