#ifndef WORLD_H
#define WORLD_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {

class Entity;
class System;
class Observer;
class QueryBuilder;
class Component;
class Relationship;

class World : public Node {
    GDCLASS(World, Node)

private:
    Dictionary entities;
    Dictionary systems_by_group;
    Dictionary component_entity_index;
    
    Array observers;
    Array _observer_queue;
    bool _processing_observers = false;

    NodePath entity_nodes_root;
    NodePath system_nodes_root;

    Dictionary _query_result_cache;

    // Add public access to reverse_relationship_index for QueryBuilder
public:
    Dictionary relationship_entity_index;
    Dictionary reverse_relationship_index; // Made public for QueryBuilder access

private:
    int _cache_hits = 0;
    int _cache_misses = 0;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    World();
    ~World();

    void initialize();
    void add_entity(Entity *entity);
    void add_entities(const Array &p_entities);
    void remove_entity(Entity *entity);
    void disable_entity(Entity *entity);
    void enable_entity(Entity *entity);

    void add_system(System *system, bool topo_sort = false);
    void add_systems(const Array &p_systems, bool topo_sort = false);
    void remove_system(System *system, bool topo_sort = false);

    void add_observer(Observer *observer);
    void add_observers(const Array &p_observers);
    void remove_observer(Observer *observer);

    void purge(bool should_free = true);

    void process(double delta, const String &group = "");
    
    Ref<QueryBuilder> get_query();
    Array _query(const Array &all, const Array &any, const Array &none);

    void set_entity_nodes_root(const NodePath &p_path);
    NodePath get_entity_nodes_root() const;
    void set_system_nodes_root(const NodePath &p_path);
    NodePath get_system_nodes_root() const;

    // Helper method for cache stats
    Dictionary get_cache_stats() const;
    void reset_cache_stats();

private:
    String _generate_query_cache_key(const Array &all, const Array &any, const Array &none);
    void _add_entity_to_index(Entity *entity, const String &component_path);
    void _remove_entity_from_index(Entity *entity, const String &component_path);

    void _on_entity_component_added(Object *entity, Object *component);
    void _on_entity_component_removed(Object *entity, Object *component);
    void _on_entity_component_property_changed(Object *entity, Object *component, const StringName &property, const Variant &old_value, const Variant &new_value);
    
    void _process_observer_queue();
    void _handle_observer_component_added(Entity *entity, Component *component);
    void _handle_observer_component_removed(Entity *entity, Component *component);
    void _handle_observer_component_changed(Entity *entity, Component *component, const StringName &property, const Variant &new_value, const Variant &old_value);
};

}

#endif