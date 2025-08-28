#ifndef QUERY_BUILDER_H
#define QUERY_BUILDER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {

class World;
class Entity;
class Component;
class Relationship;

class QueryBuilder : public RefCounted {
    GDCLASS(QueryBuilder, RefCounted)

private:
    World* world;
    Array all_components;
    Array any_components;
    Array none_components;
    Array relationships;
    Array exclude_relationships;
    Array groups;
    Array exclude_groups;
    Array all_components_queries;
    Array any_components_queries;

    bool cache_valid = false;
    Array cached_result;

    Array _internal_execute();

protected:
    static void _bind_methods();
    
    GDVIRTUAL0R(Array, execute)
    GDVIRTUAL1R(Array, matches, const Array &)
    GDVIRTUAL1R(Ref<QueryBuilder>, combine, const Ref<QueryBuilder> &)

public:
    QueryBuilder();
    ~QueryBuilder();

    void _init(World* p_world);

    QueryBuilder* with_all(const Array &p_components);
    QueryBuilder* with_any(const Array &p_components);
    QueryBuilder* with_none(const Array &p_components);
    QueryBuilder* with_relationship(const Array &p_relationships);
    QueryBuilder* without_relationship(const Array &p_relationships);
    QueryBuilder* with_group(const Array &p_groups);
    QueryBuilder* without_group(const Array &p_groups);
    QueryBuilder* with_reverse_relationship(const Array &p_relationships);
    
    QueryBuilder* clear();
    virtual Ref<QueryBuilder> combine(const Ref<QueryBuilder> &other);
    virtual Array matches(const Array &p_entities);
    void invalidate_cache();

    virtual Array execute();
    Object* execute_one();

    bool is_empty() const;
    Array as_array() const;
    QueryBuilder* compile(const String &query);
};

}

#endif