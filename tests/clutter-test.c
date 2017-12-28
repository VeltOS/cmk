#include <clutter/clutter.h>
#include "../src/cmk-clutter.h"

int main(int argc, char **argv)
{
	if(clutter_init(&argc, &argv) != CLUTTER_INIT_SUCCESS)
		return 1;

	ClutterActor *stage = clutter_stage_new();
	clutter_stage_set_title(CLUTTER_STAGE(stage), "Cmk-Clutter Test");
	clutter_stage_set_user_resizable(CLUTTER_STAGE(stage), TRUE);
	g_signal_connect(stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);

	//CmkButton *button = cmk_button_new(CMK_BUTTON_FLAT);
	CmkLabel *button = cmk_label_new();
	clutter_actor_add_child(stage, CMK_CLUTTER(button));
	clutter_actor_set_position(CMK_CLUTTER(button), 100, 100);
	//clutter_actor_set_size(CMK_CLUTTER(button), 500, 500);

	PangoLayout *layout = cmk_label_get_layout(button);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	cmk_label_set_text(button, "The quick brown fox jumps over the lazy dog.");
	//clutter_actor_set_size(CMK_CLUTTER(button), 500, 500);

	ClutterColor c = {255, 0, 0, 100};
	clutter_actor_set_background_color(CMK_CLUTTER(button), &c);

	clutter_actor_set_size(stage, 500, 500);

	clutter_actor_show(stage);

	clutter_main();
}
