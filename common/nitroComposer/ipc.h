#ifndef COMMON_NITROCOMPOSER_IPC_H
#define COMMON_NITROCOMPOSER_IPC_H

#include <cstdint>

#include "nitroComposer/infoRecords.h"
#include "nitroComposer/bank.h"
#include "nitroComposer/wave.h"

namespace NitroComposer {

#define FIFO_NITRO_COMPOSER FIFO_USER_01

	struct BaseIPC {
		enum class CommandType {
			Invalid,
			PowerOn,
			LoadBank,
			LoadWaveArchive,
			PlaySequence,
			StopSequence,
			SetTempo,
			SetMainVolume,
			GetVar,
			SetVar,
			ReserveChannels
		};

		CommandType command;
	};

	struct PlayTrackIPC : BaseIPC {
		std::uint8_t *sequenceData;
		std::uint32_t length;
		std::uint32_t channelMask;
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
		const InstrumentBank *bank;
	};

	struct LoadWaveArchiveIPC : BaseIPC {
		std::uint8_t slot;
		const LoadedWaveArchive *archive;
	};

	struct ReserveChannelsIPC : BaseIPC {
		std::uint16_t reservations;
	};
}

#endif