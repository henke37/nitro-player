#include "sequencePlayer.h"

#include <nds/fifocommon.h>
#include <nds/timers.h>
#include <nds/interrupts.h>

#include <cassert>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

	SequencePlayer sequencePlayer;

	const size_t fifoBuffSize = 32;

	void SequencePlayer::Init() {
		setupFifo();
		setupTimer();

		for(unsigned int var = 0; var < numVariables; ++var) {
			variables[var] = -1;
		}
	}

	void SequencePlayer::SetVar(std::uint8_t var, std::int16_t val) {
		variables[var] = val;
	}

	std::int16_t SequencePlayer::GetVar(std::uint8_t var) const {
		return variables[var];
	}

	void SequencePlayer::setupFifo() {
		fifoSetDatamsgHandler(FIFO_NITRO_COMPOSER, fifoDatagramHandler, nullptr);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes, void *userdata) {
		sequencePlayer.fifoDatagramHandler(num_bytes);
	}

	void SequencePlayer::fifoDatagramHandler(int num_bytes) {
		u8 fifoBuffer[fifoBuffSize];
		fifoGetDatamsg(FIFO_USER_01, fifoBuffSize, fifoBuffer);

		auto ipc = reinterpret_cast<BaseIPC *>(fifoBuffer);

		switch(ipc->command) {
		case BaseIPC::CommandType::SetVar:
		{
			SetVarIPC *setVarIpc = static_cast<SetVarIPC *>(ipc);
			SetVar(setVarIpc->var, setVarIpc->val);
		} break;

		case BaseIPC::CommandType::GetVar:
		{
			GetVarIPC *getVarIpc = static_cast<GetVarIPC *>(ipc);
			std::int16_t val = GetVar(getVarIpc->var);
			bool success = fifoSendValue32(FIFO_NITRO_COMPOSER, val);
			assert(success);
		} break;

		case BaseIPC::CommandType::SetTempo:
		{
			SetTempoIPC *setTempoIpc = static_cast<SetTempoIPC *>(ipc);
			this->tempo = setTempoIpc->tempo;
		} break;
		case BaseIPC::CommandType::SetMainVolume:
		{
			SetMainVolumeIPC *setMainVolumeIpc = static_cast<SetMainVolumeIPC *>(ipc);
			this->mainVolume = setMainVolumeIpc->volume;
		} break;

		case BaseIPC::CommandType::LoadBank:
		{
			LoadBankIPC *loadBankIpc = static_cast<LoadBankIPC *>(ipc);
		} break;

		default:
			assert(0);
		}
	}

	void SequencePlayer::setupTimer() {
		timerStart(1, ClockDivider_64, 2728, ISR);
	}

	void SequencePlayer::ISR() {
		sequencePlayer.Tick();
	}

	void SequencePlayer::Tick() {

		for(unsigned int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
			auto &track = tracks[trackIndex];
			track.Tick();
		}

		for(unsigned int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex) {
			auto &voice = voices[voiceIndex];
			voice.Tick();
		}
	}

}