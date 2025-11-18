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
			ReserveChannels,
			InitStream,
			StreamPushBlock,
			StreamRetireBlock
		};

		CommandType command;
	};

	struct SequencePlayerIPC : BaseIPC {

	};

	struct PlayTrackIPC : SequencePlayerIPC {
		std::uint8_t *sequenceData;
		std::uint32_t length;
		std::uint16_t channelMask;
		std::uint8_t sequenceVolume;
	};

	struct SetMainVolumeIPC : BaseIPC {
		std::uint8_t volume;
	};

	struct SetTempoIPC : SequencePlayerIPC {
		std::uint8_t tempo;
	};

	struct SetVarIPC : SequencePlayerIPC {
		std::uint8_t var;
		std::int16_t val;
	};

	struct GetVarIPC : SequencePlayerIPC {
		std::uint8_t var;
	};

	struct LoadBankIPC : SequencePlayerIPC {
		const InstrumentBank *bank;
	};

	struct LoadWaveArchiveIPC : SequencePlayerIPC {
		std::uint8_t slot;
		const LoadedWaveArchive *archive;
	};

	struct ReserveChannelsIPC : BaseIPC {
		std::uint16_t reservations;
	};

	struct StreamPlayerIPC : BaseIPC {
	};

	struct InitStreamIPC : StreamPlayerIPC {
		WaveEncoding encoding;
		bool stereo;
		std::uint16_t timer;
	};

	struct StreamPushBlockIPC : StreamPlayerIPC {
		std::uint32_t blockId;
		std::uint32_t blockDataSize;
		std::uint32_t blockSampleCount;
		void *blockData[2];
	};

	struct StreamRetireBlockIPC : StreamPlayerIPC {
		std::uint32_t blockId;
	};
}

#endif