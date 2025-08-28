#ifndef SYSTEM_H
#define SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

class QueryBuilder;
class Entity;

class System : public Node {
    GDCLASS(System, Node)

public:
    enum Runs {
        Before,
        After,
    };

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
private:
    String group;
    bool process_empty = false;
    bool active = true;
    int order = 0;
    bool paused = false;
    Ref<QueryBuilder> q;

public:
    System();
    ~System();
    
    void _handle(double delta);
    
    void set_group(const String &p_group);
    String get_group() const;
    void set_process_empty(bool p_process_empty);
    bool get_process_empty() const;
    void set_active(bool p_active);
    bool get_active() const;
    void set_order(int p_order);
    int get_order() const;
    void set_paused(bool p_paused);
    bool get_paused() const;
    
    void set_q(const Ref<QueryBuilder> &p_q);
    Ref<QueryBuilder> get_q();

    Dictionary deps();
    Ref<QueryBuilder> query();
    Array sub_systems();
    void setup();
    void process(Entity *entity, double delta);
    bool process_all(const Array &entities, double delta);
};

}

VARIANT_ENUM_CAST(System::Runs);

#endif