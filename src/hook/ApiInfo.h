#pragma once

#include <cstdint>

struct AudioApiInfo
{
	uint64_t offset_step_up = 0;
	uint64_t offset_step_down = 0;
	bool got_success = false;
};

extern "C" 
bool ComputeAudioApiInfo(AudioApiInfo& info);


