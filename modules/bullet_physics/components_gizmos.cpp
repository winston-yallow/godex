#include "components_gizmos.h"

#include "../godot/nodes/entity.h"
#include "debug_utilities.h"

void BtShapeBoxGizmo::init() {
	const Color gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/shape", Color(0.5, 0.7, 1));
	create_material("shape_material", gizmo_color);

	const float gizmo_value = gizmo_color.get_v();
	const Color gizmo_color_disabled = Color(gizmo_value, gizmo_value, gizmo_value, 0.65);
	create_material("shape_material_disabled", gizmo_color_disabled);

	create_handle_material("handles");
}

void BtShapeBoxGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(box_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name);

		Vector<Vector3> lines;
		AABB aabb;
		aabb.position = -half_extents;
		aabb.size = half_extents * 2.0;

		for (int i = 0; i < 12; i++) {
			Vector3 a, b;
			aabb.get_edge(i, a, b);
			lines.push_back(a);
			lines.push_back(b);
		}

		Vector<Vector3> handles;

		for (int i = 0; i < 3; i++) {
			Vector3 ax;
			ax[i] = half_extents[i];
			handles.push_back(ax);
		}

		p_gizmo->add_lines(lines, material);
		p_gizmo->add_collision_segments(lines);
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeBoxGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(box_component_name)) {
		return 3;
	}

	return 0;
}

String BtShapeBoxGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "half_extents_x";
	} else if (p_idx == 1) {
		return "half_extents_y";
	} else {
		return "half_extents_z";
	}
}

Variant BtShapeBoxGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(box_component_name) == false, Variant());

	const Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name, Space::GLOBAL);
	ERR_FAIL_COND_V_MSG((p_idx < 0) || p_idx > 2, Variant(), "The index " + itos(p_idx) + "is invalid. It should go from 0 to 2 (inclusive).");
	return half_extents[p_idx];
}

void BtShapeBoxGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(box_component_name) == false);

	Transform gt = entity->get_global_transform();
	Transform gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = ra[p_idx];
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name);
	half_extents[p_idx] = d;

	entity->set_component_value(box_component_name, half_extents_name, half_extents);
}

void BtShapeBoxGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(box_component_name) == false);

	Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name);

	if (p_cancel) {
		entity->set_component_value(box_component_name, half_extents_name, p_restore);
	} else {
		UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();
		ur->create_action(TTR("Change Shape Box Half Extent"));
		ur->add_do_method(entity, "set_component_value", box_component_name, half_extents_name, half_extents);
		ur->add_undo_method(entity, "set_component_value", box_component_name, half_extents_name, p_restore);
		ur->commit_action();
	}
}

void BtShapeSphereGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtShapeSphereGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(sphere_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(sphere_component_name, radius_name);

		Vector<Vector3> points;

		for (int i = 0; i <= 360; i++) {
			float ra = Math::deg2rad((float)i);
			float rb = Math::deg2rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y));
			points.push_back(Vector3(b.x, 0, b.y));
			points.push_back(Vector3(0, a.x, a.y));
			points.push_back(Vector3(0, b.x, b.y));
			points.push_back(Vector3(a.x, a.y, 0));
			points.push_back(Vector3(b.x, b.y, 0));
		}

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y));
			collision_segments.push_back(Vector3(b.x, 0, b.y));
			collision_segments.push_back(Vector3(0, a.x, a.y));
			collision_segments.push_back(Vector3(0, b.x, b.y));
			collision_segments.push_back(Vector3(a.x, a.y, 0));
			collision_segments.push_back(Vector3(b.x, b.y, 0));
		}

		p_gizmo->add_lines(points, material);
		p_gizmo->add_collision_segments(collision_segments);
		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, 0, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeSphereGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(sphere_component_name)) {
		return 1;
	}

	return 0;
}

String BtShapeSphereGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "radius";
}

Variant BtShapeSphereGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(sphere_component_name) == false, Variant());

	return entity->get_component_value(sphere_component_name, radius_name);
}

void BtShapeSphereGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(sphere_component_name) == false);

	Transform gt = entity->get_global_transform();
	Transform gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), Vector3(4096, 0, 0), sg[0], sg[1], ra, rb);
	float d = ra.x;
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	entity->set_component_value(sphere_component_name, radius_name, d);
}

void BtShapeSphereGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(sphere_component_name) == false);

	const real_t radius = entity->get_component_value(sphere_component_name, radius_name);

	if (p_cancel) {
		entity->set_component_value(sphere_component_name, radius_name, p_restore);
	} else {
		UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();
		ur->create_action(TTR("Change Shape Sphere Radius"));
		ur->add_do_method(entity, "set_component_value", sphere_component_name, radius_name, radius);
		ur->add_undo_method(entity, "set_component_value", sphere_component_name, radius_name, p_restore);
		ur->commit_action();
	}
}

void BtShapeCapsuleGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtShapeCapsuleGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(capsule_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(capsule_component_name, radius_name);
		const real_t height = entity->get_component_value(capsule_component_name, height_name);

		Vector<Vector3> points;

		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg2rad((float)i);
			float rb = Math::deg2rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y) + d);
			points.push_back(Vector3(b.x, 0, b.y) + d);

			points.push_back(Vector3(a.x, 0, a.y) - d);
			points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				points.push_back(Vector3(a.x, 0, a.y) + d);
				points.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 180 ? d : -d;

			points.push_back(Vector3(0, a.x, a.y) + dud);
			points.push_back(Vector3(0, b.x, b.y) + dud);
			points.push_back(Vector3(a.y, a.x, 0) + dud);
			points.push_back(Vector3(b.y, b.x, 0) + dud);
		}

		p_gizmo->add_lines(points, material);

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) + d);

			collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 16 == 0) {
				collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 32 ? d : -d;

			collision_segments.push_back(Vector3(0, a.x, a.y) + dud);
			collision_segments.push_back(Vector3(0, b.x, b.y) + dud);
			collision_segments.push_back(Vector3(a.y, a.x, 0) + dud);
			collision_segments.push_back(Vector3(b.y, b.x, 0) + dud);
		}

		p_gizmo->add_collision_segments(collision_segments);

		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, 0, 0));
		handles.push_back(Vector3(0, height * 0.5 + radius, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeCapsuleGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(capsule_component_name)) {
		return 2;
	}

	return 0;
}

String BtShapeCapsuleGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "radius";
	} else {
		return "height";
	}
}

Variant BtShapeCapsuleGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(capsule_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(capsule_component_name, radius_name);
	} else {
		return entity->get_component_value(capsule_component_name, height_name);
	}
}

void BtShapeCapsuleGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(capsule_component_name) == false);

	Transform gt = entity->get_global_transform();
	Transform gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = axis.dot(ra);
	if (p_idx == 1) {
		const real_t radius = entity->get_component_value(capsule_component_name, radius_name);
		d -= radius;
	}

	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(capsule_component_name, radius_name, d);
	} else {
		entity->set_component_value(capsule_component_name, height_name, d * 2.0);
	}
}

void BtShapeCapsuleGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(capsule_component_name) == false);

	const real_t v = entity->get_component_value(capsule_component_name, p_idx == 0 ? radius_name : height_name);

	if (p_cancel) {
		entity->set_component_value(capsule_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
	} else {
		UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();
		ur->create_action(TTR("Change Shape Capsule Radius"));
		ur->add_do_method(entity, "set_component_value", capsule_component_name, p_idx == 0 ? radius_name : height_name, v);
		ur->add_undo_method(entity, "set_component_value", capsule_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
		ur->commit_action();
	}
}

void BtShapeConeGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtShapeConeGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(cone_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(cone_component_name, radius_name);
		const real_t height = entity->get_component_value(cone_component_name, height_name);

		Vector<Vector3> points;

		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg2rad((float)i);
			float rb = Math::deg2rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y) - d);
			points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				points.push_back(Vector3(a.x, 0, a.y) - d);
				points.push_back(d);
			}
		}

		p_gizmo->add_lines(points, material);

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 16 == 0) {
				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
				collision_segments.push_back(d);
			}
		}

		p_gizmo->add_collision_segments(collision_segments);

		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, -height * 0.5, 0));
		handles.push_back(Vector3(0, height * 0.5, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeConeGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(cone_component_name)) {
		return 2;
	}

	return 0;
}

String BtShapeConeGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "radius";
	} else {
		return "height";
	}
}

Variant BtShapeConeGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(cone_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(cone_component_name, radius_name);
	} else {
		return entity->get_component_value(cone_component_name, height_name);
	}
}

void BtShapeConeGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cone_component_name) == false);

	Transform gt = entity->get_global_transform();
	Transform gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = axis.dot(ra);

	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(cone_component_name, radius_name, d);
	} else {
		entity->set_component_value(cone_component_name, height_name, d * 2.0);
	}
}

void BtShapeConeGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cone_component_name) == false);

	const real_t v = entity->get_component_value(cone_component_name, p_idx == 0 ? radius_name : height_name);

	if (p_cancel) {
		entity->set_component_value(cone_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
	} else {
		UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();
		ur->create_action(TTR("Change Shape Cone Radius"));
		ur->add_do_method(entity, "set_component_value", cone_component_name, p_idx == 0 ? radius_name : height_name, v);
		ur->add_undo_method(entity, "set_component_value", cone_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
		ur->commit_action();
	}
}

void BtShapeCylinderGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtShapeCylinderGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(cylinder_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(cylinder_component_name, radius_name);
		const real_t height = entity->get_component_value(cylinder_component_name, height_name);

		Vector<Vector3> points;

		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg2rad((float)i);
			float rb = Math::deg2rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y) + d);
			points.push_back(Vector3(b.x, 0, b.y) + d);

			points.push_back(Vector3(a.x, 0, a.y) - d);
			points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				points.push_back(Vector3(a.x, 0, a.y) + d);
				points.push_back(Vector3(a.x, 0, a.y) - d);

				points.push_back(Vector3(a.x, 0, a.y) + d);
				points.push_back(d);

				points.push_back(Vector3(a.x, 0, a.y) - d);
				points.push_back(-d);
			}
		}

		p_gizmo->add_lines(points, material);

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) + d);

			collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 16 == 0) {
				collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);

				collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
				collision_segments.push_back(d);

				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
				collision_segments.push_back(-d);
			}
		}

		p_gizmo->add_collision_segments(collision_segments);

		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, 0, 0));
		handles.push_back(Vector3(0, height * 0.5, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeCylinderGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(cylinder_component_name)) {
		return 2;
	}

	return 0;
}

String BtShapeCylinderGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "radius";
	} else {
		return "height";
	}
}

Variant BtShapeCylinderGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(cylinder_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(cylinder_component_name, radius_name);
	} else {
		return entity->get_component_value(cylinder_component_name, height_name);
	}
}

void BtShapeCylinderGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cylinder_component_name) == false);

	Transform gt = entity->get_global_transform();
	Transform gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = axis.dot(ra);
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(cylinder_component_name, radius_name, d);
	} else {
		entity->set_component_value(cylinder_component_name, height_name, d * 2.0);
	}
}

void BtShapeCylinderGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cylinder_component_name) == false);

	const real_t v = entity->get_component_value(cylinder_component_name, p_idx == 0 ? radius_name : height_name);

	if (p_cancel) {
		entity->set_component_value(cylinder_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
	} else {
		UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();
		ur->create_action(TTR("Change Shape Capsule Radius"));
		ur->add_do_method(entity, "set_component_value", cylinder_component_name, p_idx == 0 ? radius_name : height_name, v);
		ur->add_undo_method(entity, "set_component_value", cylinder_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
		ur->commit_action();
	}
}

class DebugMeshComponentGizmoData : public ComponentGizmoData {
public:
	// Cache data.
	Ref<ArrayMesh> mesh;
};

void BtShapeConvexGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtShapeConvexGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(convex_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);

		// Check the cache.
		Ref<ArrayMesh> mesh;
		Ref<ComponentGizmoData> *d = entity->get_internal_entity().gizmo_data.lookup_ptr(convex_component_name);
		if (d) {
			Ref<DebugMeshComponentGizmoData> td = *d;
			if (td.is_valid()) {
				mesh = td->mesh;
			}
		}

		if (mesh.is_null()) {
			const Vector<Vector3> points = entity->get_component_value(convex_component_name, points_name);
			mesh = generate_mesh_from_points(points);

			// Cache this
			entity->get_internal_entity().gizmo_data.insert(convex_component_name, mesh);
		}

		p_gizmo->add_mesh(mesh, false, Ref<SkinReference>(), material);
	}
}

int BtShapeConvexGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	return 0;
}

String BtShapeConvexGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant BtShapeConvexGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void BtShapeConvexGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
}

void BtShapeConvexGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
}

void BtShapeTrimeshGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtShapeTrimeshGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(trimesh_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);

		// Check the cache.
		Ref<ArrayMesh> mesh;
		Ref<ComponentGizmoData> *d = entity->get_internal_entity().gizmo_data.lookup_ptr(trimesh_component_name);
		if (d) {
			Ref<DebugMeshComponentGizmoData> td = *d;
			if (td.is_valid()) {
				mesh = td->mesh;
			}
		}

		if (mesh.is_null()) {
			const Vector<Vector3> faces = entity->get_component_value(trimesh_component_name, faces_name);
			mesh = generate_mesh_from_faces(faces);

			// Cache this
			entity->get_internal_entity().gizmo_data.insert(trimesh_component_name, mesh);
		}

		p_gizmo->add_mesh(mesh, false, Ref<SkinReference>(), material);
	}
}

int BtShapeTrimeshGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	return 0;
}

String BtShapeTrimeshGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant BtShapeTrimeshGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void BtShapeTrimeshGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
}

void BtShapeTrimeshGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
}
