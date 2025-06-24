#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include "sseq.h"
#include "sbnk.h"
#include "swar.h"
#include "sdatFile.h"

class SequencePlayer {
public:
	SequencePlayer();
	~SequencePlayer();

	void SetSdat(const SDatFile *sdat);

	void LoadSequence(unsigned int sequenceId);
	void LoadSequence(const std::string &sequenceName);
	void LoadSequence(const std::unique_ptr<SequenceInfoRecord> &);

	void LoadBank(unsigned int bankId);

private:
	const SDatFile *sdat;

	std::unique_ptr<SSEQ> sseq;
	unsigned int loadedBankIndex = 0xFFFFFFFF;
	std::unique_ptr<SBNK> sbnk;
	std::unique_ptr<SWAR> swars[4];

	std::uint16_t channelMask;

};

#endif