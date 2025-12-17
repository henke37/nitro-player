#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include <nds/cothread.h>

#include "nitroComposer/ipc.h"

#include "sseq.h"
#include "sbnk.h"
#include "swar.h"
#include "sdatFile.h"

#include "../fifoMutex.h"

namespace NitroComposer {

	class FifoMutexLock {
	public:
		FifoMutexLock();
		~FifoMutexLock();

	private:
		FifoMutex mutex;
	};

	class StreamPlayer;

	class SequencePlayer {
	public:
		SequencePlayer();
		~SequencePlayer();

		void SetSdat(const SDatFile *sdat);

		void PlaySequence(unsigned int sequenceId);
		void PlaySequence(const std::string &sequenceName);
		void PlaySequence(const std::unique_ptr<SequenceInfoRecord> &);
		void AbortSequence();
		void KillSequence();

		void LoadBank(unsigned int bankId);
		void LoadWaveFormsForCurrentBank();
		void LoadWaveFormForInstrument(InstrumentBank::BaseInstrument *inst);
		void LoadWaveFormForInstrument(InstrumentBank::PCMInstrument *inst);
		void LoadWaveFormForInstrument(InstrumentBank::SplitInstrument *inst);
		void LoadWaveFormForInstrument(InstrumentBank::Drumkit *inst);
		void LoadWaveArchive(unsigned int archiveSlot, std::uint16_t archiveId);
		void LoadWaveArchiveData(unsigned int archiveSlot, const std::unique_ptr<WaveArchiveInfoRecord> &info);
		void LoadWaveArchiveWaveForm(unsigned int archiveSlot, std::uint16_t waveId);
		void UnloadWaveArchiveWaveForms(unsigned int archiveSlot);

		void SetVar(std::uint8_t var, std::int16_t val);
		std::int16_t GetVar(std::uint8_t var) const;
		void SetTrackMute(std::uint8_t trackId, bool mute);

		bool IsPlaying() const { return isPlaying; }

	private:
		std::int32_t playerId = -1;
		const SDatFile *sdat;

		std::unique_ptr<SSEQ> sseq;
		unsigned int loadedBankIndex = 0xFFFFFFFF;
		std::unique_ptr<SBNK> sbnk;
		std::unique_ptr<SWAR> swars[4];

		LoadedWaveArchive loadedWaveArchives[4];

		std::uint16_t channelMask;

		std::unique_ptr<std::uint8_t[]> sequenceData;

		bool isPlaying = false;

		void sequenceEnded();
		friend class MusicEngine;
	};

	class MusicEngine {
	public:
		MusicEngine();
		~MusicEngine();
		void SetMainVolume(std::uint8_t volume);
	private:
		cosema_t asyncEvtSemaphore;

		static void fifoHandler(void *, void *userdata);
		static int msgPumpThread(void *arg);
		void msgPump();
		void dispatchAsyncEvent(const AsyncEventIPC *event);

		void ipcPowerOn();

		struct RegisteredPlayer {
			SequencePlayer *player;
			std::int32_t id;
		};
		std::vector<RegisteredPlayer> registeredPlayers;
		std::int32_t registerPlayer(SequencePlayer *player);
		void unregisterPlayer(SequencePlayer *player);
		SequencePlayer *findPlayerById(std::int32_t id);
		std::int32_t nextPlayerId = 1;

		StreamPlayer *currentStreamPlayer = nullptr;

		friend class StreamPlayer;
		friend class SequencePlayer;
		friend class FifoMutexLock;
	};

	extern MusicEngine musicEngine;
}
#endif