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

	CmkButton *button = cmk_button_new(CMK_BUTTON_FLAT);
	clutter_actor_add_child(stage, CMK_CLUTTER(button));
	clutter_actor_set_position(CMK_CLUTTER(button), 0, 0);

	clutter_actor_set_size(stage, 1000, 1000);

	clutter_actor_show(stage);

	clutter_main();
}
