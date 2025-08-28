#ifndef COMPONENT_H
#define COMPONENT_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class Component : public Resource {
    GDCLASS(Component, Resource)

protected:
    static void _bind_methods();
    
public:
    Component();
    ~Component();
    
    Dictionary serialize();
    bool equals(const Ref<Component> &other);
    
    void emit_property_changed(const String &property_name, const Variant &old_value, const Variant &new_value);
};

}

#endif // COMPONENT_H