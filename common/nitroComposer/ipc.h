#ifndef COMMON_NITROCOMPOSER_IPC_H
#define COMMON_NITROCOMPOSER_IPC_H

#include <cstdint>

#include "nitroComposer/infoRecords.h"
#include "nitroComposer/bank.h"
#include "nitroComposer/wave.h"

namespace NitroComposer {

#define FIFO_NITRO_COMPOSER FIFO_USER_01

	const size_t fifoBuffSize = 32;

	struct BaseIPC {
		enum class CommandType : std::uint8_t {
			Invalid = 0,
			PowerOn = 1,
			SetMainVolume = 2,
			ReserveChannels = 50,
			LoadBank = 101,
			LoadWaveArchive = 102,
			PlaySequence = 120,
			StopSequence = 121,
			SetTempo = 122,
			GetVar = 110,
			SetVar = 111,
			InitStream = 200,
			StopStream = 201,
			StreamPushBlock = 202
		};

		CommandType command;
	};

	struct AsyncEventIPC {
		enum class EventType : std::uint8_t {
			SequenceEnded = 102,
			StreamEnded = 201,
			StreamOutOfData = 202,
			StreamRetireBlock = 203
		};

		EventType eventId;
	};

	struct SequenceStatusEventIPC : AsyncEventIPC {
		std::int32_t playerId;
	};

	struct ReserveChannelsIPC : BaseIPC {
		std::uint16_t reservations;
	};

	struct SetMainVolumeIPC : BaseIPC {
		std::uint8_t volume;
	};

	struct SequencePlayerIPC : BaseIPC {
		std::int32_t playerId;
	};

	struct PlayTrackIPC : SequencePlayerIPC {
		std::uint8_t *sequenceData;
		std::uint32_t length;
		std::ptrdiff_t startPos;
		std::uint16_t channelMask;
		std::uint8_t sequenceVolume;
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
		std::uint32_t startPos;
		void *blockData[2];
	};

	struct StreamRetireBlockIPC : AsyncEventIPC {
		std::uint32_t blockId;
	};
}

#endif