#pragma once

#include "../databags/databag.h"
#include "../ecs_types.h"
#include "../storage/storage.h"
#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class StorageBase;
class World;

namespace godex {
class Databag;
}

/// Utility that can be used to create an entity with components.
/// You can use it in this way:
/// ```
///	World world;
///
///	world.create_entity()
///			.with(TransformComponent())
///			.with(MeshComponent());
/// ```
class EntityBuilder {
	friend class World;

	EntityID entity;
	World *world;

private:
	EntityBuilder(World *p_world);
	EntityBuilder &operator=(const EntityBuilder &) = delete;
	EntityBuilder &operator=(EntityBuilder) = delete;
	EntityBuilder(const EntityBuilder &) = delete;
	EntityBuilder() = delete;

public:
	template <class C>
	const EntityBuilder &with(const C &p_data) const;
	const EntityBuilder &with(uint32_t p_component_id, const Dictionary &p_data) const;

	template <class C>
	const EntityBuilder &with(godex::SID p_shared_component) const {
		return with(C::get_component_id(), p_shared_component);
	}
	const EntityBuilder &with(uint32_t p_component_id, godex::SID p_shared_component) const;

	operator EntityID() const {
		return entity;
	}
};

// TODO make this under godex namespace.

// TODO consider to split this in multiple `Databag`, one for removal and the
// other for creation. So two `Systems` can run in parallel.
class WorldCommands : public godex::Databag {
	DATABAG(WorldCommands)

	friend class World;

	/// Used to keep tracks of entity IDs.
	uint32_t entity_register = 0;

	/// List of `Entity` to destroy.
	LocalVector<EntityID> garbage_list;

	static void _bind_methods();

public:
	/// Immediately creates a new `Entity`.
	EntityID create_entity();

	/// Mark this `Entity` for disposal.
	void destroy_deferred(EntityID p_entity);
};

// IMPORTANT, when multithreading is implemented, all the `System`s asking for
// the world must run in single thread.
class World : public godex::Databag {
	DATABAG(World)

	friend class Pipeline;

	WorldCommands commands;
	LocalVector<StorageBase *> storages;
	LocalVector<godex::Databag *> databags;
	EntityBuilder entity_builder = EntityBuilder(this);
	bool is_dispatching_in_progress = false;

	/// Storages configuration, the format is as follows:
	/// {"Component Name" :{"param_1": 11, "param_2": 11},
	///  "Component Name" :{"param_1": 11, "param_2": 11},
	///  "Component Name" :{"param_1": 11, "param_2": 11}}
	Dictionary storages_config;

	static void _bind_methods();

public:
	World();
	~World();

	/// Creates a new Entity id. You can add the components using the function
	/// `add_component`.
	EntityID create_entity_index();

	/// Creates a new Entity id and returns an `EntityBuilder`.
	/// You can use the `EntityBuilder` in this way:
	/// ```
	///	World world;
	///
	///	world.create_entity()
	///			.with(TransformComponent())
	///			.with(MeshComponent());
	/// ```
	///
	/// It's possible to get the `EntityID` just by assining the `EntityBuilder`
	/// to an `EntityID`.
	/// ```
	///	EntityID entity = world.create_entity()
	///			.with(MeshComponent());
	/// ```
	///
	/// Note: The `EntityBuilder` reference points to an internal variable.
	/// It's undefined behavior use it in any other way than the above one.
	const EntityBuilder &create_entity();

	/// Remove the entity from this World.
	void destroy_entity(EntityID p_entity);

	WorldCommands &get_commands();
	const WorldCommands &get_commands() const;

	/// Flushes every pending action.
	void flush();

	/// Adds a new component (or sets the default if already exists) to a
	/// specific Entity.
	template <class C>
	void add_component(EntityID p_entity, const C &p_data);

	template <class C>
	void remove_component(EntityID p_entity);

	template <class C>
	bool has_component(EntityID p_entity) const;

	/// Adds a new component using the component id and  a `Dictionary` that
	/// contains the initialization parameters.
	/// Usually this function is used to initialize the script components.
	void add_component(EntityID p_entity, uint32_t p_component_id, const Dictionary &p_data);
	void remove_component(EntityID p_entity, uint32_t p_component_id);
	bool has_component(EntityID p_entity, uint32_t p_component_id) const;

	template <class C>
	godex::SID create_shared_component(const C &p_component);
	godex::SID create_shared_component(uint32_t p_component_id, const Dictionary &p_component_data);
	void add_shared_component(EntityID p_entity, uint32_t p_component_id, godex::SID p_shared_component_id);

	/// Returns the const storage pointed by the give ID.
	const StorageBase *get_storage(uint32_t p_storage_id) const;

	/// Returns the storage pointed by the given component ID.
	StorageBase *get_storage(uint32_t p_storage_id);

	/// Returns the constant storage pointer.
	/// If the storage doesn't exist, returns null.
	/// If the type is wrong, this function crashes.
	template <class C>
	const Storage<const C> *get_storage() const;

	/// Returns the storage pointer.
	/// If the storage doesn't exist, returns null.
	/// If the type is wrong, this function crashes.
	template <class C>
	Storage<C> *get_storage();

	/// Returns the storage pointed by the given component ID.
	/// If the storage doesn't exist, or it's not shared, returns null.
	SharedStorageBase *get_shared_storage(uint32_t p_storage_id);

	/// Returns the storage pointed by the given component ID.
	/// If the storage doesn't exist, or it's not shared, returns null.
	const SharedStorageBase *get_shared_storage(uint32_t p_storage_id) const;

	/// Returns the storage pointer.
	/// If the storage doesn't exist, returns null.
	/// If the type is wrong, this function crashes.
	template <class C>
	SharedStorage<C> *get_shared_storage();

	/// Returns the storage pointer.
	/// If the storage doesn't exist, returns null.
	/// If the type is wrong, this function crashes.
	template <class C>
	const SharedStorage<const C> *get_shared_storage() const;

	/// Adds a new databag or does nothing.
	template <class R>
	R &create_databag();

	void create_databag(godex::databag_id p_id);

	template <class R>
	void remove_databag();

	void remove_databag(godex::databag_id p_id);

	/// Retuns a databag pointer.
	template <class R>
	R *get_databag();

	/// Retuns a databag pointer.
	template <class R>
	const R *get_databag() const;

	/// Retuns a databag pointer.
	godex::Databag *get_databag(godex::databag_id p_id);

	/// Retuns a databag pointer.
	const godex::Databag *get_databag(godex::databag_id p_id) const;

private:
	/// Creates a new component storage into the world, if the storage
	/// already exists, does nothing.
	template <class C>
	void create_storage();
	void create_storage(uint32_t p_component_id);

	/// Destroy a component storage if exists.
	// TODO when this is called?
	template <class C>
	void destroy_storage();
	void destroy_storage(uint32_t p_component_id);
};

template <class C>
const EntityBuilder &EntityBuilder::with(const C &p_data) const {
	world->add_component(entity, p_data);
	return *this;
}

template <class C>
void World::add_component(EntityID p_entity, const C &p_data) {
	create_storage<C>();
	Storage<C> *storage = get_storage<C>();
	ERR_FAIL_COND(storage == nullptr);
	storage->insert(p_entity, p_data);
}

template <class C>
void World::remove_component(EntityID p_entity) {
	remove_component(p_entity, C::get_component_id());
}

template <class C>
bool World::has_component(EntityID p_entity) const {
	return has_component(p_entity, C::get_component_id());
}

template <class C>
godex::SID World::create_shared_component(const C &p_component_data) {
	create_storage<C>();
	SharedStorage<C> *storage = get_shared_storage<C>();
	ERR_FAIL_COND_V_MSG(storage == nullptr, godex::SID_NONE, "The storage is not supposed to be `nullptr` at this point.");
	return storage->create_shared_component(p_component_data);
}

template <class C>
const Storage<const C> *World::get_storage() const {
	const uint32_t id = C::get_component_id();

	if (id >= storages.size()) {
		return nullptr;
	}

	return static_cast<Storage<const C> *>(storages[id]);
}

template <class C>
Storage<C> *World::get_storage() {
	const uint32_t id = C::get_component_id();

	if (id >= storages.size()) {
		return nullptr;
	}

	return static_cast<Storage<C> *>(storages[id]);
}

template <class C>
SharedStorage<C> *World::get_shared_storage() {
	const uint32_t id = C::get_component_id();

	if (id >= storages.size()) {
		return nullptr;
	}

	return static_cast<SharedStorage<C> *>(storages[id]);
}

template <class C>
const SharedStorage<const C> *World::get_shared_storage() const {
	const uint32_t id = C::get_component_id();

	if (id >= storages.size()) {
		return nullptr;
	}

	return static_cast<const SharedStorage<const C> *>(storages[id]);
}

template <class R>
R &World::create_databag() {
	const godex::databag_id id = R::get_databag_id();

	create_databag(id);
	// This function is never supposed to fail.
	CRASH_COND_MSG(databags[id] == nullptr, "The function `create_databag` is not supposed to fail, is this databag registered?");

	return *static_cast<R *>(databags[id]);
}

template <class R>
void World::remove_databag() {
	const godex::databag_id id = R::get_databag_id();
	remove_databag(id);
}

template <class C>
void World::create_storage() {
	create_storage(C::get_component_id());
}

template <class C>
void World::destroy_storage() {
	destroy_storage(C::get_component_id());
}

template <class R>
R *World::get_databag() {
	const godex::databag_id id = R::get_databag_id();
	godex::Databag *r = get_databag(id);

	if (unlikely(r == nullptr)) {
		return nullptr;
	}

	return static_cast<R *>(r);
}

template <class R>
const R *World::get_databag() const {
	const godex::databag_id id = R::get_databag_id();
	const godex::Databag *r = get_databag(id);

	if (unlikely(r == nullptr)) {
		return nullptr;
	}

	return static_cast<const R *>(r);
}
