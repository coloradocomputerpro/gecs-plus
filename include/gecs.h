#ifndef GECS_H
#define GECS_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot {

class World;
class Entity;
class Component;

class GECS : public Node {
    GDCLASS(GECS, Node)

private:
    static GECS *singleton;
    World *world;
    bool debug = true;
    Variant wildcard = Variant();
    Array entity_preprocessors;
    Array entity_postprocessors;

    void _on_world_exited();

protected:
    static void _bind_methods();

public:
    GECS();
    ~GECS();

    static GECS *get_singleton();

    void set_world(World *p_world);
    World *get_world() const;
    void process(double delta, const String &group = "");

    void set_debug(bool p_debug);
    bool get_debug() const;

    void set_entity_preprocessors(const Array &p_processors);
    Array get_entity_preprocessors() const;

    void set_entity_postprocessors(const Array &p_processors);
    Array get_entity_postprocessors() const;

    Array get_components(const Array &entities, const Ref<Script> &component_type, const Ref<Component> &default_component = nullptr) const;

    static Array intersect(const Array &array1, const Array &array2);
    static Array union_arrays(const Array &array1, const Array &array2);
    static Array difference(const Array &array1, const Array &array2);
    static void topological_sort(Dictionary systems_by_group);
};

}

#endif