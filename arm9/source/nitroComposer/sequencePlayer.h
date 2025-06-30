#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

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

		void LoadSequence(unsigned int sequenceId);
		void LoadSequence(const std::string &sequenceName);
		void LoadSequence(const std::unique_ptr<SequenceInfoRecord> &);

		void LoadBank(unsigned int bankId);

		void SetVar(std::uint8_t var, std::int16_t val);
		std::int16_t GetVar(std::uint8_t var) const;

		void SetMainVolume(std::uint8_t volume);

	private:
		const SDatFile *sdat;

		std::unique_ptr<SSEQ> sseq;
		unsigned int loadedBankIndex = 0xFFFFFFFF;
		std::unique_ptr<SBNK> sbnk;
		std::unique_ptr<SWAR> swars[4];

		std::uint16_t channelMask;

	};

}
#endif