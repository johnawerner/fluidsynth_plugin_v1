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
    int channel_map[16];

protected:
	static void _bind_methods();

public:
	FluidSynthNode();
	~FluidSynthNode();

    // Settings
    int create_settings();
    int change_setting_int(String setting, int value);
    int change_setting_dbl(String setting, double value);
    int change_setting_str(String setting, String value);
    int delete_settings();

    // Synth
    int load_synth(String sf_path);
    int load_soundfont(String sf_path, int reset);
    int unload_synth(bool include_settings);
    void map_channel(int channel, int mapped_channel);
    int setup_channel(int channel, int program, int reverb, int chorus,
        int volume = 100, int pan = 64, int expression = 127);
    void _input(const Ref<InputEvent> &event) override;


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