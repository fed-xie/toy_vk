#pragma once

#include "toy_platform.h"
#include "toy_allocator.h"
#include <stdint.h>


typedef struct toy_asset_pool_t toy_asset_pool_t, *toy_asset_pool_p;

typedef void (*toy_destroy_asset_fp) (toy_asset_pool_p asset_pool, void* asset);


typedef struct toy_asset_pool_chunk_t toy_asset_pool_chunk_t, *toy_asset_pool_chunk_p;
typedef struct toy_asset_pool_t {
	toy_asset_pool_chunk_p* chunks;
	uint32_t chunk_count;
	uint32_t chunk_block_count; // block count of each chunk, NOT total count
	size_t asset_size;
	size_t asset_alignment;
	toy_allocator_t chunk_alc;
	toy_allocator_t alc;
	toy_destroy_asset_fp destroy_fp;
	void* context;
	const char* literal_name; // Just a pointer, not a copy
}toy_asset_pool_t, *toy_asset_pool_p;


typedef struct toy_asset_pool_item_ref_t {
	toy_asset_pool_p pool;
	uint32_t index;
	uint32_t next_ref;
}toy_asset_pool_item_ref_t;

typedef struct toy_asset_item_ref_pool_t {
	toy_asset_pool_item_ref_t* refs;
	uint32_t pool_length;
	uint32_t next_block;
	toy_allocator_t alc;
}toy_asset_item_ref_pool_t;


typedef struct toy_asset_pool_data_ref_t {
	void* data;
	uint32_t ref_count;
	uint32_t next;
}toy_asset_pool_data_ref_t;

typedef struct toy_asset_data_ref_pool_t {
	toy_asset_pool_data_ref_t* refs;
	uint32_t pool_length;
	uint32_t next_block;
	toy_allocator_t alc;
}toy_asset_data_ref_pool_t;


typedef struct toy_stage_data_block_t {
	const void* data;
	size_t size;
	size_t alignment;
}toy_stage_data_block_t;


enum toy_vertex_attribute_slot_t {
	TOY_VERTEX_ATTRIBUTE_SLOT_POSITION = 0,
	TOY_VERTEX_ATTRIBUTE_SLOT_NORMAL,
	TOY_VERTEX_ATTRIBUTE_SLOT_TEXCOORD,
	TOY_VERTEX_ATTRIBUTE_SLOT_MAX,
};

struct toy_vertex_attribute_slot_descriptor_t {
	enum toy_vertex_attribute_slot_t slot;
	uint16_t stride; // Stride for per vertex
	uint16_t offset;
};

typedef struct toy_vertex_attribute_descriptor_t {
	uint32_t slot_count;
	uint32_t stride; // Stride per vertex
	const struct toy_vertex_attribute_slot_descriptor_t* slot_descs;
}toy_vertex_attribute_descriptor_t;


typedef struct toy_host_mesh_primitive_t {
	const void* attributes;
	size_t attribute_size;
	const toy_vertex_attribute_descriptor_t* attr_desc;

	const void* indices;
	size_t index_size;

	uint32_t vertex_count;
	uint32_t index_count;
}toy_host_mesh_primitive_t;


typedef struct toy_mesh_t {
	uint32_t primitive_index;
	uint32_t material_index;
}toy_mesh_t;



enum toy_image_sampler_filter_t {
	TOY_IMAGE_SAMPLER_FILTER_NEAREST = 0x1,
	TOY_IMAGE_SAMPLER_FILTER_LINEAR = 0x3,
	TOY_IMAGE_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST = (TOY_IMAGE_SAMPLER_FILTER_NEAREST << 4) | TOY_IMAGE_SAMPLER_FILTER_NEAREST,
	TOY_IMAGE_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST = (TOY_IMAGE_SAMPLER_FILTER_LINEAR << 4) | TOY_IMAGE_SAMPLER_FILTER_NEAREST,
	TOY_IMAGE_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR = (TOY_IMAGE_SAMPLER_FILTER_NEAREST << 4) | TOY_IMAGE_SAMPLER_FILTER_LINEAR,
	TOY_IMAGE_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR = (TOY_IMAGE_SAMPLER_FILTER_LINEAR << 4) | TOY_IMAGE_SAMPLER_FILTER_LINEAR,
};

enum toy_image_sampler_wrap_t {
	TOY_IMAGE_SAMPLER_WRAP_REPEAT,
	TOY_IMAGE_SAMPLER_WRAP_MIRRORED_REPEAT,
	TOY_IMAGE_SAMPLER_WRAP_CLAMP_TO_EDGE,
#if TOY_DRIVER_VULKAN
	TOY_IMAGE_SAMPLER_WRAP_CLAMP_TO_BORDER, // Vulkan only
	TOY_IMAGE_SAMPLER_WRAP_MIRROR_CLAMP_TO_EDGE, // Vulkan only
#endif
};

typedef struct toy_image_sampler_t {
	enum toy_image_sampler_filter_t mag_filter; // NEAREST | LINEAR only
	enum toy_image_sampler_filter_t min_filter;
	enum toy_image_sampler_wrap_t wrap_u;
	enum toy_image_sampler_wrap_t wrap_v;
	enum toy_image_sampler_wrap_t wrap_w;
}toy_image_sampler_t;



TOY_EXTERN_C_START

void toy_init_asset_pool (
	size_t asset_size,
	size_t asset_alignment,
	toy_destroy_asset_fp destroy_asset_fp,
	const toy_allocator_t* chunk_alc,
	const toy_allocator_t* alc,
	void* context,
	const char* literal_name,
	toy_asset_pool_t* output
);

void toy_destroy_asset_pool (
	const toy_allocator_t* alc,
	toy_asset_pool_p pool
);

void* toy_get_asset_item (toy_asset_pool_p pool, uint32_t index);

toy_inline void* toy_get_asset_item2 (toy_asset_pool_item_ref_t* ref) {
	return toy_get_asset_item(ref->pool, ref->index);
}

uint32_t toy_alloc_asset_item (
	toy_asset_pool_p pool,
	toy_error_t* error
);

// Free asset item
void toy_raw_free_asset_item (
	toy_asset_pool_p pool,
	uint32_t index
);

// Destroy and free asset item
void toy_free_asset_item (
	toy_asset_pool_p pool,
	uint32_t index
);

uint32_t toy_add_asset_ref (toy_asset_pool_p pool, uint32_t index, uint32_t ref_count);

uint32_t toy_sub_asset_ref (toy_asset_pool_p pool, uint32_t index, uint32_t ref_count);

uint32_t toy_get_asset_ref (toy_asset_pool_p pool, uint32_t index);

void toy_create_asset_item_ref_pool (
	uint32_t initial_length,
	const toy_allocator_t* alc,
	toy_asset_item_ref_pool_t* output,
	toy_error_t* error
);

void toy_destroy_asset_ref_pool (toy_asset_item_ref_pool_t* ref_pool);

// return UINT32_MAX when failed
uint32_t toy_alloc_asset_item_ref (toy_asset_item_ref_pool_t* ref_pool);

void toy_free_asset_item_ref (toy_asset_item_ref_pool_t* ref_pool, uint32_t index);

toy_inline toy_asset_pool_item_ref_t* toy_get_asset_item_ref (toy_asset_item_ref_pool_t* ref_pool, uint32_t index) {
	return &ref_pool->refs[index];
}

toy_inline toy_asset_pool_item_ref_t* toy_get_next_asset_item_ref (toy_asset_item_ref_pool_t* ref_pool, toy_asset_pool_item_ref_t* ref) {
	return ref->next_ref >= ref_pool->pool_length ? NULL : &ref_pool->refs[ref->next_ref];
}

TOY_EXTERN_C_END
