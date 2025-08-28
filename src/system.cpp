#include "system.h"
#include "gecs.h"
#include "world.h"
#include "entity.h"
#include "query_builder.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

System::System() {}

System::~System() {}

void System::_notification(int p_what) {
    Node::_notification(p_what);
}

void System::_bind_methods() {
    BIND_ENUM_CONSTANT(Before);
    BIND_ENUM_CONSTANT(After);
    
    ClassDB::bind_method(D_METHOD("get_group"), &System::get_group);
    ClassDB::bind_method(D_METHOD("set_group", "p_value"), &System::set_group);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "group"), "set_group", "get_group");
    
    ClassDB::bind_method(D_METHOD("get_process_empty"), &System::get_process_empty);
    ClassDB::bind_method(D_METHOD("set_process_empty", "p_value"), &System::set_process_empty);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "process_empty"), "set_process_empty", "get_process_empty");
    
    ClassDB::bind_method(D_METHOD("get_active"), &System::get_active);
    ClassDB::bind_method(D_METHOD("set_active", "p_value"), &System::set_active);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "get_active");
    
    ClassDB::bind_method(D_METHOD("get_order"), &System::get_order);
    ClassDB::bind_method(D_METHOD("set_order", "p_value"), &System::set_order);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "order"), "set_order", "get_order");
    
    ClassDB::bind_method(D_METHOD("get_paused"), &System::get_paused);
    ClassDB::bind_method(D_METHOD("set_paused", "p_value"), &System::set_paused);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused"), "set_paused", "get_paused");

    ClassDB::bind_method(D_METHOD("_handle", "delta"), &System::_handle);
    ClassDB::bind_method(D_METHOD("set_q", "query_builder"), &System::set_q);
    ClassDB::bind_method(D_METHOD("get_q"), &System::get_q);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "q", PROPERTY_HINT_RESOURCE_TYPE, "QueryBuilder", PROPERTY_USAGE_NO_EDITOR), "", "get_q");
}

void System::set_q(const Ref<QueryBuilder> &p_q) {
    q = p_q;
}

Ref<QueryBuilder> System::get_q() {
    if (q.is_null()) {
        GECS* ecs = GECS::get_singleton();
        if (ecs && ecs->get_world()) {
            set_q(ecs->get_world()->get_query());
        }
    }
    return q;
}

void System::_handle(double delta) {
    if (!active || paused) {
        return;
    }

    if (has_method("sub_systems") && !sub_systems().is_empty()) {
        return;
    }
    
    Ref<QueryBuilder> qb = query();
    if (qb.is_valid()) {
        Array entities = qb->execute();
        process_all(entities, delta);
    }
}

Dictionary System::deps() {
    if (has_method("deps")) {
        return call("deps");
    }
    return Dictionary();
}

Ref<QueryBuilder> System::query() {
    if (has_method("query")) {
        return call("query");
    }
    process_empty = true;
    return get_q();
}

Array System::sub_systems() {
    if (has_method("sub_systems")) {
        return call("sub_systems");
    }
    return Array();
}

void System::setup() {
    if (has_method("setup")) {
        call("setup");
    }
}

void System::process(Entity *entity, double delta) {
    if (has_method("process")) {
        call("process", entity, delta);
    }
}

bool System::process_all(const Array &entities, double delta) {
    if (has_method("process_all")) {
        return call("process_all", entities, delta);
    }

    if (entities.is_empty() && process_empty) {
        process(nullptr, delta);
        return true;
    }
    
    for (int i = 0; i < entities.size(); i++) {
        Entity* entity = Object::cast_to<Entity>(entities[i]);
        if (entity) {
            process(entity, delta);
            entity->on_update(delta);
        }
    }
    return !entities.is_empty();
}

void System::set_group(const String &p_group) { 
    group = p_group;
}

String System::get_group() const { 
    return group;
}

void System::set_process_empty(bool p_process_empty) { 
    process_empty = p_process_empty;
}

bool System::get_process_empty() const { 
    return process_empty;
}

void System::set_active(bool p_active) { 
    active = p_active;
}

bool System::get_active() const { 
    return active;
}

void System::set_order(int p_order) { 
    order = p_order;
}

int System::get_order() const { 
    return order;
}

void System::set_paused(bool p_paused) { 
    paused = p_paused;
}

bool System::get_paused() const { 
    return paused;
}