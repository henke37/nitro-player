#ifndef COMMON_NITROCOMPOSER_IPC_H
#define COMMON_NITROCOMPOSER_IPC_H

#include <cstdint>

struct BaseIPC {
	enum class CommandType {
		Invalid,
		LoadBank,
		PlaySequence,
		StopSequence,
		SetTempo,
		SetMainVolume,
		GetVar,
		SetVar
	};
	
	CommandType command;
};

struct PlayTrack : BaseIPC {
	std::uint8_t *commands;
	std::uint32_t length;
};

struct SetMainVolume : BaseIPC {
	std::uint8_t volume;
};

struct SetTempo : BaseIPC {
	std::uint8_t tempo;
};

struct SetVar : BaseIPC {
	std::uint8_t var;
	std::int16_t val;
};

struct GetVar : BaseIPC {
	std::uint8_t var;
};

#endif