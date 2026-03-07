#pragma once
#include <vector>

// I just realized we don't need this for now

namespace globals {
	inline std::vector<uintptr_t> bases;
	inline std::vector<DWORD> pids;
	inline inst crvxPtr(0, 0);
	inline inst lastDm(0, 0);
}