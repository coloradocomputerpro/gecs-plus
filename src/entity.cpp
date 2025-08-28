#include "entity.h"
#include "component.h"
#include "relationship.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

Entity::Entity() {}

Entity::~Entity() {}

void Entity::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_component", "component"), &Entity::add_component);
    ClassDB::bind_method(D_METHOD("add_components", "components"), &Entity::add_components);
    ClassDB::bind_method(D_METHOD("remove_component", "component"), &Entity::remove_component);
    ClassDB::bind_method(D_METHOD("remove_components", "components"), &Entity::remove_components);
    ClassDB::bind_method(D_METHOD("remove_all_components"), &Entity::remove_all_components);
    ClassDB::bind_method(D_METHOD("deferred_remove_component", "component"), &Entity::deferred_remove_component);
    ClassDB::bind_method(D_METHOD("get_component", "component_script"), &Entity::get_component);
    ClassDB::bind_method(D_METHOD("has_component", "component_script"), &Entity::has_component);
    
    ClassDB::bind_method(D_METHOD("add_relationship", "relationship"), &Entity::add_relationship);
    ClassDB::bind_method(D_METHOD("add_relationships", "relationships"), &Entity::add_relationships);
    ClassDB::bind_method(D_METHOD("remove_relationship", "relationship"), &Entity::remove_relationship);
    ClassDB::bind_method(D_METHOD("remove_relationships", "relationships"), &Entity::remove_relationships);
    ClassDB::bind_method(D_METHOD("get_relationship", "relationship_query", "single", "weak"), &Entity::get_relationship, DEFVAL(true), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_relationships", "relationship_query", "weak"), &Entity::get_relationships, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("has_relationship", "relationship_query", "weak"), &Entity::has_relationship, DEFVAL(false));

    ClassDB::bind_method(D_METHOD("get_component_resources"), &Entity::get_component_resources);
    ClassDB::bind_method(D_METHOD("set_component_resources", "p_value"), &Entity::set_component_resources);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "component_resources", PROPERTY_HINT_TYPE_STRING, "17/17:Component"), "set_component_resources", "get_component_resources");

    ClassDB::bind_method(D_METHOD("is_enabled"), &Entity::is_enabled);
    ClassDB::bind_method(D_METHOD("set_enabled", "p_value"), &Entity::set_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ADD_SIGNAL(MethodInfo("component_added", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "component")));
    ADD_SIGNAL(MethodInfo("component_removed", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "component")));
    ADD_SIGNAL(MethodInfo("component_property_changed",
        PropertyInfo(Variant::OBJECT, "entity"),
        PropertyInfo(Variant::OBJECT, "component"),
        PropertyInfo(Variant::STRING_NAME, "property"),
        PropertyInfo(Variant::NIL, "old_value"),
        PropertyInfo(Variant::NIL, "new_value")));
    ADD_SIGNAL(MethodInfo("relationship_added", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "relationship")));
    ADD_SIGNAL(MethodInfo("relationship_removed", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::OBJECT, "relationship")));
}

void Entity::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint()) {
        _initialize();
    }
}

void Entity::_initialize() {
    Array defined_components = define_components();
    for (int i = 0; i < defined_components.size(); i++) {
        Ref<Resource> res = defined_components[i];
        if (res.is_valid()) {
            component_resources.push_back(res);
        }
    }
    
    for (int i = 0; i < component_resources.size(); i++) {
        Ref<Resource> res = component_resources[i];
        if (res.is_valid()) {
            add_component(res->duplicate());
        }
    }
    
    on_ready();
}

void Entity::on_ready() {
    if (has_method("on_ready")) {
        call("on_ready");
    }
}

void Entity::on_update(double delta) {
    if (has_method("on_update")) {
        call("on_update", delta);
    }
}

void Entity::on_destroy() {
    if (has_method("on_destroy")) {
        call("on_destroy");
    }
}

void Entity::on_disable() {
    if (has_method("on_disable")) {
        call("on_disable");
    }
}

void Entity::on_enable() {
    if (has_method("on_enable")) {
        call("on_enable");
    }
}

Array Entity::define_components() {
    if (has_method("define_components")) {
        return call("define_components");
    }
    return Array();
}

void Entity::add_component(const Ref<Component> &p_component) {
    if (p_component.is_null()) return;
    Ref<Script> scr = p_component->get_script();
    if (scr.is_null()) return;

    String path = scr->get_path();
    if (components.has(path)) {
        remove_component(Ref<Component>(components[path]));
    }
    components[path] = p_component;
    p_component->connect("property_changed", callable_mp(this, &Entity::_on_component_property_changed));
    emit_signal("component_added", this, p_component);
}

void Entity::add_components(const Array &p_components) {
    for (int i = 0; i < p_components.size(); i++) {
        add_component(p_components[i]);
    }
}

void Entity::remove_component(const Ref<Resource> &p_component) {
    if (p_component.is_null()) return;
    
    String resource_path;
    
    Ref<Component> comp_instance = Object::cast_to<Component>(p_component.ptr());
    if (comp_instance.is_valid()) {
        Ref<Script> scr = comp_instance->get_script();
        if (scr.is_null()) return;
        resource_path = scr->get_path();
    } else {
        Ref<Script> scr = Object::cast_to<Script>(p_component.ptr());
        if (scr.is_valid()) {
            resource_path = scr->get_path();
        } else {
            resource_path = p_component->get_path();
        }
    }
    
    if (resource_path.is_empty()) return;

    if (components.has(resource_path)) {
        Ref<Component> removed_comp = components[resource_path];
        if (removed_comp.is_valid() && removed_comp->is_connected("property_changed", callable_mp(this, &Entity::_on_component_property_changed))) {
            removed_comp->disconnect("property_changed", callable_mp(this, &Entity::_on_component_property_changed));
        }
        components.erase(resource_path);
        emit_signal("component_removed", this, removed_comp);
    }
}

void Entity::remove_components(const Array &p_components) {
    for (int i = 0; i < p_components.size(); i++) {
        remove_component(p_components[i]);
    }
}

void Entity::remove_all_components() {
    Array comps = components.values();
    for (int i = 0; i < comps.size(); i++) {
        remove_component(comps[i]);
    }
}

void Entity::deferred_remove_component(const Ref<Component> &p_component) {
    call_deferred("remove_component", p_component);
}

void Entity::_on_component_property_changed(Object *component, const StringName &property, const Variant &old_value, const Variant &new_value) {
    emit_signal("component_property_changed", this, component, property, old_value, new_value);
}

Ref<Component> Entity::get_component(const Ref<Resource> &p_component_script) const {
    if (p_component_script.is_null()) {
        return Ref<Component>();
    }
    
    String path;
    Ref<Script> scr = Object::cast_to<Script>(p_component_script.ptr());
    if (scr.is_valid()) {
        path = scr->get_path();
    } else {
        path = p_component_script->get_path();
    }
    
    if (components.has(path)) {
        return components[path];
    }
    return Ref<Component>();
}

bool Entity::has_component(const Ref<Resource> &p_component_script) const {
    if (p_component_script.is_null()) {
        return false;
    }
    
    String path;
    Ref<Script> scr = Object::cast_to<Script>(p_component_script.ptr());
    if (scr.is_valid()) {
        path = scr->get_path();
    } else {
        path = p_component_script->get_path();
    }
    
    return components.has(path);
}

void Entity::add_relationship(const Ref<Relationship> &p_relationship) {
    if (p_relationship.is_null()) return;
    p_relationship->set_source(this);
    relationships.push_back(p_relationship);
    emit_signal("relationship_added", this, p_relationship);
}

void Entity::add_relationships(const Array &p_relationships) {
    for (int i = 0; i < p_relationships.size(); ++i) {
        add_relationship(p_relationships[i]);
    }
}

void Entity::remove_relationship(const Ref<Relationship> &p_relationship) {
    if (p_relationship.is_null()) return;
    for (int i = relationships.size() - 1; i >= 0; --i) {
        Ref<Relationship> rel = relationships[i];
        if (rel.is_valid() && rel->matches(p_relationship)) {
            relationships.remove_at(i);
            emit_signal("relationship_removed", this, rel);
        }
    }
}

void Entity::remove_relationships(const Array &p_relationships) {
    for (int i = 0; i < p_relationships.size(); ++i) {
        remove_relationship(p_relationships[i]);
    }
}

Variant Entity::get_relationship(const Ref<Relationship> &p_relationship_query, bool single, bool weak) const {
    Array results;
    for (int i = 0; i < relationships.size(); i++) {
        Ref<Relationship> rel = relationships[i];
        if (!rel->is_valid()) {
            continue;
        }
        if (rel->matches(p_relationship_query, weak)) {
            if (single) {
                return rel;
            }
            results.append(rel);
        }
    }
    if (single) {
        return Variant();
    }
    return results;
}

Array Entity::get_relationships(const Ref<Relationship> &p_relationship_query, bool weak) const {
    return get_relationship(p_relationship_query, false, weak);
}

bool Entity::has_relationship(const Ref<Relationship> &p_relationship_query, bool weak) const {
    return get_relationship(p_relationship_query, true, weak).get_type() != Variant::NIL;
}

void Entity::set_component_resources(const TypedArray<Component> &p_resources) {
    component_resources = p_resources;
}

TypedArray<Component> Entity::get_component_resources() const {
    return component_resources;
}

void Entity::set_enabled(bool p_enabled) {
    enabled = p_enabled;
}

bool Entity::is_enabled() const {
    return enabled;
}