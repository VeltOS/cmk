#include <gtk/gtk.h>
#include "../src/cmk-gtk.h"

static const CmkNamedColor Colors[] = {
	{"background", {73,  86, 92, 255}},
	{"foreground", {255, 255, 255, 204}},
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

	CmkPalette *pal2 = cmk_palette_get_base(0);
	cmk_palette_set_colors(pal2, Colors);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_valign(vbox, GTK_ALIGN_FILL);
	gtk_widget_set_halign(vbox, GTK_ALIGN_FILL);
	gtk_widget_show(vbox);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(box);
	//gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

	// Add widget
	CmkLabel *button = cmk_label_new("abcdefghijklm nopqrstuvwxyz");
	PangoLayout *layout = cmk_label_get_layout(button);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	//gtk_box_pack_start(GTK_BOX(box
	gtk_box_set_center_widget(GTK_BOX(box), CMK_GTK(button));
	//gtk_container_add(GTK_CONTAINER(vbox), CMK_GTK(button));
	gtk_widget_set_valign(CMK_GTK(button), GTK_ALIGN_CENTER);
	gtk_widget_set_halign(CMK_GTK(button), GTK_ALIGN_CENTER);
	gtk_widget_show(CMK_GTK(button));

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
