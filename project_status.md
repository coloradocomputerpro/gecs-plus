### Project Status (provided by Google Gemini)

### Plugin Status Summary

The GECS Plus (C++) GDExtension is a **partially complete** port of the original GECS (GDScript) addon. The core architecture and fundamental classes (`Entity`, `Component`, `World`, `System`, `Relationship`) have been successfully implemented, and the basic functionality of adding/removing entities and components, and processing systems works as expected.

However, several advanced features and quality-of-life integrations are currently missing:

  * **Advanced Queries:** The C++ `QueryBuilder` lacks the GDScript version's powerful ability to filter entities based on the *values* of their component properties (e.g., `health > 10`). It can only query for the presence or absence of components.
  * **Editor Debugger:** The entire debugging interface, a major feature of the GDScript addon for inspecting the world state at runtime, has not been implemented.
  * **Sub-System Logic:** The logic for a `System` to process a list of sub-systems is defined in the GDScript version but is only a stub in the C++ implementation.
  * **Helper Utilities:** Features like the custom logger, project settings integration, and the `QueryBuilder` pooling mechanism are absent in the C++ version.

The GDExtension provides a solid foundation of the core ECS pattern but is not yet at full feature parity with its GDScript counterpart.

-----

### `GECS` (GDScript: `_ECS`)

The central singleton for the framework. In C++, this is handled via `GECS::get_singleton()` after being registered in `register_types.cpp`, whereas GDScript uses an autoload singleton.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Properties** | | | |
| `world` | ✅ | ✅ | Implemented. Manages the active world instance. |
| `debug` | ✅ | ✅ | Implemented. |
| `entity_preprocessors`| ✅ | ✅ | Implemented. |
| `entity_postprocessors`| ✅ | ✅ | Implemented. |
| `wildcard` | ✅ | ✅ | Implemented as a `Variant()`. |
| **Signals** | | | |
| `world_changed` | ✅ | ✅ | Implemented. |
| `world_exited` | ✅ | ✅ | Implemented. |
| **Methods** | | | |
| `process()` | ✅ | ✅ | Implemented. |
| `get_components()` | ✅ | ✅ | Implemented. |
| `_on_world_exited()` | ✅ | ✅ | Implemented. |
| **Static Methods** | | | (Moved from `ArrayExtensions.gd`) |
| `intersect()` | ✅ | ✅ | Implemented as a static method on the `GECS` class. |
| `union_arrays()` | ✅ | ✅ | Implemented as `union_arrays` instead of `union`. |
| `difference()` | ✅ | ✅ | Implemented as a static method on the `GECS` class. |
| `topological_sort()` | ✅ | ✅ | Implemented as a static method on the `GECS` class. |

-----

### `Component`

The base class for all data components. The implementation is nearly identical.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Signals** | | | |
| `property_changed` | ✅ | ✅ | Implemented. In C++, it's emitted via the `emit_property_changed` helper method. |
| **Methods** | | | |
| `equals()` | ✅ | ✅ | Implemented. Both versions compare properties to check for equality. |
| `serialize()` | ✅ | ✅ | Implemented. Both versions serialize script properties to a `Dictionary`. |
| **C++ Specific** | | | |
| `_bind_methods()` | N/A | ✅ | Standard GDExtension method binding. |
| `Constructor/Destructor`| N/A | ✅ | Standard C++ constructors and destructors. |

-----

### `Entity`

The container for components. The C++ version successfully ports the core functionality.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Properties** | | | |
| `enabled` | ✅ | ✅ | Implemented. |
| `component_resources` | ✅ | ✅ | Implemented. |
| `_state` dictionary | ✅ | ❌ | The generic state dictionary is not present in the C++ version. |
| **Signals** | | | |
| `component_added` | ✅ | ✅ | Implemented. |
| `component_removed` | ✅ | ✅ | Implemented. |
| `component_property_changed`| ✅ | ✅ | Implemented. |
| `relationship_added` | ✅ | ✅ | Implemented. |
| `relationship_removed` | ✅ | ✅ | Implemented. |
| **Methods** | | | |
| `add_component()` | ✅ | ✅ | Implemented. |
| `add_components()` | ✅ | ✅ | Implemented. |
| `remove_component()` | ✅ | ✅ | Implemented. |
| `remove_components()` | ✅ | ✅ | Implemented. |
| `remove_all_components()`| ✅ | ✅ | Implemented. |
| `deferred_remove_component()`| ✅ | ✅ | Implemented. |
| `get_component()` | ✅ | ✅ | Implemented. |
| `has_component()` | ✅ | ✅ | Implemented. |
| `add_relationship()` | ✅ | ✅ | Implemented. |
| `add_relationships()` | ✅ | ✅ | Implemented. |
| `remove_relationship()`| ✅ | ✅ | Implemented. |
| `remove_relationships()`| ✅ | ✅ | Implemented. |
| `get_relationship()` | ✅ | ✅ | Implemented. |
| `get_relationships()` | ✅ | ✅ | Implemented. |
| `has_relationship()` | ✅ | ✅ | Implemented. |
| `on_ready()` | ✅ | ✅ | Implemented. |
| `on_update()` | ✅ | ✅ | Implemented. |
| `on_destroy()` | ✅ | ✅ | Implemented. |
| `on_disable()` | ✅ | ✅ | Implemented. |
| `on_enable()` | ✅ | ✅ | Implemented. |
| `define_components()` | ✅ | ✅ | Implemented. |
| **C++ Specific** | | | |
| `_notification()` | N/A | ✅ | Handles `NOTIFICATION_READY`. |
| `_bind_methods()` | N/A | ✅ | Standard GDExtension method binding. |

-----

### `Relationship`

Represents a link between entities.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Properties** | | | |
| `relation` | ✅ | ✅ | Implemented. |
| `target` | ✅ | ✅ | Implemented. |
| `source` | ✅ | ✅ | Implemented. |
| **Methods** | | | |
| `_init()` | ✅ | ✅ | Implemented. |
| `matches()` | ✅ | ✅ | Implemented with identical logic for weak and strong matching. |
| `valid()` | ✅ | ⚠️ | C++ version is named `is_valid()`. The implementation is a compromise and does not fully check target validity due to noted difficulties accessing `ObjectDB`. |
| **C++ Specific** | | | |
| `_bind_methods()` | N/A | ✅ | Standard GDExtension method binding. |
| `Constructor/Destructor`| N/A | ✅ | Standard C++ constructors and destructors. |

-----

### `System`

The base class for all logic and behavior.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Properties** | | | |
| `group` | ✅ | ✅ | Implemented. |
| `process_empty` | ✅ | ✅ | Implemented. |
| `active` | ✅ | ✅ | Implemented. |
| `paused` | ✅ | ✅ | Implemented. |
| `q` (QueryBuilder) | ✅ | ✅ | Implemented. |
| **Methods** | | | |
| `deps()` | ✅ | ✅ | Implemented. |
| `query()` | ✅ | ✅ | Implemented. |
| `sub_systems()` | ✅ | ⚠️ | The method exists, but the core logic in `_handle()` to process the sub-systems is missing in the C++ version, making the feature non-functional. |
| `setup()` | ✅ | ✅ | Implemented. |
| `process()` | ✅ | ✅ | Implemented. |
| `process_all()` | ✅ | ✅ | Implemented. |
| `_handle()` | ✅ | ⚠️ | Partially implemented. Does not contain the logic to iterate and process `sub_systems()`. |
| **C++ Specific** | | | |
| `_bind_methods()` | N/A | ✅ | Standard GDExtension method binding. |
| `enum Runs` | N/A | ✅ | The `Runs` enum is defined inside the class scope. |

-----

### `Observer`

A reactive system that responds to component changes.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Properties** | | | |
| `q` (QueryBuilder) | ✅ | ✅ | Implemented. |
| **Methods** | | | |
| `match()` | ✅ | ✅ | Implemented as a virtual method. |
| `watch()` | ✅ | ✅ | Implemented as a virtual method. |
| `on_component_added()` | ✅ | ✅ | Implemented as a virtual method. |
| `on_component_removed()` | ✅ | ✅ | Implemented as a virtual method. |
| `on_component_changed()` | ✅ | ✅ | Implemented as a virtual method. |
| **C++ Specific** | | | |
| `_bind_methods()` | N/A | ✅ | Standard GDExtension method binding. |
| `GDVIRTUAL` Macros | N/A | ✅ | Used to expose virtual methods to Godot. |

-----

### `QueryBuilder`

Used to construct and execute queries for entities.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Methods** | | | |
| `with_all()` | ✅ | ⚠️ | Partially implemented. Does not support advanced dictionary queries for component property values. |
| `with_any()` | ✅ | ⚠️ | Partially implemented. Does not support advanced dictionary queries for component property values. |
| `with_none()` | ✅ | ✅ | Implemented. |
| `with_relationship()` | ✅ | ✅ | Implemented. |
| `without_relationship()`| ✅ | ✅ | Implemented. |
| `with_reverse_relationship()`| ✅ | ⚠️ | Stubbed out. The logic inside the C++ method is incomplete. |
| `with_group()` | ✅ | ✅ | Implemented. |
| `without_group()` | ✅ | ✅ | Implemented. |
| `execute()` | ✅ | ✅ | Implemented. |
| `execute_one()` | ✅ | ✅ | Implemented. |
| `clear()` | ✅ | ✅ | Implemented. |
| `invalidate_cache()` | ✅ | ✅ | Implemented. |
| `is_empty()` | ✅ | ✅ | Implemented. |
| `matches()` | ✅ | ✅ | Implemented. |
| `combine()` | ✅ | ✅ | Implemented. |
| `compile()` | ✅ | ❌ | Stubbed out. The C++ version does not implement query compilation from a string. |

-----

### `World`

Manages all entities, systems, and observers.

| Feature | GDScript Status | C++ Status | Notes |
| :--- | :---: | :---: | :--- |
| **Properties** | | | |
| `entity_nodes_root` | ✅ | ✅ | Implemented. |
| `system_nodes_root` | ✅ | ✅ | Implemented. |
| `query` (getter) | ✅ | ✅ | Implemented via `get_query()`. The GDScript pooling mechanism is not present. |
| **Signals** | | | |
| (All signals) | ✅ | ✅ | All signals (`entity_added`, `entity_removed`, `component_added`, etc.) are implemented. |
| **Methods** | | | |
| `initialize()` | ✅ | ✅ | Implemented. |
| `process()` | ✅ | ✅ | Implemented. |
| `add_entity()` | ✅ | ✅ | Implemented. |
| `add_entities()` | ✅ | ✅ | Implemented. |
| `remove_entity()` | ✅ | ✅ | Implemented. |
| `disable_entity()` | ✅ | ✅ | Implemented. |
| `enable_entity()` | ✅ | ✅ | Implemented. |
| `add_system()` | ✅ | ✅ | Implemented. |
| `add_systems()` | ✅ | ✅ | Implemented. |
| `remove_system()` | ✅ | ✅ | Implemented. |
| `remove_system_group()`| ✅ | ❌ | Not implemented. |
| `add_observer()` | ✅ | ✅ | Implemented. |
| `add_observers()` | ✅ | ✅ | Implemented. |
| `remove_observer()` | ✅ | ✅ | Implemented. |
| `purge()` | ✅ | ✅ | Implemented. |
| `update_pause_state()`| ✅ | ❌ | Not implemented. |
| `_query()` | ✅ | ✅ | Implemented. Caching logic is present. |
| `get_cache_stats()` | ✅ | ✅ | Implemented. |
| `reset_cache_stats()`| ✅ | ✅ | Implemented. |