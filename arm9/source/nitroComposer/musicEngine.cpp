#include "sequencePlayer.h"

#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/system.h>

#include "nitroComposer/ipc.h"

namespace NitroComposer {

	MusicEngine musicEngine;

	MusicEngine::MusicEngine() {
		cosema_init(&asyncEvtSemaphore, 0);

		cothread_create(msgPumpThread, this, 0x1000, COTHREAD_DETACHED);

		ipcPowerOn();

		powerOn(PM_SOUND_AMP);
	}

	MusicEngine::~MusicEngine() {}

	void MusicEngine::fifoISR() {
		if(!fifoCheckDatamsg(FIFO_NITRO_COMPOSER)) return;
		cosema_signal(&musicEngine.asyncEvtSemaphore);
	}

	int MusicEngine::msgPumpThread(void *arg) {
		MusicEngine *player = static_cast<MusicEngine *>(arg);
		player->msgPump();
		return 0;
	}

	void MusicEngine::msgPump() {
		while(true) {
			cosema_wait(&asyncEvtSemaphore);
			{
				u8 fifoBuffer[fifoBuffSize];
				fifoGetDatamsg(FIFO_NITRO_COMPOSER, fifoBuffSize, fifoBuffer);

				auto ipc = reinterpret_cast<AsyncEventIPC *>(fifoBuffer);

				dispatchAsyncEvent(ipc);
				puts("MusicEngine msgPump processing FIFO message");
			}
		}
	}

	void MusicEngine::dispatchAsyncEvent(const AsyncEventIPC *event) {
		switch(event->eventId) {
		case AsyncEventIPC::EventType::SequenceEnded:
		{
			// TODO: multiple players
			//this->sequenceEnded();
		} break;
		default:
			sassert(false, "Unknown async event %u", static_cast<std::uint8_t>(event->eventId));
			break;
		}
	}

	void MusicEngine::ipcPowerOn() {
		std::unique_ptr<BaseIPC> buff = std::make_unique<BaseIPC>();
		buff->command = BaseIPC::CommandType::PowerOn;
		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(BaseIPC), (u8 *)buff.get());
		assert(success);
	}
	

	void MusicEngine::SetMainVolume(std::uint8_t volume) {

		std::unique_ptr<SetMainVolumeIPC> buff = std::make_unique<SetMainVolumeIPC>();
		buff->command = BaseIPC::CommandType::SetMainVolume;
		buff->volume = volume;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetMainVolumeIPC), (u8 *)buff.get());
		assert(success);
	}
}