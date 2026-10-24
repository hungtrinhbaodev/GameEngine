#pragma once
#include <cstdint>

struct Handle {

	uint64_t handle = 0;

	inline Handle() { handle = 0; }

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
};