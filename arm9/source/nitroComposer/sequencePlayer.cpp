#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

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


	void SequencePlayer::SetVar(std::uint8_t var, std::int16_t val) {
		std::unique_ptr<SetVarIPC> buff = std::make_unique<SetVarIPC>();
		buff->command = BaseIPC::CommandType::SetVar;
		buff->var = var;
		buff->val = val;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)buff.get());
	}
	std::int16_t SequencePlayer::GetVar(std::uint8_t var) const {
		std::unique_ptr<GetVarIPC> buff = std::make_unique<GetVarIPC>();
		buff->command = BaseIPC::CommandType::GetVar;
		buff->var = var;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)buff.get());

		fifoWaitValue32(FIFO_NITRO_COMPOSER);

		return static_cast<std::int16_t> (fifoGetValue32(FIFO_NITRO_COMPOSER));

	}

	void SequencePlayer::SetMainVolume(std::uint8_t volume) {

		std::unique_ptr<SetMainVolumeIPC> buff = std::make_unique<SetMainVolumeIPC>();
		buff->command = BaseIPC::CommandType::SetMainVolume;
		buff->volume = volume;

		fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetVarIPC), (u8 *)buff.get());
	}

}