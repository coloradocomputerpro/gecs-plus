#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "component.h"
#include "relationship.h"
#include "query_builder.h"
#include "entity.h"
#include "observer.h"
#include "system.h"
#include "world.h"
#include "gecs.h"

using namespace godot;

static GECS *ecs_singleton;

void initialize_gecs_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register classes in dependency order
    ClassDB::register_class<Component>();
    ClassDB::register_class<Relationship>();
    ClassDB::register_class<QueryBuilder>();
    ClassDB::register_class<Entity>();
    ClassDB::register_class<Observer>();
    ClassDB::register_class<System>();
    ClassDB::register_class<World>();
    ClassDB::register_class<GECS>();

    // Create and register singleton
    ecs_singleton = memnew(GECS);
    Engine::get_singleton()->register_singleton("ECS", GECS::get_singleton());
}

void uninitialize_gecs_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    Engine::get_singleton()->unregister_singleton("ECS");
    memdelete(ecs_singleton);
}

extern "C" {
    GDExtensionBool GDE_EXPORT gecsplus_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
        init_obj.register_initializer(initialize_gecs_module);
        init_obj.register_terminator(uninitialize_gecs_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}