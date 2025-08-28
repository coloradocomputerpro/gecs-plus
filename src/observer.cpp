#include "observer.h"
#include "world.h"
#include "gecs.h"
#include "query_builder.h"
#include "entity.h"
#include "component.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Observer::Observer() {}
Observer::~Observer() {}

void Observer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_q", "query_builder"), &Observer::set_q);
}

void Observer::set_q(const Ref<QueryBuilder>& p_q) {
    q = p_q;
}

Ref<QueryBuilder> Observer::match() {
    if (has_method("match")) {
        return call("match");
    }
    
    if (q.is_null()) {
        GECS* ecs = GECS::get_singleton();
        if (ecs && ecs->get_world()) {
            q = ecs->get_world()->get_query();
        }
    }
    return q;
}

Ref<Resource> Observer::watch() {
    if (has_method("watch")) {
        return call("watch");
    }
    UtilityFunctions::push_error("Observer must override the watch() method.");
    return Ref<Resource>();
}

void Observer::on_component_added(Entity *entity, Ref<Resource> component) {
    if (has_method("on_component_added")) {
        call("on_component_added", entity, component);
    }
}

void Observer::on_component_removed(Entity *entity, Ref<Resource> component) {
    if (has_method("on_component_removed")) {
        call("on_component_removed", entity, component);
    }
}

void Observer::on_component_changed(Entity *entity, Ref<Resource> component, const StringName &property, const Variant &new_value, const Variant &old_value) {
    if (has_method("on_component_changed")) {
        call("on_component_changed", entity, component, property, new_value, old_value);
    }
}