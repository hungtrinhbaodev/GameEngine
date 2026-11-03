#pragma once
#include <consts.h>
#include <cstdint>

struct Handle {

	static inline Handle make_invalid_handle() { return Handle(Const::MAX_UINT32, 0); }

	uint64_t handle = ::Const::MAX_UINT32;

	inline Handle() { handle = ::Const::MAX_UINT32; }

	inline Handle(uint32_t id, uint32_t generation) { make(generation, id); }

	inline void make(uint32_t generation, uint32_t id) { handle = (uint64_t)generation << 32 | (uint64_t)id; }

	/**
	 * Id of the handle in moment.
	 */
	inline uint32_t id() const { return (uint32_t)handle; }

	/**
	 * Index of id when created.
	 */
	inline uint32_t generation() const { return (uint32_t)(handle >> 32); }

	inline bool is_invalid() const { return id() == ::Const::MAX_UINT32; }
};