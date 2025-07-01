#ifndef COMMON_NITROCOMPOSER_IPC_H
#define COMMON_NITROCOMPOSER_IPC_H

#include <cstdint>

namespace NitroComposer {

#define FIFO_NITRO_COMPOSER FIFO_USER_01

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

	struct PlayTrackIPC : BaseIPC {
		std::uint8_t *commands;
		std::uint32_t length;
	};

	struct SetMainVolumeIPC : BaseIPC {
		std::uint8_t volume;
	};

	struct SetTempoIPC : BaseIPC {
		std::uint8_t tempo;
	};

	struct SetVarIPC : BaseIPC {
		std::uint8_t var;
		std::int16_t val;
	};

	struct GetVarIPC : BaseIPC {
		std::uint8_t var;
	};

	struct LoadBankIPC : BaseIPC {

	};

}

#endif