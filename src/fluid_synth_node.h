#ifndef FLUIDSYNTHNODE_H
#define FLUIDSYNTHNODE_H

#include <godot_cpp/classes/node.hpp>
#include <fluidsynth.h>

namespace godot {

class FluidSynthNode : public Node {
	GDCLASS(FluidSynthNode, Node)

private:
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_audio_driver_t *adriver;
    fluid_player_t *player;
    int cur_sfont_id;

protected:
	static void _bind_methods();

public:
	FluidSynthNode();
	~FluidSynthNode();

    // Settings
    int load_settings(String file_path);
    int save_settings(String file_path);
    int unload_settings();

    // Synth
    int load_synth(String sf_path);
    int load_soundfont(String sf_path, int reset);
    int unload_synth(bool include_settings);

    // Player
    int create_midi_player();
    int delete_midi_player();
    int player_load_midi(String file_path);
    int player_play();
    int player_seek(int tick);
    int player_stop();
};

}

#endif