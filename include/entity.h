#ifndef ENTITY_H
#define ENTITY_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {

class Component;
class Relationship;

class Entity : public Node {
    GDCLASS(Entity, Node)

private:
    bool enabled = true;
    Dictionary components;
    Array relationships;
    TypedArray<Component> component_resources;

    void _on_component_property_changed(Object *component, const StringName &property, const Variant &old_value, const Variant &new_value);

protected:
    static void _bind_methods();
    void _notification(int p_what);
    void _initialize();

public:
    Entity();
    ~Entity();

    void add_component(const Ref<Component> &p_component);
    void add_components(const Array &p_components);
    void remove_component(const Ref<Resource> &p_component);
    void remove_components(const Array &p_components);
    void remove_all_components();
    void deferred_remove_component(const Ref<Component> &p_component);
    Ref<Component> get_component(const Ref<Resource> &p_component_script) const;
    bool has_component(const Ref<Resource> &p_component_script) const;
    
    void add_relationship(const Ref<Relationship> &p_relationship);
    void add_relationships(const Array &p_relationships);
    void remove_relationship(const Ref<Relationship> &p_relationship);
    void remove_relationships(const Array &p_relationships);
    Variant get_relationship(const Ref<Relationship> &p_relationship_query, bool single = true, bool weak = false) const;
    Array get_relationships(const Ref<Relationship> &p_relationship_query, bool weak = false) const;
    bool has_relationship(const Ref<Relationship> &p_relationship_query, bool weak = false) const;

    void set_component_resources(const TypedArray<Component> &p_resources);
    TypedArray<Component> get_component_resources() const;
    
    void set_enabled(bool p_enabled);
    bool is_enabled() const;

    void on_ready();
    void on_update(double delta);
    void on_destroy();
    void on_disable();
    void on_enable();
    Array define_components();
};

}

#endif // ENTITY_H