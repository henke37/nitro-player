#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>

#include "nitroComposer/ipc.h"

SequencePlayer::SequencePlayer() : sdat(nullptr) {}

SequencePlayer::~SequencePlayer() {}

void SequencePlayer::SetSdat(const SDatFile *sdat) {
	if(this->sdat == sdat) return;

	sseq.reset();
	sbnk.reset();
	for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
		swars[swarIndex].reset();
	}

	this->sdat = sdat;
}

void SequencePlayer::LoadSequence(unsigned int sequenceId) {
	const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceId);
	sassert(sequenceInfo, "Unknown sequence %u", sequenceId);
	LoadSequence(sequenceInfo);
}

void SequencePlayer::LoadSequence(const std::string &sequenceName) {
	const std::unique_ptr<SequenceInfoRecord> &sequenceInfo = sdat->GetSequenceInfo(sequenceName);
	sassert(sequenceInfo, "Unknown sequence \"%s\"", sequenceName.c_str());
	LoadSequence(sequenceInfo);
}

void SequencePlayer::LoadSequence(const std::unique_ptr<SequenceInfoRecord> &sequenceInfo) {
	LoadBank(sequenceInfo->bankId);
	sseq = sdat->OpenSequence(sequenceInfo);
}

void SequencePlayer::LoadBank(unsigned int bankId) {
	if(bankId == loadedBankIndex) return;
	loadedBankIndex = bankId;

	auto &bankInfo = sdat->GetBankInfo(bankId);
	sassert(bankInfo, "No bank info");
	for(unsigned int swarIndex = 0; swarIndex < 4; ++swarIndex) {
		if(bankInfo->swars[swarIndex] == 0xFFFF) {
			swars[swarIndex].reset();
		} else {
			swars[swarIndex] = sdat->OpenWaveArchive(bankInfo->swars[swarIndex]);
		}
	}
	sbnk = sdat->OpenBank(bankInfo);
	//printf("Loaded bank %s.\n", sdat->GetNameForBank(bankId).c_str());
}
