#include "fluid_synth_node.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_midi.hpp>

using namespace godot;

void FluidSynthNode::_bind_methods() {
    // Settings methods
	ClassDB::bind_method(D_METHOD("create_settings"), &FluidSynthNode::create_settings);
	ClassDB::bind_method(D_METHOD("change_setting_int", "setting", "value"), &FluidSynthNode::change_setting_int);
	ClassDB::bind_method(D_METHOD("change_setting_dbl", "setting", "value"), &FluidSynthNode::change_setting_dbl);
	ClassDB::bind_method(D_METHOD("change_setting_str", "setting", "value"), &FluidSynthNode::change_setting_str);
	ClassDB::bind_method(D_METHOD("delete_settings"), &FluidSynthNode::delete_settings);

    // Synth methods
	ClassDB::bind_method(D_METHOD("load_synth", "sf_path", "listen_ext_input"), &FluidSynthNode::load_synth);
	ClassDB::bind_method(D_METHOD("load_soundfont", "sf_path", "reset"), &FluidSynthNode::load_soundfont);
	ClassDB::bind_method(D_METHOD("unload_synth"), &FluidSynthNode::unload_synth);
	ClassDB::bind_method(D_METHOD("map_channel", "channel", "mapped_channel"), &FluidSynthNode::map_channel);
	ClassDB::bind_method(D_METHOD("setup_channel", "channel", "program", "reverb", "chorus",
        "volume", "pan", "expression"), &FluidSynthNode::setup_channel);
	ClassDB::bind_method(D_METHOD("set_interpolation", "val"), &FluidSynthNode::set_interpolation);
	ClassDB::bind_method(D_METHOD("render_file", "midi_file", "output_file", "sf_path", "interpolation"),
        &FluidSynthNode::render_file);

    // Player methods
	ClassDB::bind_method(D_METHOD("create_midi_player"), &FluidSynthNode::create_midi_player);
	ClassDB::bind_method(D_METHOD("delete_midi_player"), &FluidSynthNode::delete_midi_player);
	ClassDB::bind_method(D_METHOD("player_load_midi", "file_path"), &FluidSynthNode::player_load_midi);
	ClassDB::bind_method(D_METHOD("player_play", "loop_count"), &FluidSynthNode::player_play);
	ClassDB::bind_method(D_METHOD("player_seek", "tick"), &FluidSynthNode::player_seek);
	ClassDB::bind_method(D_METHOD("player_stop"), &FluidSynthNode::player_stop);
}

FluidSynthNode::FluidSynthNode() {
    settings = NULL;
    synth = NULL;
    adriver = NULL;
    player = NULL;
    set_process_input(false);
    for (int i = 0; i < 16; ++i) {
        channel_map[i] = i;
    }
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

int FluidSynthNode::create_settings() {
    // Create the settings.
    settings = new_fluid_settings();
    ERR_FAIL_COND_V_MSG(settings == NULL, -1,
		"Failed to create FluidSynth settings");

    return 0;
}

int FluidSynthNode::change_setting_int(String setting, int value){
    if (settings != NULL) {
        ERR_FAIL_COND_V_MSG(
            fluid_settings_setint(settings, setting.ascii(), value) == FLUID_FAILED, -1,
            vformat("Failed to change setting: %s", setting));
    }
    else {
        ERR_FAIL_V_MSG(-1, "FluidSynth settings not created");
    }
    return 0;
}

int FluidSynthNode::change_setting_dbl(String setting, double value){
    if (settings != NULL) {
        ERR_FAIL_COND_V_MSG(
            fluid_settings_setnum(settings, setting.ascii(), value) == FLUID_FAILED, -1,
            vformat("Failed to change setting: %s", setting));
    }
    else {
        ERR_FAIL_V_MSG(-1, "FluidSynth settings not created");
    }
    return 0;
}

int FluidSynthNode::change_setting_str(String setting, String value){
    if (settings != NULL) {
        ERR_FAIL_COND_V_MSG(
            fluid_settings_setstr(settings, setting.ascii(), value.ascii()) == FLUID_FAILED, -1,
            vformat("Failed to change setting: %s", setting));
    }
    else {
        ERR_FAIL_V_MSG(-1, "FluidSynth settings not created");
    }
    return 0;
}


int FluidSynthNode::delete_settings() {

    if (settings != NULL) {
        delete_fluid_settings(settings);
    }
    settings = NULL;

    return 0;
}

int FluidSynthNode::load_synth(String sf_path, bool listen_ext_input) {
    // Don't create if synth exists
    ERR_FAIL_COND_V_MSG(synth != NULL, -1,
        "FluidSynth instance exists, unload before creating a new one");

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

    set_process_input(listen_ext_input);

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
        vformat("Failed to load SoundFont: %s", sf_path));

    return cur_sfont_id;
}

int FluidSynthNode::unload_synth(bool include_settings) {
    /* Clean up */
    set_process_input(false);
    delete_midi_player();
    delete_fluid_audio_driver(adriver);
    adriver = NULL;
    delete_fluid_synth(synth);
    synth = NULL;
    if (include_settings) {
        delete_settings();
    }

    return 0;
}

void FluidSynthNode::map_channel(int channel, int mapped_channel) {
    channel_map[channel] = mapped_channel;
}

int FluidSynthNode::setup_channel(int channel, int program, int reverb, int chorus,
    int volume, int pan, int expression) {
    
    ERR_FAIL_COND_V_MSG(synth == NULL, -1,
        "Create a FluidSynth instance before channel setup");
    
    // Reset all controllers
    fluid_synth_cc(synth, channel, 121, 0);

    fluid_synth_program_change(synth, channel, program);
    fluid_synth_cc(synth, channel, 91, reverb);
    fluid_synth_cc(synth, channel, 93, chorus);
    fluid_synth_cc(synth, channel, 7, volume);
    fluid_synth_cc(synth, channel, 10, pan);
    fluid_synth_cc(synth, channel, 11, expression);

    return 0;
}

int FluidSynthNode::set_interpolation(int method) {
    fluid_interp interp_method = FLUID_INTERP_DEFAULT;
    switch(method) {
        case 0:
            interp_method = FLUID_INTERP_NONE;
            break;
        case 1:
            interp_method = FLUID_INTERP_LINEAR;
            break;
        case 2:
            interp_method = FLUID_INTERP_4THORDER;
            break;
        case 3:
            interp_method = FLUID_INTERP_7THORDER;
            break;
    }

    ERR_FAIL_COND_V_MSG(fluid_synth_set_interp_method(synth, -1, interp_method) == FLUID_FAILED, -1,
    "Failed to set interpolation method");

    return 0;
}

int FluidSynthNode::render_file(String midi_file, String output_file, String sf_path,
                                int interpolation) {
        
    unload_synth(false);
    // specify the file to store the audio to
    // make sure you compiled fluidsynth with libsndfile to get a real wave file
    // otherwise this file will only contain raw s16 stereo PCM
    fluid_settings_setstr(settings, "audio.file.name", output_file.ascii());

    char** timing_source;
    fluid_settings_dupstr(settings, "player.timing_source", timing_source);
    // use number of samples processed as timing source, rather than the system timer
    fluid_settings_setstr(settings, "player.timing-source", "sample");
    
    int lock_memory = 1;
    fluid_settings_getint(settings, "synth.lock_memory", &lock_memory);
    // since this is a non-realtime scenario, there is no need to pin the sample data
    fluid_settings_setint(settings, "synth.lock-memory", 0);
    
    synth = new_fluid_synth(settings);
    
    fluid_synth_sfload(synth, sf_path.ascii(), true);
    set_interpolation(interpolation);
    
    player = new_fluid_player(synth);
    fluid_player_add(player, midi_file.ascii());
    fluid_player_play(player);
    
    fluid_file_renderer_t* renderer = new_fluid_file_renderer(synth);
    
    while (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING)
    {
        if (fluid_file_renderer_process_block(renderer) != FLUID_OK)
        {
            break;
        }
    }
    
    // just for sure: stop the playback explicitly and wait until finished
    fluid_player_stop(player);
    fluid_player_join(player);
    
    delete_fluid_file_renderer(renderer);
    unload_synth(false);
    fluid_settings_setstr(settings, "player.timing-source", *timing_source);
    fluid_settings_setint(settings, "synth.lock-memory", lock_memory);
    load_synth(sf_path, false);
}


void FluidSynthNode::_input(const Ref<InputEvent> &event) {
    InputEventMIDI* midi_event;
    if ((midi_event = dynamic_cast<InputEventMIDI*>(*event)) != nullptr ) {
        int channel = channel_map[midi_event->get_channel()];
        switch(midi_event->get_message()) {
            case MIDI_MESSAGE_NOTE_OFF:
                fluid_synth_noteoff(synth, channel, midi_event->get_pitch());
                break;
            case MIDI_MESSAGE_NOTE_ON:
                fluid_synth_noteon(synth, channel, midi_event->get_pitch(),
                    midi_event->get_velocity());
                break;
            case MIDI_MESSAGE_AFTERTOUCH:
                fluid_synth_key_pressure(synth, channel, midi_event->get_pitch(),
                    midi_event->get_pressure());
                break;
            case MIDI_MESSAGE_CHANNEL_PRESSURE:
                fluid_synth_channel_pressure(synth, channel, midi_event->get_pressure());
                break;
            case MIDI_MESSAGE_CONTROL_CHANGE:
                fluid_synth_cc(synth, channel, midi_event->get_controller_number(),
                    midi_event->get_controller_value());
                break;
            case MIDI_MESSAGE_PITCH_BEND:
                fluid_synth_pitch_bend(synth, channel, midi_event->get_pitch());
                break;
            case MIDI_MESSAGE_PROGRAM_CHANGE:
                fluid_synth_program_change(synth, channel, midi_event->get_instrument());
                break;
            default:
                break;
        }
    }
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

int FluidSynthNode::player_play(int loop_count) {
    if (player != NULL) {
        if (loop_count >= -1) {
            fluid_player_set_loop(player, loop_count);
        }

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
