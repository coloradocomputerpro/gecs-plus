#include "world.h"
#include "entity.h"
#include "system.h"
#include "observer.h"
#include "query_builder.h"
#include "component.h"
#include "relationship.h"
#include "gecs.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

World::World() {}

World::~World() {}

void World::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_entity", "entity"), &World::add_entity);
    ClassDB::bind_method(D_METHOD("remove_entity", "entity"), &World::remove_entity);
    ClassDB::bind_method(D_METHOD("disable_entity", "entity"), &World::disable_entity);
    ClassDB::bind_method(D_METHOD("enable_entity", "entity"), &World::enable_entity);
    ClassDB::bind_method(D_METHOD("add_system", "system", "topo_sort"), &World::add_system, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_observer", "observer"), &World::add_observer);
    ClassDB::bind_method(D_METHOD("process", "delta", "group"), &World::process, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("get_query"), &World::get_query);
    ClassDB::bind_method(D_METHOD("purge", "should_free"), &World::purge, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("initialize"), &World::initialize);

    ClassDB::bind_method(D_METHOD("set_entity_nodes_root", "path"), &World::set_entity_nodes_root);
    ClassDB::bind_method(D_METHOD("get_entity_nodes_root"), &World::get_entity_nodes_root);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "entity_nodes_root", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node"), "set_entity_nodes_root", "get_entity_nodes_root");

    ClassDB::bind_method(D_METHOD("set_system_nodes_root", "path"), &World::set_system_nodes_root);
    ClassDB::bind_method(D_METHOD("get_system_nodes_root"), &World::get_system_nodes_root);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "system_nodes_root", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node"), "set_system_nodes_root", "get_system_nodes_root");

    ClassDB::bind_method(D_METHOD("get_cache_stats"), &World::get_cache_stats);
    ClassDB::bind_method(D_METHOD("reset_cache_stats"), &World::reset_cache_stats);

    ADD_SIGNAL(MethodInfo("entity_added", PropertyInfo(Variant::OBJECT, "entity")));
    ADD_SIGNAL(MethodInfo("entity_removed", PropertyInfo(Variant::OBJECT, "entity")));
    ADD_SIGNAL(MethodInfo("entity_disabled", PropertyInfo(Variant::OBJECT, "entity")));
    ADD_SIGNAL(MethodInfo("entity_enabled", PropertyInfo(Variant::OBJECT, "entity")));
    ADD_SIGNAL(MethodInfo("system_added", PropertyInfo(Variant::OBJECT, "system")));
    ADD_SIGNAL(MethodInfo("system_removed", PropertyInfo(Variant::OBJECT, "system")));
    ADD_SIGNAL(MethodInfo("component_added", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "component")));
    ADD_SIGNAL(MethodInfo("component_removed", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "component")));
    ADD_SIGNAL(MethodInfo("component_changed", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "component"), PropertyInfo(Variant::STRING, "property"), PropertyInfo(Variant::NIL, "new_value"), PropertyInfo(Variant::NIL, "old_value")));
    ADD_SIGNAL(MethodInfo("relationship_added", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "relationship")));
    ADD_SIGNAL(MethodInfo("relationship_removed", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "relationship")));
    ADD_SIGNAL(MethodInfo("cache_invalidated"));
}

void World::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint()) {
        initialize();
    }
}

void World::initialize() {
    if (entity_nodes_root.is_empty() || !has_node(entity_nodes_root)) {
        Node* n = memnew(Node);
        n->set_name("Entities");
        add_child(n);
        set_entity_nodes_root(n->get_path());
    }
    if (system_nodes_root.is_empty() || !has_node(system_nodes_root)) {
        Node* n = memnew(Node);
        n->set_name("Systems");
        add_child(n);
        set_system_nodes_root(n->get_path());
    }

    if (has_node(system_nodes_root)) {
        Node* sys_root = get_node<Node>(system_nodes_root);
        for (int i = 0; i < sys_root->get_child_count(); ++i) {
            if (System* s = Object::cast_to<System>(sys_root->get_child(i))) {
                add_system(s);
            } else if (Observer* o = Object::cast_to<Observer>(sys_root->get_child(i))) {
                add_observer(o);
            }
        }
        GECS::topological_sort(systems_by_group);
    }
    
    if (has_node(entity_nodes_root)) {
        Node* ent_root = get_node<Node>(entity_nodes_root);
        for (int i = 0; i < ent_root->get_child_count(); ++i) {
            if (Entity* e = Object::cast_to<Entity>(ent_root->get_child(i))) {
                add_entity(e);
            }
        }
    }
}

void World::add_entity(Entity *entity) {
    if (!entity) return;
    Node* ent_root = get_node<Node>(entity_nodes_root);
    if (ent_root && !entity->is_inside_tree()) {
        ent_root->add_child(entity);
    }

    int64_t id = entity->get_instance_id();
    entities[id] = entity;
    
    emit_signal("entity_added", entity);

    entity->connect("component_added", callable_mp(this, &World::_on_entity_component_added));
    entity->connect("component_removed", callable_mp(this, &World::_on_entity_component_removed));
    entity->connect("component_property_changed", callable_mp(this, &World::_on_entity_component_property_changed));
    
    TypedArray<Component> existing_components = entity->get_component_resources();
    for (int i = 0; i < existing_components.size(); i++) {
        Ref<Component> comp = existing_components[i];
        if (comp.is_valid()) {
            Ref<Script> scr = comp->get_script();
            if(scr.is_valid()) {
                _add_entity_to_index(entity, scr->get_path());
            }
        }
    }

    GECS* ecs = GECS::get_singleton();
    if(ecs) {
        Array preprocessors = ecs->get_entity_preprocessors();
        for(int i = 0; i < preprocessors.size(); i++) {
            Callable c = preprocessors[i];
            Array args;
            args.push_back(entity);
            c.callv(args);
        }
    }
    
    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::add_entities(const Array &p_entities) {
    for (int i = 0; i < p_entities.size(); ++i) {
        Entity *entity = Object::cast_to<Entity>(p_entities[i]);
        if (entity) {
            add_entity(entity);
        }
    }
}

void World::remove_entity(Entity *entity) {
    if (!entity) return;

    GECS* ecs = GECS::get_singleton();
    if(ecs) {
        Array postprocessors = ecs->get_entity_postprocessors();
        for(int i = 0; i < postprocessors.size(); i++) {
            Callable c = postprocessors[i];
            Array args;
            args.push_back(entity);
            c.callv(args);
        }
    }

    emit_signal("entity_removed", entity);
    
    int64_t id = entity->get_instance_id();
    entities.erase(id);
    TypedArray<Component> existing_components = entity->get_component_resources();
    for (int i = 0; i < existing_components.size(); i++) {
        Ref<Component> comp = existing_components[i];
        if (comp.is_valid()) {
            Ref<Script> scr = comp->get_script();
            if (scr.is_valid()) {
                _remove_entity_from_index(entity, scr->get_path());
            }
        }
    }

    entity->disconnect("component_added", callable_mp(this, &World::_on_entity_component_added));
    entity->disconnect("component_removed", callable_mp(this, &World::_on_entity_component_removed));
    entity->disconnect("component_property_changed", callable_mp(this, &World::_on_entity_component_property_changed));
    entity->on_destroy();
    entity->queue_free();
    
    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::disable_entity(Entity *entity) {
    if (!entity) return;
    entity->set_enabled(false);
    emit_signal("entity_disabled", entity);

    entity->disconnect("component_added", callable_mp(this, &World::_on_entity_component_added));
    entity->disconnect("component_removed", callable_mp(this, &World::_on_entity_component_removed));
    entity->disconnect("component_property_changed", callable_mp(this, &World::_on_entity_component_property_changed));

    entity->on_disable();
    entity->set_process(false);
    entity->set_physics_process(false);

    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::enable_entity(Entity *entity) {
    if (!entity) return;
    entity->set_enabled(true);
    emit_signal("entity_enabled", entity);

    entity->connect("component_added", callable_mp(this, &World::_on_entity_component_added));
    entity->connect("component_removed", callable_mp(this, &World::_on_entity_component_removed));
    entity->connect("component_property_changed", callable_mp(this, &World::_on_entity_component_property_changed));

    entity->on_enable();
    entity->set_process(true);
    entity->set_physics_process(true);

    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::add_system(System *system, bool topo_sort) {
    if (!system) return;
    Node* sys_root = get_node<Node>(system_nodes_root);
    if(sys_root && !system->is_inside_tree()) {
        sys_root->add_child(system);
    }

    String group = system->get_group();
    if (!systems_by_group.has(group)) {
        systems_by_group[group] = Array();
    }
    Array group_systems = systems_by_group[group];
    group_systems.push_back(system);
    systems_by_group[group] = group_systems;
    
    system->set_q(get_query());
    system->setup();
    if (topo_sort) {
        GECS::topological_sort(systems_by_group);
    }
    emit_signal("system_added", system);
}

void World::add_systems(const Array &p_systems, bool topo_sort) {
    for (int i = 0; i < p_systems.size(); ++i) {
        System *system = Object::cast_to<System>(p_systems[i]);
        if (system) {
            add_system(system, false);
        }
    }
    if (topo_sort) {
        GECS::topological_sort(systems_by_group);
    }
}

void World::remove_system(System *system, bool topo_sort) {
    if (!system) return;
    String group = system->get_group();
    if (systems_by_group.has(group)) {
        Array group_systems = systems_by_group[group];
        group_systems.erase(system);
        systems_by_group[group] = group_systems;
        if (group_systems.is_empty()) {
            systems_by_group.erase(group);
        }
    }
    emit_signal("system_removed", system);
    system->queue_free();
    if (topo_sort) {
        GECS::topological_sort(systems_by_group);
    }
}

void World::add_observer(Observer *observer) {
    if (!observer) return;
    Node* sys_root = get_node<Node>(system_nodes_root);
    if(sys_root && !observer->is_inside_tree()) {
        sys_root->add_child(observer);
    }
    observer->set_q(get_query());
    observers.push_back(observer);
}

void World::add_observers(const Array &p_observers) {
    for (int i = 0; i < p_observers.size(); ++i) {
        Observer *observer = Object::cast_to<Observer>(p_observers[i]);
        if (observer) {
            add_observer(observer);
        }
    }
}

void World::remove_observer(Observer *observer) {
    if (!observer) return;
    observers.erase(observer);
    observer->queue_free();
}

void World::purge(bool should_free) {
    Array entity_values = entities.values();
    for (int i = 0; i < entity_values.size(); ++i) {
        remove_entity(Object::cast_to<Entity>(entity_values[i]));
    }
    
    Array group_keys = systems_by_group.keys();
    for (int i = 0; i < group_keys.size(); ++i) {
        Array systems_in_group = systems_by_group[group_keys[i]];
        for (int j = 0; j < systems_in_group.size(); ++j) {
            remove_system(Object::cast_to<System>(systems_in_group[j]));
        }
    }

    for (int i = 0; i < observers.size(); ++i) {
        remove_observer(Object::cast_to<Observer>(observers[i]));
    }

    if (should_free) {
        queue_free();
    }
}

void World::process(double delta, const String &group) {
    if (systems_by_group.has(group)) {
        Array group_systems = systems_by_group[group];
        for (int i = 0; i < group_systems.size(); i++) {
            System *system = Object::cast_to<System>(group_systems[i]);
            if (system && system->get_active() && !system->get_paused()) {
                system->_handle(delta);
            }
        }
    }
    _process_observer_queue();
}

void World::_process_observer_queue() {
    if (_processing_observers || _observer_queue.is_empty()) {
        return;
    }
    _processing_observers = true;
    Array current_queue = _observer_queue;
    _observer_queue.clear();
    for (int i = 0; i < current_queue.size(); i++) {
        Dictionary event = current_queue[i];
        String type = event["type"];
        
        Entity* entity = Object::cast_to<Entity>(event["entity"]);
        Component* component = Object::cast_to<Component>(event["component"]);
        if (!entity || !component) {
            continue;
        }

        if (type == "component_added") {
            _handle_observer_component_added(entity, component);
        } else if (type == "component_removed") {
            _handle_observer_component_removed(entity, component);
        } else if (type == "component_changed") {
            _handle_observer_component_changed(entity, component, event["property"], event["new_value"], event["old_value"]);
        }
    }
    _processing_observers = false;
}

Ref<QueryBuilder> World::get_query() {
    Ref<QueryBuilder> qb;
    qb.instantiate();
    qb->_init(this);
    connect("cache_invalidated", callable_mp(qb.ptr(), &QueryBuilder::invalidate_cache));
    return qb;
}

String World::_generate_query_cache_key(const Array &all_comps, const Array &any_comps, const Array &none_comps) {
    Array all_paths, any_paths, none_paths;
    for(int i = 0; i < all_comps.size(); i++) {
        Ref<Script> scr = all_comps[i];
        if (scr.is_valid()) all_paths.push_back(scr->get_path());
    }
    for(int i = 0; i < any_comps.size(); i++) {
        Ref<Script> scr = any_comps[i];
        if (scr.is_valid()) any_paths.push_back(scr->get_path());
    }
    for(int i = 0; i < none_comps.size(); i++) {
        Ref<Script> scr = none_comps[i];
        if (scr.is_valid()) none_paths.push_back(scr->get_path());
    }
    
    all_paths.sort();
    any_paths.sort();
    none_paths.sort();
    return "ALL:" + String(",").join(all_paths) + "|ANY:" + String(",").join(any_paths) + "|NONE:" + String(",").join(none_paths);
}

Array World::_query(const Array &all_comps, const Array &any_comps, const Array &none_comps) {
    if (all_comps.is_empty() && any_comps.is_empty() && none_comps.is_empty()) {
        return entities.values();
    }
    
    String cache_key = _generate_query_cache_key(all_comps, any_comps, none_comps);
    if (_query_result_cache.has(cache_key)) {
        return _query_result_cache[cache_key];
    }
    
    Array result;
    bool has_all_filter = !all_comps.is_empty();
    if (has_all_filter) {
        Ref<Script> smallest_set_script;
        int smallest_size = -1;
        for (int i = 0; i < all_comps.size(); ++i) {
            Ref<Script> script = all_comps[i];
            if (script.is_null()) continue;
            String path = script->get_path();
            if (!component_entity_index.has(path)) {
                _query_result_cache[cache_key] = Array();
                return Array();
            }
            Array entities_with_comp = component_entity_index[path];
            if (smallest_size == -1 || entities_with_comp.size() < smallest_size) {
                smallest_size = entities_with_comp.size();
                smallest_set_script = script;
            }
        }
        
        if (smallest_set_script.is_null()) {
            _query_result_cache[cache_key] = Array();
            return Array();
        }

        result = component_entity_index[smallest_set_script->get_path()].duplicate();
        for (int i = 0; i < all_comps.size(); ++i) {
            Ref<Script> script = all_comps[i];
            if (script.is_null() || script == smallest_set_script) continue;
            result = GECS::intersect(result, component_entity_index[script->get_path()]);
            if (result.is_empty()) {
                _query_result_cache[cache_key] = Array();
                return Array();
            }
        }
    }
    
    if (!any_comps.is_empty()) {
        Array any_result;
        for (int i = 0; i < any_comps.size(); ++i) {
            Ref<Script> script = any_comps[i];
            if (script.is_null()) continue;
            String path = script->get_path();
            if (component_entity_index.has(path)) {
                any_result = GECS::union_arrays(any_result, component_entity_index[path]);
            }
        }
        if (has_all_filter) {
            result = GECS::intersect(result, any_result);
        } else {
            result = any_result;
        }
    }
    
    if (!has_all_filter && any_comps.is_empty()) {
        result = entities.values();
    }
    
    if (!none_comps.is_empty()) {
        for (int i = 0; i < none_comps.size(); ++i) {
            Ref<Script> script = none_comps[i];
            if (script.is_null()) continue;
            String path = script->get_path();
            if (component_entity_index.has(path)) {
                result = GECS::difference(result, component_entity_index[path]);
            }
        }
    }

    _query_result_cache[cache_key] = result;
    return result;
}

void World::_add_entity_to_index(Entity *entity, const String &component_path) {
    if (!component_entity_index.has(component_path)) {
        component_entity_index[component_path] = Array();
    }
    Array list = component_entity_index[component_path];
    if (!list.has(entity)) {
        list.push_back(entity);
        component_entity_index[component_path] = list;
    }
}

void World::_remove_entity_from_index(Entity *entity, const String &component_path) {
    if (component_entity_index.has(component_path)) {
        Array list = component_entity_index[component_path];
        list.erase(entity);
        component_entity_index[component_path] = list;
    }
}

void World::_on_entity_component_added(Object *entity_obj, Object *component_obj) {
    Entity* entity = Object::cast_to<Entity>(entity_obj);
    Component* component = Object::cast_to<Component>(component_obj);
    if (!entity || !component) return;
    
    Ref<Script> script = component->get_script();
    if (script.is_null()) return;

    _add_entity_to_index(entity, script->get_path());
    emit_signal("component_added", entity, component);

    Dictionary event;
    event["type"] = "component_added";
    event["entity"] = entity;
    event["component"] = component;
    _observer_queue.push_back(event);

    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::_on_entity_component_removed(Object *entity_obj, Object *component_obj) {
    Entity* entity = Object::cast_to<Entity>(entity_obj);
    Component* component = Object::cast_to<Component>(component_obj);
    if (!entity || !component) return;
    
    Ref<Script> script = component->get_script();
    if (script.is_null()) return;
    
    _remove_entity_from_index(entity, script->get_path());
    emit_signal("component_removed", entity, component);

    Dictionary event;
    event["type"] = "component_removed";
    event["entity"] = entity;
    event["component"] = component;
    _observer_queue.push_back(event);

    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::_on_entity_component_property_changed(Object *entity_obj, Object *component_obj, const StringName &property, const Variant &old_value, const Variant &new_value) {
    Entity* entity = Object::cast_to<Entity>(entity_obj);
    Component* component = Object::cast_to<Component>(component_obj);
    if (!entity || !component) return;

    emit_signal("component_changed", entity, component, property, new_value, old_value);

    Dictionary event;
    event["type"] = "component_changed";
    event["entity"] = entity;
    event["component"] = component;
    event["property"] = property;
    event["new_value"] = new_value;
    event["old_value"] = old_value;
    _observer_queue.push_back(event);
    
    _query_result_cache.clear();
    emit_signal("cache_invalidated");
}

void World::_handle_observer_component_added(Entity *entity, Component *component) {
    for (int i = 0; i < observers.size(); i++) {
        Observer *observer = Object::cast_to<Observer>(observers[i]);
        if (!observer) continue;

        Ref<Resource> watch_script = observer->watch();

        Ref<Script> comp_script = component->get_script();
        if (watch_script.is_valid() && comp_script.is_valid() && watch_script->get_path() == comp_script->get_path()) {
            Ref<QueryBuilder> qb = observer->match();
            if (qb.is_valid() && qb->execute().has(entity)) {
                observer->on_component_added(entity, component);
            }
        }
    }
}

void World::_handle_observer_component_removed(Entity *entity, Component *component) {
    for (int i = 0; i < observers.size(); i++) {
        Observer *observer = Object::cast_to<Observer>(observers[i]);
        if (!observer) continue;

        Ref<Resource> watch_script = observer->watch();
        Ref<Script> comp_script = component->get_script();
        if (watch_script.is_valid() && comp_script.is_valid() && watch_script->get_path() == comp_script->get_path()) {
            observer->on_component_removed(entity, component);
        }
    }
}

void World::_handle_observer_component_changed(Entity *entity, Component *component, const StringName &property, const Variant &new_value, const Variant &old_value) {
    for (int i = 0; i < observers.size(); i++) {
        Observer *observer = Object::cast_to<Observer>(observers[i]);
        if (!observer) continue;

        Ref<Resource> watch_script = observer->watch();
        Ref<Script> comp_script = component->get_script();
        if (watch_script.is_valid() && comp_script.is_valid() && watch_script->get_path() == comp_script->get_path()) {
            Ref<QueryBuilder> qb = observer->match();
            if (qb.is_valid() && qb->execute().has(entity)) {
                observer->on_component_changed(entity, component, property, new_value, old_value);
            }
        }
    }
}

void World::set_entity_nodes_root(const NodePath &p_path) {
    entity_nodes_root = p_path;
}

NodePath World::get_entity_nodes_root() const {
    return entity_nodes_root;
}

void World::set_system_nodes_root(const NodePath &p_path) {
    system_nodes_root = p_path;
}

NodePath World::get_system_nodes_root() const {
    return system_nodes_root;
}

Dictionary World::get_cache_stats() const {
    int total_requests = _cache_hits + _cache_misses;
    double hit_rate = 0.0;
    if (total_requests > 0) {
        hit_rate = (double)_cache_hits / (double)total_requests;
    }
    
    Dictionary stats;
    stats["cache_hits"] = _cache_hits;
    stats["cache_misses"] = _cache_misses;
    stats["hit_rate"] = hit_rate;
    stats["cached_queries"] = (int)_query_result_cache.size();
    
    return stats;
}

void World::reset_cache_stats() {
    _cache_hits = 0;
    _cache_misses = 0;
}