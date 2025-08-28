#include "query_builder.h"
#include "world.h"
#include "entity.h"
#include "component.h"
#include "relationship.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

QueryBuilder::QueryBuilder() : world(nullptr) {}

QueryBuilder::~QueryBuilder() {}

void QueryBuilder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("with_all", "components"), &QueryBuilder::with_all);
    ClassDB::bind_method(D_METHOD("with_any", "components"), &QueryBuilder::with_any);
    ClassDB::bind_method(D_METHOD("with_none", "components"), &QueryBuilder::with_none);
    ClassDB::bind_method(D_METHOD("with_relationship", "relationships"), &QueryBuilder::with_relationship);
    ClassDB::bind_method(D_METHOD("without_relationship", "relationships"), &QueryBuilder::without_relationship);
    ClassDB::bind_method(D_METHOD("with_group", "groups"), &QueryBuilder::with_group);
    ClassDB::bind_method(D_METHOD("without_group", "groups"), &QueryBuilder::without_group);
    ClassDB::bind_method(D_METHOD("with_reverse_relationship", "relationships"), &QueryBuilder::with_reverse_relationship);
    
    ClassDB::bind_method(D_METHOD("execute_one"), &QueryBuilder::execute_one);
    ClassDB::bind_method(D_METHOD("clear"), &QueryBuilder::clear);
    ClassDB::bind_method(D_METHOD("invalidate_cache"), &QueryBuilder::invalidate_cache);
    ClassDB::bind_method(D_METHOD("is_empty"), &QueryBuilder::is_empty);
    ClassDB::bind_method(D_METHOD("as_array"), &QueryBuilder::as_array);
    ClassDB::bind_method(D_METHOD("compile", "query"), &QueryBuilder::compile);

    GDVIRTUAL_BIND(execute);
    GDVIRTUAL_BIND(matches, "entities");
    GDVIRTUAL_BIND(combine, "other");
}

void QueryBuilder::_init(World* p_world) {
    world = p_world;
}

QueryBuilder* QueryBuilder::with_all(const Array &p_components) {
    all_components = p_components;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::with_any(const Array &p_components) {
    any_components = p_components;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::with_none(const Array &p_components) {
    none_components = p_components;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::with_relationship(const Array &p_relationships) {
    relationships = p_relationships;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::without_relationship(const Array &p_relationships) {
    exclude_relationships = p_relationships;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::with_group(const Array &p_groups) {
    groups = p_groups;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::without_group(const Array &p_groups) {
    exclude_groups = p_groups;
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::with_reverse_relationship(const Array &p_relationships) {
    for (int i = 0; i < p_relationships.size(); i++) {
        Variant rel_var = p_relationships[i];
        if (rel_var.get_type() == Variant::OBJECT) {
            Ref<Relationship> rel = rel_var;
            if (rel.is_valid() && rel->get_relation().is_valid()) {
                Ref<Script> rel_script = rel->get_relation()->get_script();
                if (rel_script.is_valid()) {
                    String rev_key = "reverse_" + rel_script->get_path();
                    if (world && world->reverse_relationship_index.has(rev_key)) {
                    }
                }
            }
        }
    }
    invalidate_cache();
    return this;
}

QueryBuilder* QueryBuilder::clear() {
    all_components.clear();
    any_components.clear();
    none_components.clear();
    relationships.clear();
    exclude_relationships.clear();
    groups.clear();
    exclude_groups.clear();
    invalidate_cache();
    return this;
}

void QueryBuilder::invalidate_cache() {
    cache_valid = false;
    cached_result.clear();
}

Array QueryBuilder::execute() {
    Array ret;
    if (GDVIRTUAL_CALL(execute, ret)) {
        return ret;
    }
    
    if (cache_valid) {
        return cached_result;
    }
    if (world) {
        cached_result = _internal_execute();
        cache_valid = true;
        return cached_result;
    }
    return Array();
}

Object* QueryBuilder::execute_one() {
    Array result = execute();
    if (!result.is_empty()) {
        return result[0];
    }
    return nullptr;
}

Array QueryBuilder::_internal_execute() {
    if (!world) {
        return Array();
    }
    
    Array result = world->_query(all_components, any_components, none_components);
    if (relationships.is_empty() && exclude_relationships.is_empty() && groups.is_empty() && exclude_groups.is_empty()) {
        return result;
    }

    Array filtered_result;
    for (int i = 0; i < result.size(); ++i) {
        Entity* entity = Object::cast_to<Entity>(result[i]);
        if (!entity) continue;

        bool match = true;
        
        if (!groups.is_empty()) {
            bool in_any_group = false;
            for (int g = 0; g < groups.size(); ++g) {
                String group_name = groups[g];
                if (entity->is_in_group(group_name)) {
                    in_any_group = true;
                    break;
                }
            }
            if (!in_any_group) match = false;
        }
        if (match && !exclude_groups.is_empty()) {
            for (int g = 0; g < exclude_groups.size(); ++g) {
                String group_name = exclude_groups[g];
                if (entity->is_in_group(group_name)) {
                    match = false;
                    break;
                }
            }
        }

        if (match && !relationships.is_empty()) {
            for (int r = 0; r < relationships.size(); ++r) {
                Variant rel_var = relationships[r];
                if (rel_var.get_type() == Variant::OBJECT) {
                    Ref<Relationship> rel = rel_var;
                    if (rel.is_valid() && !entity->has_relationship(rel)) {
                        match = false;
                        break;
                    }
                }
            }
        }
        if (match && !exclude_relationships.is_empty()) {
            for (int r = 0; r < exclude_relationships.size(); ++r) {
                Variant rel_var = exclude_relationships[r];
                if (rel_var.get_type() == Variant::OBJECT) {
                    Ref<Relationship> rel = rel_var;
                    if (rel.is_valid() && entity->has_relationship(rel)) {
                        match = false;
                        break;
                    }
                }
            }
        }
        
        if (match) {
            filtered_result.push_back(entity);
        }
    }
    return filtered_result;
}

Array QueryBuilder::matches(const Array &p_entities) {
    Array ret;
    if (GDVIRTUAL_CALL(matches, p_entities, ret)) {
        return ret;
    }
    
    if (is_empty()) {
        return p_entities;
    }
    
    Array result;
    for (int i = 0; i < p_entities.size(); i++) {
        Entity *entity = Object::cast_to<Entity>(p_entities[i]);
        if (!entity) continue;
        
        bool match = true;

        for (int j = 0; j < all_components.size(); j++) {
            Variant comp_var = all_components[j];
            if (comp_var.get_type() == Variant::OBJECT) {
                Ref<Resource> comp = comp_var;
                if (comp.is_valid() && !entity->has_component(comp)) {
                    match = false;
                    break;
                }
            }
        }
        if (!match) continue;
        if (!any_components.is_empty()) {
            bool any_match = false;
            for (int j = 0; j < any_components.size(); j++) {
                Variant comp_var = any_components[j];
                if (comp_var.get_type() == Variant::OBJECT) {
                    Ref<Resource> comp = comp_var;
                    if (comp.is_valid() && entity->has_component(comp)) {
                        any_match = true;
                        break;
                    }
                }
            }
            if (!any_match) match = false;
        }
        if (!match) continue;
        for (int j = 0; j < none_components.size(); j++) {
            Variant comp_var = none_components[j];
            if (comp_var.get_type() == Variant::OBJECT) {
                Ref<Resource> comp = comp_var;
                if (comp.is_valid() && entity->has_component(comp)) {
                    match = false;
                    break;
                }
            }
        }
        if (!match) continue;
        if (match) {
            result.push_back(entity);
        }
    }
    return result;
}

Ref<QueryBuilder> QueryBuilder::combine(const Ref<QueryBuilder> &other) {
    Ref<QueryBuilder> ret;
    if (GDVIRTUAL_CALL(combine, other, ret)) {
        return ret;
    }
    
    if (other.is_valid()) {
        all_components.append_array(other->all_components);
        any_components.append_array(other->any_components);
        none_components.append_array(other->none_components);
        relationships.append_array(other->relationships);
        exclude_relationships.append_array(other->exclude_relationships);
        groups.append_array(other->groups);
        exclude_groups.append_array(other->exclude_groups);
        invalidate_cache();
    }
    return this;
}

bool QueryBuilder::is_empty() const {
    return (
        all_components.is_empty() &&
        any_components.is_empty() &&
        none_components.is_empty() &&
        relationships.is_empty() &&
        exclude_relationships.is_empty() &&
        groups.is_empty() &&
        exclude_groups.is_empty()
    );
}

Array QueryBuilder::as_array() const {
    Array result;
    result.append(all_components);
    result.append(any_components);
    result.append(none_components);
    result.append(relationships);
    result.append(exclude_relationships);
    result.append(groups);
    result.append(exclude_groups);
    return result;
}

QueryBuilder* QueryBuilder::compile(const String &query) {
    clear();
    
    PackedStringArray tokens = query.split(" ");
    for (int i = 0; i < tokens.size(); i++) {
        String token = tokens[i];
        if (token == "with_all" && i + 1 < tokens.size()) {
            String components_str = tokens[i + 1];
        }
        else if (token == "with_any" && i + 1 < tokens.size()) {
            String components_str = tokens[i + 1];
        }
        else if (token == "with_none" && i + 1 < tokens.size()) {
            String components_str = tokens[i + 1];
        }
    }
    
    UtilityFunctions::print("Query compilation basic implementation: " + query);
    return this;
}