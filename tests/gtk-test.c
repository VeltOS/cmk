#include <gtk/gtk.h>
#include "../src/cmk-gtk.h"

static const CmkNamedColor Colors[] = {
	{"foreground", {1,  1, 1, 0.8}},
	{"background", {0.012, 0.363, 0.457, 1}},
	{"hover", {1,  1, 1, 0.2}},
	{"selected", {1,  1, 1, 0.1}},
	{NULL}
};

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	// Create window
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Cmk-Gtk Test");
	gtk_widget_set_size_request(window, 20, 20);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	//CmkPalette *pal = cmk_palette_get_base(0);
	//CmkColor col = {1, 0, 0, 1};
	//cmk_palette_set_color(pal, "foreground", &col);

	CmkPalette *pal2 = cmk_palette_get_primary(0);
	cmk_palette_set_colors(pal2, Colors);
	//cmk_palette_set_shade(pal2, 1.2);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_valign(vbox, GTK_ALIGN_FILL);
	gtk_widget_set_halign(vbox, GTK_ALIGN_FILL);
	gtk_widget_show(vbox);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(box);
	//gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

	// Add widget
	CmkLabel *label = cmk_label_new("abcdefghijklm nopqrstuvwxyz");
	PangoLayout *layout = cmk_label_get_layout(label);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	//gtk_box_pack_start(GTK_BOX(box
	gtk_box_set_center_widget(GTK_BOX(box), CMK_GTK(label));
	//gtk_container_add(GTK_CONTAINER(vbox), CMK_GTK(label));
	gtk_widget_set_valign(CMK_GTK(label), GTK_ALIGN_CENTER);
	gtk_widget_set_halign(CMK_GTK(label), GTK_ALIGN_CENTER);
	gtk_widget_show(CMK_GTK(label));

	// Button
	CmkButton *button = cmk_button_new_with_label(CMK_BUTTON_RAISED, "Install VeltOS");
	gtk_widget_set_valign(CMK_GTK(button), GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(box), CMK_GTK(button), TRUE, TRUE, 0);
	gtk_widget_show(CMK_GTK(button));

	// Icon
	CmkIcon *icon = cmk_icon_new_full("velt", NULL, 128, false);
	gtk_box_pack_start(GTK_BOX(box), CMK_GTK(icon), TRUE, TRUE, 0);
	gtk_widget_show(CMK_GTK(icon));

	//gtk_container_add(GTK_CONTAINER(vbox), box);
	gtk_box_set_center_widget(GTK_BOX(vbox), box);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show(window);

	//col.g = 1;
	//cmk_palette_set_color(pal, "foreground", &col);

	//pal = cmk_palette_get_base(g_type_from_name("CmkLabel"));
	//CmkColor col2 = {0, 1, 1, 1};
	//cmk_palette_set_color(pal, "foreground", &col2);

	gtk_main();
	return 0;
}
