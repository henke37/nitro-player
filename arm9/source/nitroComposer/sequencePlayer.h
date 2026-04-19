#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include <nds/cothread.h>

#include "nitroComposer/ipc.h"
#include "nitroComposer/debugFlags.h"

#include "sseq.h"
#include "ssar.h"
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
	class SequencePlayer;

	class SequencePlayerGroup {
		public:
		SequencePlayerGroup();
		SequencePlayerGroup(const SequencePlayerGroup &) = delete;
		~SequencePlayerGroup();

		void KillAllSequences();

		void SetSdat(const SDatFile *sdat);

		void LoadSequence(unsigned int sequenceId);
		void LoadSequence(const std::string &sequenceName);
		void LoadSequenceArchive(unsigned int archiveId);
		void LoadSequenceArchive(const std::string &archiveName);
		const std::unique_ptr<SequenceInfoRecord> &GetLoadedSequenceInfo() const;

		void LoadBank(unsigned int bankId);
		void LoadWaveFormsForCurrentBank();
		void LoadWaveArchive(unsigned int archiveSlot, std::uint16_t archiveId);
		void LoadWaveArchiveData(unsigned int archiveSlot, const std::unique_ptr<WaveArchiveInfoRecord> &info);
		void UnloadWaveArchiveWaveForms(unsigned int archiveSlot);

	private:
		std::vector<SequencePlayer *> players;
		void RegisterPlayer(SequencePlayer *player);
		void UnregisterPlayer(SequencePlayer *player);

		const SDatFile *sdat;

		union {
			std::unique_ptr<SSEQ> sseq;
			std::unique_ptr<SSAR> ssar;
		};
		unsigned int loadedSequenceId = 0xFFFFFFFF;
		unsigned int loadedSequenceArchiveId = 0xFFFFFFFF;
		bool hasLoadedSequenceData() const;
		void UnloadSequenceData();

		void LoadSequence(const std::unique_ptr<SequenceInfoRecord> &);
		void LoadSequenceArchive(const std::unique_ptr<SequenceArchiveRecord> &);

		unsigned int loadedBankIndex = 0xFFFFFFFF;

		std::unique_ptr<SBNK> sbnk;
		std::unique_ptr<SWAR> swars[4];

		size_t sequenceDataSize;
		std::unique_ptr<std::uint8_t[]> sequenceData;

		void LoadWaveFormForInstrument(InstrumentBank::BaseInstrument *inst);
		void LoadWaveFormForInstrument(InstrumentBank::PCMInstrument *inst);
		void LoadWaveFormForInstrument(InstrumentBank::SplitInstrument *inst);
		void LoadWaveFormForInstrument(InstrumentBank::Drumkit *inst);
		void LoadWaveArchiveWaveForm(unsigned int archiveSlot, std::uint16_t waveId);

		void sendLoadBankIPC(const NitroComposer::InstrumentBank *loadedBank);
		void sendLoadWaveArchiveIPC(unsigned int archiveSlot, const NitroComposer::LoadedWaveArchive *loadedArchive);

		LoadedWaveArchive loadedWaveArchives[4];

		friend class SequencePlayer;
	};

	class SequencePlayer {
	public:
		SequencePlayer(SequencePlayerGroup *group);
		SequencePlayer(const SequencePlayer &) = delete;
		~SequencePlayer();

		void PlaySequence(unsigned int sequenceId);
		void PlaySequence(const std::string &sequenceName);
		void PlayArchiveSequence(unsigned int archiveId, unsigned int sequenceId);
		void PlayArchiveSequence(unsigned int archiveId, const std::string &sequenceName);
		void PlayArchiveSequence(const std::string &archiveName, unsigned int sequenceId);
		void PlayArchiveSequence(const std::string &archiveName, const std::string &sequenceName);
		void AbortSequence();
		void KillSequence();

		void SetVar(std::uint8_t var, std::int16_t val);
		std::int16_t GetVar(std::uint8_t var) const;
		void SetTrackMute(std::uint8_t trackId, bool mute);

		void SetAllowedChannelsOverride(std::uint16_t channels);

		bool IsPlaying() const { return isPlaying; }

	private:
		std::int32_t playerId = -1;

		SequencePlayerGroup *group;

		void PlaySequence(const std::unique_ptr<SequenceInfoRecord> &sequenceInfo);

		void sendLoadBankIPC(const NitroComposer::InstrumentBank *loadedBank);
		void sendLoadWaveArchiveIPC(unsigned int archiveSlot, const NitroComposer::LoadedWaveArchive *loadedArchive);

		bool isPlaying = false;

		void sequenceEnded();
		friend class MusicEngine;
		friend class SequencePlayerGroup;
	};

	class MusicEngine {
	public:
		MusicEngine();
		MusicEngine(const MusicEngine &) = delete;
		~MusicEngine();
		void SetMainVolume(std::uint8_t volume);
		void SetDebugFlags(DebugFlags flags);
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