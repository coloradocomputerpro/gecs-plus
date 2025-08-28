#include "component.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Component::Component() {}

Component::~Component() {}

void Component::_bind_methods() {
    ClassDB::bind_method(D_METHOD("emit_property_changed", "property_name", "old_value", "new_value"), &Component::emit_property_changed);
    ClassDB::bind_method(D_METHOD("serialize"), &Component::serialize);
    ClassDB::bind_method(D_METHOD("equals", "other"), &Component::equals);

    ADD_SIGNAL(MethodInfo("property_changed", 
        PropertyInfo(Variant::OBJECT, "component"),
        PropertyInfo(Variant::STRING, "property_name"), 
        PropertyInfo(Variant::NIL, "old_value"),
        PropertyInfo(Variant::NIL, "new_value")));
}

Dictionary Component::serialize() {
    if (has_method("serialize")) {
        return call("serialize");
    }
    
    Dictionary data;
    Ref<Script> scr = get_script();
    if (scr.is_valid()) {
        TypedArray<Dictionary> props = scr->get_script_property_list();
        for (int i = 0; i < props.size(); i++) {
            Dictionary p = props[i];
            String name = p["name"];
            Variant value = get(name);
            data[name] = value;
        }
    }
    return data;
}

bool Component::equals(const Ref<Component> &other) {
    if (has_method("equals")) {
        return call("equals", other);
    }
    
    if (other.is_null()) {
        return false;
    }
    
    Ref<Script> scr = get_script();
    Ref<Script> other_scr = other->get_script();
    if (scr.is_null() || other_scr.is_null() || scr != other_scr) {
        return false;
    }

    TypedArray<Dictionary> props = scr->get_script_property_list();
    for (int i = 0; i < props.size(); i++) {
        Dictionary p = props[i];
        String name = p["name"];
        Variant this_value = get(name);
        Variant other_value = other->get(name);
        if (this_value != other_value) {
            return false;
        }
    }
    return true;
}

void Component::emit_property_changed(const String &property_name, const Variant &old_value, const Variant &new_value) {
    emit_signal("property_changed", this, property_name, old_value, new_value);
}