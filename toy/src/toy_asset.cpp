#include "include/toy_asset.h"

#include "toy_assert.h"
#include "include/toy_log.h"
#include <atomic>

struct toy_asset_pool_chunk_t {
	uint32_t next_free_block_index;
	uint32_t allocated_block_count;

	std::atomic_uint32_t* ref_counts;
	uintptr_t data_area;
};

TOY_EXTERN_C_START

static toy_asset_pool_chunk_p toy_alloc_asset_chunk (
	toy_asset_pool_t* pool,
	const toy_allocator_t* chunk_alc)
{
	toy_asset_pool_chunk_p chunk = (toy_asset_pool_chunk_p)toy_alloc(chunk_alc, TOY_MEMORY_CHUNK_SIZE);
	if (NULL == chunk)
		return NULL;

	chunk->next_free_block_index = 0;
	chunk->allocated_block_count = 0;
	chunk->ref_counts = (std::atomic_uint32_t*)((uintptr_t)chunk + sizeof(toy_asset_pool_chunk_t));
	chunk->data_area = (uintptr_t)chunk->ref_counts + sizeof(*(chunk->ref_counts)) * pool->chunk_block_count;
	if (pool->asset_alignment > 0) {
		size_t mask = pool->asset_alignment - 1;
		size_t padding = pool->asset_alignment - (chunk->data_area & mask);
		chunk->data_area += padding;
	}
	TOY_ASSERT(chunk->data_area + pool->asset_size * pool->chunk_block_count < (uintptr_t)chunk + TOY_MEMORY_CHUNK_SIZE);
	for (uint32_t i = 0; i < pool->chunk_block_count - 1; ++i)
		chunk->ref_counts[i].store(i + 1);
	chunk->ref_counts[pool->chunk_block_count - 1].store(UINT32_MAX);

	return chunk;
}


static void toy_destroy_asset_pool_chunk (
	toy_asset_pool_p pool,
	toy_asset_pool_chunk_p chunk)
{
	if (NULL != pool->destroy_fp) {
		// Set all free block ref_count = 0
		uint32_t free_index = chunk->next_free_block_index;
		while (UINT32_MAX != free_index) {
			uint32_t next_free_index = chunk->ref_counts[free_index].load();
			chunk->ref_counts[free_index].store(0);
			free_index = next_free_index;
		}

		// Destroy all block which ref_count != 0
		for (uint32_t i = 0; i < pool->chunk_block_count; ++i) {
			if (0 != chunk->ref_counts[i].load()) {
				void* asset_item = (void*)(chunk->data_area + pool->asset_size * i);
				pool->destroy_fp(pool, asset_item);
				if (NULL != pool->literal_name)
					toy_log_w("Destroy asset pool (%s) without clean all items, item ref_count: %u", pool->literal_name, chunk->ref_counts[i].load());
				else
					toy_log_w("Destroy asset pool without clean all items (asset_size = %u, chunk_block_count = %u, item ref_count: %u)",
						pool->asset_size, pool->chunk_block_count, chunk->ref_counts[i].load());
			}
		}
	}

	toy_free(&pool->chunk_alc, chunk);
}


void toy_init_asset_pool (
	size_t asset_size,
	size_t asset_alignment,
	toy_destroy_asset_fp destroy_asset_fp,
	const toy_allocator_t* chunk_alc,
	const toy_allocator_t* alc,
	void* context,
	const char* literal_name,
	toy_asset_pool_t* output)
{
	output->chunks = NULL;
	output->chunk_count = 0;
	size_t data_area_size = TOY_MEMORY_CHUNK_SIZE - sizeof(toy_asset_pool_chunk_t);
	output->chunk_block_count = (data_area_size - asset_alignment) / (sizeof(*((output->chunks[0])->ref_counts)) + asset_size);
	TOY_ASSERT(output->chunk_block_count > 16);
	output->asset_size = asset_size;
	output->asset_alignment = asset_alignment;
	output->chunk_alc = *chunk_alc;
	output->alc = *alc;
	output->destroy_fp = destroy_asset_fp;
	output->context = context;
	output->literal_name = literal_name;
}


void toy_destroy_asset_pool (
	const toy_allocator_t* alc,
	toy_asset_pool_p pool)
{
	if (NULL != pool->chunks) {
		for (uint16_t i = 0; i < pool->chunk_count; ++i) {
			TOY_ASSERT(NULL != pool->chunks[i]);
			toy_destroy_asset_pool_chunk(pool, pool->chunks[i]);
		}
		toy_free_aligned(alc, pool->chunks);
	}
}


void* toy_get_asset_item (toy_asset_pool_p pool, uint32_t index)
{
	TOY_ASSERT(index < pool->chunk_block_count* pool->chunk_count);

	uint64_t pool_index = index / pool->chunk_block_count;
	TOY_ASSERT(pool_index < pool->chunk_count);
	uintptr_t data = pool->chunks[pool_index]->data_area;

	return (void*)(data + (index % pool->chunk_block_count) * pool->asset_size);
}


static uint32_t toy_alloc_asset_chunk_item (toy_asset_pool_chunk_p chunk)
{
	uint32_t index = chunk->next_free_block_index;
	chunk->next_free_block_index = chunk->ref_counts[index].load();
	chunk->ref_counts[index].store(0);
	++(chunk->allocated_block_count);
	return index;
}


static void toy_free_asset_chunk_item (toy_asset_pool_chunk_p chunk, uint32_t index) {
	TOY_ASSERT(0 == chunk->ref_counts[index].load());

	chunk->ref_counts[index].store(chunk->next_free_block_index);
	chunk->next_free_block_index = index;
	TOY_ASSERT(chunk->allocated_block_count > 0);
	--(chunk->allocated_block_count);
}


uint32_t toy_alloc_asset_item (
	toy_asset_pool_p pool,
	toy_error_t* error)
{
	for (uint32_t chunk_i = 0; chunk_i < pool->chunk_count; ++chunk_i) {
		toy_asset_pool_chunk_p chunk = pool->chunks[chunk_i];
		if (chunk->allocated_block_count < pool->chunk_block_count) {
			toy_ok(error);
			return pool->chunk_block_count * chunk_i + toy_alloc_asset_chunk_item(chunk);
		}
	}

	toy_asset_pool_chunk_p* chunk_array = (toy_asset_pool_chunk_p*)toy_alloc_aligned(&pool->alc, sizeof(toy_asset_pool_chunk_p) * (pool->chunk_count + 1), sizeof(void*));
	if (toy_unlikely(NULL == chunk_array)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Extend asset chunk array failed", error);
		return UINT32_MAX;
	}

	toy_asset_pool_chunk_p chunk = toy_alloc_asset_chunk(pool, &pool->chunk_alc);
	if (toy_unlikely(NULL == chunk)) {
		toy_free_aligned(&pool->alc, chunk_array);
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Malloc asset chunk failed", error);
		return UINT32_MAX;
	}

	for (size_t i = 0; i < pool->chunk_count; ++i)
		chunk_array[i] = pool->chunks[i];
	chunk_array[pool->chunk_count] = chunk;

	if (NULL != pool->chunks)
		toy_free_aligned(&pool->alc, pool->chunks);
	pool->chunks = chunk_array;

	toy_ok(error);
	return pool->chunk_block_count * ((pool->chunk_count)++) + toy_alloc_asset_chunk_item(chunk);
}


void toy_raw_free_asset_item (
	toy_asset_pool_p pool,
	uint32_t index)
{
	TOY_ASSERT(NULL != pool && UINT32_MAX != index);

	uint32_t pool_index = index / pool->chunk_block_count;
	uint32_t item_index = index % pool->chunk_block_count;
	TOY_ASSERT(pool_index < pool->chunk_count&& index < pool->chunk_block_count);

	toy_asset_pool_chunk_p chunk = pool->chunks[pool_index];
	toy_free_asset_chunk_item(chunk, item_index);
}


void toy_free_asset_item (
	toy_asset_pool_p pool,
	uint32_t index)
{
	TOY_ASSERT(NULL != pool && UINT32_MAX != index);

	uint32_t pool_index = index / pool->chunk_block_count;
	uint32_t item_index = index % pool->chunk_block_count;
	TOY_ASSERT(pool_index < pool->chunk_count&& index < pool->chunk_block_count);

	toy_asset_pool_chunk_p chunk = pool->chunks[pool_index];

	if (toy_likely(NULL != pool->destroy_fp)) {
		void* asset_item = (void*)(chunk->data_area + pool->asset_size * item_index);
		pool->destroy_fp(pool, asset_item);
	}

	toy_free_asset_chunk_item(chunk, item_index);
}


uint32_t toy_add_asset_ref (toy_asset_pool_p pool, uint32_t index, uint32_t ref_count)
{
	TOY_ASSERT(NULL != pool && UINT32_MAX != index);

	uint32_t pool_index = index / pool->chunk_block_count;
	uint32_t item_index = index % pool->chunk_block_count;

	TOY_ASSERT(pool_index < pool->chunk_count && index < pool->chunk_block_count);

	uint32_t count_before = pool->chunks[pool_index]->ref_counts[item_index].fetch_add(ref_count);
	return count_before;
}


uint32_t toy_sub_asset_ref (toy_asset_pool_p pool, uint32_t index, uint32_t ref_count)
{
	TOY_ASSERT(NULL != pool && UINT32_MAX != index);

	uint32_t pool_index = index / pool->chunk_block_count;
	uint32_t item_index = index % pool->chunk_block_count;

	TOY_ASSERT(pool_index < pool->chunk_count && index < pool->chunk_block_count);

	uint32_t count_before = pool->chunks[pool_index]->ref_counts[item_index].fetch_sub(ref_count);
	TOY_ASSERT(count_before >= ref_count);
	return count_before;
}


uint32_t toy_get_asset_ref (toy_asset_pool_p pool, uint32_t index)
{
	TOY_ASSERT(NULL != pool && UINT32_MAX != index);

	uint32_t pool_index = index / pool->chunk_block_count;
	uint32_t item_index = index % pool->chunk_block_count;

	TOY_ASSERT(pool_index < pool->chunk_count && index < pool->chunk_block_count);
	return pool->chunks[pool_index]->ref_counts[item_index].load();
}


void toy_create_asset_item_ref_pool (
	uint32_t initial_length,
	const toy_allocator_t* alc,
	toy_asset_item_ref_pool_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(0 < initial_length && NULL != alc);
	output->refs = (toy_asset_pool_item_ref_t*)toy_alloc_aligned(alc, sizeof(toy_asset_pool_item_ref_t) * initial_length, sizeof(void*));
	if (toy_unlikely(NULL == output->refs)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to alloc asset references", error);
		return;
	}

	output->pool_length = initial_length;
	output->next_block = 0;
	output->alc = *alc;

	for (uint32_t i = 0; i < initial_length; ++i)
		output->refs[i].next_ref = i + 1;
	output->refs[initial_length - 1].next_ref = UINT32_MAX;

	toy_ok(error);
	return;
}


void toy_destroy_asset_ref_pool (toy_asset_item_ref_pool_t* ref_pool)
{
	toy_free_aligned(&ref_pool->alc, ref_pool->refs);
}


uint32_t toy_alloc_asset_item_ref (toy_asset_item_ref_pool_t* ref_pool)
{
	if (ref_pool->pool_length <= ref_pool->next_block) {
		uint32_t new_length = ref_pool->pool_length * 2;
		toy_asset_pool_item_ref_t* refs = (toy_asset_pool_item_ref_t*)toy_alloc_aligned(
			&ref_pool->alc, sizeof(toy_asset_pool_item_ref_t) * new_length, sizeof(void*));
		if (NULL == refs)
			return UINT32_MAX;

		for (uint32_t i = 0; i < ref_pool->pool_length; ++i)
			refs[i] = ref_pool->refs[i];
		for (uint32_t i = ref_pool->pool_length; i < new_length - 1; ++i)
			refs[i].next_ref = i + 1;
		refs[new_length - 1].next_ref = UINT32_MAX;

		toy_free_aligned(&ref_pool->alc, ref_pool->refs);
		ref_pool->refs = refs;
		ref_pool->next_block = ref_pool->pool_length;
		ref_pool->pool_length = new_length;
	}

	uint32_t ret = ref_pool->next_block;
	ref_pool->next_block = ref_pool->refs[ref_pool->next_block].next_ref;
	return ret;
}


void toy_free_asset_item_ref (toy_asset_item_ref_pool_t* ref_pool, uint32_t index)
{
	TOY_ASSERT(NULL != ref_pool && index < ref_pool->pool_length);

	ref_pool->refs[index].next_ref = ref_pool->next_block;
}

TOY_EXTERN_C_END
