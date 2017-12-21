#include <gtk/gtk.h>
#include "../../src/cmk.h"
#include "../../src/cmk-gtk.h"

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	// Create window
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Cmk-Gtk Test");
	gtk_widget_set_size_request(window, 640, 360);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Add widget
	CmkWidget *widget = cmk_widget_new();
	gtk_container_add(GTK_CONTAINER(window), CMK_GTK(widget));
	gtk_widget_show(CMK_GTK(widget));
	gtk_widget_set_size_request(CMK_GTK(widget), 100, 100);

	gtk_widget_show(window);

	gtk_main();
}
