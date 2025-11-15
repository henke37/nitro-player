#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include <nds/cothread.h>

#include "nitroComposer/ipc.h"

#include "sseq.h"
#include "sbnk.h"
#include "swar.h"
#include "sdatFile.h"

namespace NitroComposer {

	class SequencePlayer {
	public:
		SequencePlayer();
		~SequencePlayer();

		void SetSdat(const SDatFile *sdat);

		void PlaySequence(unsigned int sequenceId);
		void PlaySequence(const std::string &sequenceName);
		void PlaySequence(const std::unique_ptr<SequenceInfoRecord> &);
		void AbortSequence();

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

		void SetMainVolume(std::uint8_t volume);

	private:
		const SDatFile *sdat;

		std::unique_ptr<SSEQ> sseq;
		unsigned int loadedBankIndex = 0xFFFFFFFF;
		std::unique_ptr<SBNK> sbnk;
		std::unique_ptr<SWAR> swars[4];

		LoadedWaveArchive loadedWaveArchives[4];

		std::uint16_t channelMask;

		std::unique_ptr<std::uint8_t[]> sequenceData;

		cosema_t asyncEvtSemaphore;

		static void fifoISR();
		static int msgPumpThread(void *arg);
		void msgPump();

		void ipcPowerOn();

		friend class FifoMutexLock;
	};

	extern SequencePlayer sequencePlayer;
}
#endif