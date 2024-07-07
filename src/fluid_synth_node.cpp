#include "fluid_synth_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

void FluidSynthNode::_bind_methods() {
    // Settings methods
	ClassDB::bind_method(D_METHOD("load_settings", "file_path"), &FluidSynthNode::load_settings);
	ClassDB::bind_method(D_METHOD("save_settings", "file_path"), &FluidSynthNode::save_settings);
	ClassDB::bind_method(D_METHOD("unload_settings", "include_settings"), &FluidSynthNode::unload_settings);

    // Synth methods
	ClassDB::bind_method(D_METHOD("load_synth", "sf_path"), &FluidSynthNode::load_synth);
	ClassDB::bind_method(D_METHOD("load_soundfont", "sf_path", "reset"), &FluidSynthNode::load_soundfont);
	ClassDB::bind_method(D_METHOD("unload_synth"), &FluidSynthNode::unload_synth);

    // Player methods
	ClassDB::bind_method(D_METHOD("create_midi_player"), &FluidSynthNode::create_midi_player);
	ClassDB::bind_method(D_METHOD("delete_midi_player"), &FluidSynthNode::delete_midi_player);
	ClassDB::bind_method(D_METHOD("player_load_midi", "file_path"), &FluidSynthNode::player_load_midi);
	ClassDB::bind_method(D_METHOD("player_play"), &FluidSynthNode::player_play);
	ClassDB::bind_method(D_METHOD("player_seek", "tick"), &FluidSynthNode::player_seek);
	ClassDB::bind_method(D_METHOD("player_stop"), &FluidSynthNode::player_stop);
}

FluidSynthNode::FluidSynthNode() {
    settings = NULL;
    synth = NULL;
    adriver = NULL;
    player = NULL;
}

FluidSynthNode::~FluidSynthNode() {
	if (synth != NULL) {
        unload_synth(true);
    }
    settings = NULL;
    synth = NULL;
    adriver = NULL;
    player = NULL;
}

int FluidSynthNode::load_settings(String file_path) {

    // Create the settings.
    settings = new_fluid_settings();
    ERR_FAIL_COND_V_MSG(settings == NULL, -1,
		"Failed to create FluidSynth settings");
 
    // Change the settings

    return 0;
}

int FluidSynthNode::save_settings(String file_path) {

    // TODO: Save the settings

    return 0;
}

int FluidSynthNode::unload_settings() {

    if (settings != NULL) {
        delete_fluid_settings(settings);
    }
    settings = NULL;

    return 0;
}

int FluidSynthNode::load_synth(String sf_path) {

    // Create the synthesizer
    synth = new_fluid_synth(settings);
    if(synth == NULL)
    {
        unload_synth(true);
        ERR_FAIL_V_MSG(-1, "Failed to create FluidSynth");
    }

    // Load the soundfont
    if (load_soundfont(sf_path, 1) == -1) {
        unload_synth(false);
        return -1;
    }

    /* Create the audio driver. The synthesizer starts playing as soon
       as the driver is created. */
    adriver = new_fluid_audio_driver(settings, synth);
    if (adriver == NULL)
    {
        unload_synth(true);
        ERR_FAIL_V_MSG(-1, "Failed to create audio driver for FluidSynth");
    }

    return 0;
}

int FluidSynthNode::load_soundfont(String sf_path, int reset) {

    /* Load a SoundFont and reset presets (so that new instruments
     * get used from the SoundFont)
     * Depending on the size of the SoundFont, this will take some time to complete...
     */
    ERR_FAIL_COND_V_MSG(synth == NULL, -1,
        "Create a FluidSynth instance before loading a SoundFont");

    cur_sfont_id = fluid_synth_sfload(synth, sf_path.ascii(), reset);
    ERR_FAIL_COND_V_MSG(cur_sfont_id == FLUID_FAILED, -1,
        vformat("Failed to load SoundFont: %s", sf_path.ascii()));

    return cur_sfont_id;
}

int FluidSynthNode::unload_synth(bool include_settings) {
    /* Clean up */
    delete_midi_player();
    delete_fluid_audio_driver(adriver);
    delete_fluid_synth(synth);
    if (include_settings) {
        unload_settings();
    }

    return 0;
}

int FluidSynthNode::create_midi_player() {
    ERR_FAIL_COND_V_MSG(player != NULL, -1,
        "FluidSynth player already exists");

    player = new_fluid_player(synth);

    ERR_FAIL_COND_V_MSG(player == NULL, -1,
        "Creating FluidSynth player failed");

    return 0;
}

int FluidSynthNode::delete_midi_player() {
    delete_fluid_player(player);
    player = NULL;
    return 0;
}

int FluidSynthNode::player_load_midi(String file_path) {
    ERR_FAIL_COND_V_MSG(
        fluid_player_add(player, file_path.ascii()) == FLUID_FAILED, -1,
        "FluidSynth player failed to load MIDI file");

    return 0;
}

int FluidSynthNode::player_play() {
    if (player != NULL) {
        ERR_FAIL_COND_V_MSG(fluid_player_play(player) == FLUID_FAILED, -1,
            "FluidSynth player failed to play");
    }
    return 0;
}

int FluidSynthNode::player_seek(int tick) {
    if (player != NULL) {
        ERR_FAIL_COND_V_MSG(fluid_player_seek(player, tick) == FLUID_FAILED, -1,
            "FluidSynth player failed to seek in file");
    }
    return 0;
}

int FluidSynthNode::player_stop() {
    if (player != NULL) {
        fluid_player_stop(player);
    }
    return 0;
}
