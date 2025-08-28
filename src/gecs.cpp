#include "gecs.h"
#include "world.h"
#include "system.h"
#include "entity.h"
#include "component.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

using namespace godot;

GECS *GECS::singleton = nullptr;

GECS::GECS() {
    if (singleton == nullptr) {
        singleton = this;
    }
    world = nullptr;
}

GECS::~GECS() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

GECS *GECS::get_singleton() {
    return singleton;
}

void GECS::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_world", "world"), &GECS::set_world);
    ClassDB::bind_method(D_METHOD("get_world"), &GECS::get_world);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world", PROPERTY_HINT_NODE_TYPE, "World"), "set_world", "get_world");

    ClassDB::bind_method(D_METHOD("process", "delta", "group"), &GECS::process, DEFVAL(""));

    ClassDB::bind_method(D_METHOD("set_debug", "p_debug"), &GECS::set_debug);
    ClassDB::bind_method(D_METHOD("get_debug"), &GECS::get_debug);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug"), "set_debug", "get_debug");
    
    ClassDB::bind_method(D_METHOD("set_entity_preprocessors", "p_processors"), &GECS::set_entity_preprocessors);
    ClassDB::bind_method(D_METHOD("get_entity_preprocessors"), &GECS::get_entity_preprocessors);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entity_preprocessors"), "set_entity_preprocessors", "get_entity_preprocessors");

    ClassDB::bind_method(D_METHOD("set_entity_postprocessors", "p_processors"), &GECS::set_entity_postprocessors);
    ClassDB::bind_method(D_METHOD("get_entity_postprocessors"), &GECS::get_entity_postprocessors);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entity_postprocessors"), "set_entity_postprocessors", "get_entity_postprocessors");

    ClassDB::bind_method(D_METHOD("get_components", "entities", "component_type", "default_component"), &GECS::get_components, DEFVAL(Ref<Component>()));

    ADD_SIGNAL(MethodInfo("world_changed", PropertyInfo(Variant::OBJECT, "world")));
    ADD_SIGNAL(MethodInfo("world_exited"));

    ClassDB::bind_static_method("GECS", D_METHOD("intersect", "array1", "array2"), &GECS::intersect);
    ClassDB::bind_static_method("GECS", D_METHOD("union_arrays", "array1", "array2"), &GECS::union_arrays);
    ClassDB::bind_static_method("GECS", D_METHOD("difference", "array1", "array2"), &GECS::difference);
    ClassDB::bind_static_method("GECS", D_METHOD("topological_sort", "systems_by_group"), &GECS::topological_sort);
}

void GECS::set_world(World *p_world) {
    world = p_world;
    if (world) {
        if (!world->is_inside_tree() && get_tree()) {
            get_tree()->get_root()->add_child(world);
        }
        world->connect("tree_exited", callable_mp(this, &GECS::_on_world_exited));
    }
    emit_signal("world_changed", world);
}

World *GECS::get_world() const {
    return world;
}

void GECS::_on_world_exited() {
    world = nullptr;
    emit_signal("world_exited");
}

void GECS::process(double delta, const String &group) {
    if (world) {
        world->process(delta, group);
    }
}

void GECS::set_debug(bool p_debug) {
    debug = p_debug;
}

bool GECS::get_debug() const {
    return debug;
}

void GECS::set_entity_preprocessors(const Array &p_processors) {
    entity_preprocessors = p_processors;
}

Array GECS::get_entity_preprocessors() const {
    return entity_preprocessors;
}

void GECS::set_entity_postprocessors(const Array &p_processors) {
    entity_postprocessors = p_processors;
}

Array GECS::get_entity_postprocessors() const {
    return entity_postprocessors;
}

Array GECS::get_components(const Array &entities, const Ref<Script> &component_type, const Ref<Component> &default_component) const {
    Array components;
    for (int i = 0; i < entities.size(); i++) {
        Entity *entity = Object::cast_to<Entity>(entities[i]);
        if (entity) {
            Ref<Component> component = entity->get_component(component_type);
            if (component.is_valid()) {
                components.push_back(component);
            } else if (default_component.is_valid()) {
                components.push_back(default_component);
            } else {
                UtilityFunctions::push_error("Entity does not have component: ", component_type->get_path());
            }
        }
    }
    return components;
}

Array GECS::intersect(const Array &array1, const Array &array2) {
    const Array &small_array = array1.size() < array2.size() ? array1 : array2;
    const Array &large_array = array1.size() < array2.size() ? array2 : array1;

    Dictionary lookup;
    for (int i = 0; i < large_array.size(); ++i) {
        lookup[large_array[i]] = true;
    }

    Array result;
    for (int i = 0; i < small_array.size(); ++i) {
        if (lookup.has(small_array[i])) {
            result.push_back(small_array[i]);
        }
    }
    return result;
}

Array GECS::union_arrays(const Array &array1, const Array &array2) {
    Dictionary seen;
    Array result;
    for (int i = 0; i < array1.size(); ++i) {
        Variant item = array1[i];
        if (!seen.has(item)) {
            seen[item] = true;
            result.push_back(item);
        }
    }
    
    for (int i = 0; i < array2.size(); ++i) {
        Variant item = array2[i];
        if (!seen.has(item)) {
            seen[item] = true;
            result.push_back(item);
        }
    }
    
    return result;
}

Array GECS::difference(const Array &array1, const Array &array2) {
    Dictionary lookup;
    for (int i = 0; i < array2.size(); ++i) {
        lookup[array2[i]] = true;
    }

    Array result;
    for (int i = 0; i < array1.size(); ++i) {
        if (!lookup.has(array1[i])) {
            result.push_back(array1[i]);
        }
    }
    return result;
}

void GECS::topological_sort(Dictionary systems_by_group) {
    for (const Variant &group_key : systems_by_group.keys()) {
        Array systems = systems_by_group[group_key];
        if (systems.size() <= 1) {
            continue;
        }

        Dictionary adjacency;
        Dictionary indegree;
        Array wildcard_front;
        Array wildcard_back;
        for (int i = 0; i < systems.size(); i++) {
            System *s = Object::cast_to<System>(systems[i]);
            if (s) {
                adjacency[s] = Array();
                indegree[s] = 0;
            }
        }

        for (int i = 0; i < systems.size(); i++) {
            System *s = Object::cast_to<System>(systems[i]);
            if (!s) continue;
            
            Dictionary deps_dict = s->deps();
            if (!deps_dict.has(System::Before)) deps_dict[System::Before] = Array();
            if (!deps_dict.has(System::After)) deps_dict[System::After] = Array();

            if (deps_dict.has(System::Before)) {
                Array before_array = deps_dict[System::Before];
                for (int j = 0; j < before_array.size(); j++) {
                    if (before_array[j].get_type() == Variant::NIL) {
                        wildcard_front.append(s);
                    } else if (systems.has(before_array[j])) {
                        Array adj = adjacency[s];
                        adj.append(before_array[j]);
                        adjacency[s] = adj;
                        indegree[before_array[j]] = int(indegree[before_array[j]]) + 1;
                    }
                }
            }

            if (deps_dict.has(System::After)) {
                Array after_array = deps_dict[System::After];
                for (int j = 0; j < after_array.size(); j++) {
                    if (after_array[j].get_type() == Variant::NIL) {
                        wildcard_back.append(s);
                    } else if (systems.has(after_array[j])) {
                        System *other = Object::cast_to<System>(after_array[j]);
                        if (other) {
                            Array adj = adjacency[other];
                            adj.append(s);
                            adjacency[other] = adj;
                            indegree[s] = int(indegree[s]) + 1;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < wildcard_front.size(); i++) {
            System *w = Object::cast_to<System>(wildcard_front[i]);
            for (int j = 0; j < systems.size(); j++) {
                System *other = Object::cast_to<System>(systems[j]);
                if (other && other != w) {
                    Array adj = adjacency[w];
                    if (!adj.has(other)) {
                        adj.append(other);
                        adjacency[w] = adj;
                        indegree[other] = int(indegree[other]) + 1;
                    }
                }
            }
        }

        for (int i = 0; i < wildcard_back.size(); i++) {
            System *w = Object::cast_to<System>(wildcard_back[i]);
            for (int j = 0; j < systems.size(); j++) {
                System *other = Object::cast_to<System>(systems[j]);
                if (other && other != w) {
                    Array adj = adjacency[other];
                    if (!adj.has(w)) {
                        adj.append(w);
                        adjacency[other] = adj;
                        indegree[w] = int(indegree[w]) + 1;
                    }
                }
            }
        }

        Array queue;
        for (int i = 0; i < systems.size(); i++) {
            System *s = Object::cast_to<System>(systems[i]);
            if (s && int(indegree[s]) == 0) {
                queue.append(s);
            }
        }

        Array sorted_result;
        while (queue.size() > 0) {
            System *current = Object::cast_to<System>(queue[0]);
            queue.remove_at(0);
            sorted_result.append(current);
            
            Array adj = adjacency[current];
            for (int i = 0; i < adj.size(); i++) {
                System *next = Object::cast_to<System>(adj[i]);
                if (next) {
                    indegree[next] = int(indegree[next]) - 1;
                    if (int(indegree[next]) == 0) {
                        queue.append(next);
                    }
                }
            }
        }

        if (sorted_result.size() == systems.size()) {
            systems_by_group[group_key] = sorted_result;
        } else {
            UtilityFunctions::push_error("Topological sort failed for group '", group_key, "'. Possible cycle or mismatch in dependencies.");
        }
    }
}