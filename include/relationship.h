#ifndef RELATIONSHIP_H
#define RELATIONSHIP_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {

class Component;

class Relationship : public Resource {
    GDCLASS(Relationship, Resource)

private:
    Ref<Component> relation;
    Object* target;
    Object* source;

protected:
    static void _bind_methods();
    
    // Virtual methods for GDScript override
    GDVIRTUAL1R(bool, _matches, Ref<Relationship>)
    GDVIRTUAL0R(bool, _is_valid)

public:
    Relationship();
    ~Relationship();

    void _init(const Ref<Component> &p_relation = Ref<Component>(), Object* p_target = nullptr);
    
    bool matches(const Ref<Relationship> &other, bool weak = false) const;
    bool is_valid() const;

    void set_relation(const Ref<Component> &p_relation);
    Ref<Component> get_relation() const;
    void set_target(Object *p_target);
    Object *get_target() const;
    void set_source(Object *p_source);
    Object *get_source() const;
};

}

#endif // RELATIONSHIP_H