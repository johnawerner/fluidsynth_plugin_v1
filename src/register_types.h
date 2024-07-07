#ifndef FLUIDSYNTHNODE_REGISTER_TYPES_H
#define FLUIDSYNTHNODE_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_fluidsynth_module(ModuleInitializationLevel p_level);
void uninitialize_fluidsynth_module(ModuleInitializationLevel p_level);

#endif // FLUIDSYNTHNODE_REGISTER_TYPES_H
