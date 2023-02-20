#pragma once

#include "../toy_platform.h"
#include "../toy_allocator.h"
#include <stddef.h>
#include <stdint.h>


enum toy_scene_component_type_t {
	TOY_SCENE_COMPONENT_TYPE_LOCATION = 0,
	TOY_SCENE_COMPONENT_TYPE_OBJECT_ID,
	TOY_SCENE_COMPONENT_TYPE_TREE_NODE,
	TOY_SCENE_COMPONENT_TYPE_MESH,
};


typedef struct toy_scene_entity_chunk_descriptor_t toy_scene_entity_chunk_descriptor_t;
typedef struct toy_scene_entity_chunk_header_t toy_scene_entity_chunk_header_t;

typedef struct toy_entity_ref_t {
	struct toy_scene_entity_chunk_header_t* chunk;
	uint16_t index;
}toy_entity_ref_t;

struct toy_scene_entity_chunk_header_t {
	struct toy_scene_entity_chunk_descriptor_t* chunk_desc;
	uint16_t entity_count;
	struct toy_scene_entity_chunk_header_t* prev;
	struct toy_scene_entity_chunk_header_t* next;
};

typedef ptrdiff_t (*toy_get_component_offset_fp) (
	toy_scene_entity_chunk_descriptor_t* chunk_desc,
	enum toy_scene_component_type_t type);

typedef void (*toy_create_scene_entity_fp) (
	toy_scene_entity_chunk_header_t* chunk,
	uint16_t index,
	uint32_t object_id);

typedef uint32_t (*toy_destroy_scene_entity_fp) (
	toy_scene_entity_chunk_header_t* chunk,
	uint16_t index);

typedef uint32_t (*toy_move_scene_entity_fp) (
	toy_scene_entity_chunk_header_t* chunk,
	uint16_t src_index,
	uint16_t dst_index);

typedef toy_scene_entity_chunk_header_t* (*toy_create_entity_chunk_fp) (
	toy_scene_entity_chunk_descriptor_t* chunk_desc,
	const toy_allocator_t* chunk_alc);

typedef void (*toy_destroy_entity_chunk_fp) (
	toy_scene_entity_chunk_header_t* chunk,
	const toy_allocator_t* chunk_alc);

struct toy_scene_entity_chunk_descriptor_t {
	struct toy_scene_entity_chunk_descriptor_t* next;
	void* context;
	uint16_t max_entity_count; // max entity count per chunk
	uint32_t chunk_count;
	toy_get_component_offset_fp get_comp_offset;
	toy_create_entity_chunk_fp create_chunk;
	toy_destroy_entity_chunk_fp destroy_chunk;
	toy_create_scene_entity_fp create_entity;
	toy_destroy_scene_entity_fp destroy_entity;
	toy_move_scene_entity_fp move_entity;
	struct toy_scene_entity_chunk_header_t* first_chunk;
};


TOY_EXTERN_C_START

toy_entity_ref_t toy_create_scene_chunk_entity (
	toy_scene_entity_chunk_descriptor_t* chunk_desc,
	const toy_allocator_t* chunk_alc,
	uint32_t object_id
);

toy_inline void toy_destroy_scene_chunk_entity (toy_entity_ref_t* ref) {
	ref->chunk->chunk_desc->destroy_entity(ref->chunk, ref->index);
}


toy_inline ptrdiff_t toy_get_scene_chunk_offset (toy_scene_entity_chunk_descriptor_t* desc, enum toy_scene_component_type_t type) {
	return desc->get_comp_offset(desc, type);
}


toy_inline void* toy_get_scene_chunk_offset2 (toy_entity_ref_t* ref, enum toy_scene_component_type_t type) {
	return (void*)((uintptr_t)ref->chunk + toy_get_scene_chunk_offset(ref->chunk->chunk_desc, type));
}

TOY_EXTERN_C_END
