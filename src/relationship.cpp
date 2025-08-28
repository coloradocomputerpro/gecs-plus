#include "relationship.h"
#include "component.h"
#include "entity.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Relationship::Relationship() : target(nullptr), source(nullptr) {}
Relationship::~Relationship() {}

void Relationship::_init(const Ref<Component> &p_relation, Object* p_target) {
    relation = p_relation;
    target = p_target;
}

void Relationship::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_relation"), &Relationship::get_relation);
    ClassDB::bind_method(D_METHOD("set_relation", "p_value"), &Relationship::set_relation);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "relation", PROPERTY_HINT_RESOURCE_TYPE, "Component"), "set_relation", "get_relation");
    
    ClassDB::bind_method(D_METHOD("get_target"), &Relationship::get_target);
    ClassDB::bind_method(D_METHOD("set_target", "p_value"), &Relationship::set_target);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target"), "set_target", "get_target");
    
    ClassDB::bind_method(D_METHOD("get_source"), &Relationship::get_source);
    ClassDB::bind_method(D_METHOD("set_source", "p_value"), &Relationship::set_source);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source"), "set_source", "get_source");
    
    ClassDB::bind_method(D_METHOD("matches", "other", "weak"), &Relationship::matches, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("is_valid"), &Relationship::is_valid);
    
    ClassDB::bind_method(D_METHOD("initialize", "relation", "target"), &Relationship::_init, DEFVAL(Ref<Component>()), DEFVAL(nullptr));
}

void Relationship::set_relation(const Ref<Component> &p_relation) {
    relation = p_relation;
}

Ref<Component> Relationship::get_relation() const {
    return relation;
}

void Relationship::set_target(Object *p_target) {
    target = p_target;
}

Object *Relationship::get_target() const {
    return target;
}

void Relationship::set_source(Object *p_source) {
    source = p_source;
}

Object *Relationship::get_source() const {
    return source;
}

bool Relationship::is_valid() const {
    // NOTE: Compromise to ensure compilation.
    // Proper check requires ObjectDB, which is failing to include.
    // This may lead to crashes if an entity in a relationship is freed.
    return source != nullptr;
}

bool Relationship::matches(const Ref<Relationship> &other, bool weak) const {
    if (other.is_null()) return false;

    bool rel_match = false;
    if (other->get_relation().is_null() || relation.is_null()) {
        rel_match = true;
    } else {
        if (weak) {
            Ref<Script> rel_script = relation->get_script();
            Ref<Script> other_rel_script = other->get_relation()->get_script();
            if(rel_script.is_valid() && other_rel_script.is_valid()) {
                rel_match = rel_script->get_path() == other_rel_script->get_path();
            }
        } else {
            rel_match = relation->equals(other->get_relation());
        }
    }

    bool target_match = false;
    Object* other_target = other->get_target();
    if (other_target == nullptr || target == nullptr) {
        target_match = true;
    } else {
        if (target == other_target) {
            target_match = true;
        } else {
            Entity* target_ent = Object::cast_to<Entity>(target);
            Ref<Script> target_script = Object::cast_to<Script>(target);
            Entity* other_target_ent = Object::cast_to<Entity>(other_target);
            Ref<Script> other_target_script = Object::cast_to<Script>(other_target);

            if (target_ent && other_target_script.is_valid()) {
                Ref<Script> target_ent_script = target_ent->get_script();
                target_match = target_ent_script.is_valid() && target_ent_script == other_target_script;
            } else if (target_script.is_valid() && other_target_ent) {
                Ref<Script> other_target_ent_script = other_target_ent->get_script();
                target_match = other_target_ent_script.is_valid() && other_target_ent_script == target_script;
            } else if (target_script.is_valid() && other_target_script.is_valid()) {
                 target_match = target_script == other_target_script;
            }
        }
    }

    return rel_match && target_match;
}