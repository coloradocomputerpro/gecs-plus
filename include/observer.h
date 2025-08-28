#ifndef OBSERVER_H
#define OBSERVER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include "entity.h"

namespace godot {

class QueryBuilder;

class Observer : public Node {
    GDCLASS(Observer, Node)

protected:
    static void _bind_methods();
    Ref<QueryBuilder> q;

    GDVIRTUAL0R(Object*, match)
    GDVIRTUAL0R(Ref<Resource>, watch)
    GDVIRTUAL2(on_component_added, Entity*, Ref<Resource>)
    GDVIRTUAL2(on_component_removed, Entity*, Ref<Resource>)
    GDVIRTUAL5(on_component_changed, Entity*, Ref<Resource>, const StringName &, const Variant &, const Variant &)
    
public:
    Observer();
    ~Observer();
    
    void set_q(const Ref<QueryBuilder> &p_q);

    virtual Ref<QueryBuilder> match();
    virtual Ref<Resource> watch();
    virtual void on_component_added(Entity *entity, Ref<Resource> component);
    virtual void on_component_removed(Entity *entity, Ref<Resource> component);
    virtual void on_component_changed(Entity *entity, Ref<Resource> component, const StringName &property, const Variant &new_value, const Variant &old_value);
};

}

#endif // OBSERVER_H