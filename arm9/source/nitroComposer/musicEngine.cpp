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

		fifoSetDatamsgHandler(FIFO_NITRO_COMPOSER, fifoHandler, this);

		ipcPowerOn();

		powerOn(PM_SOUND_AMP);
	}

	MusicEngine::~MusicEngine() {}

	void MusicEngine::fifoHandler(int num_bytes, void *userdata) {
		MusicEngine *engine = static_cast<MusicEngine *>(userdata);
		cosema_signal(&engine->asyncEvtSemaphore);
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
				int written=fifoGetDatamsg(FIFO_NITRO_COMPOSER, fifoBuffSize, fifoBuffer);
				sassert(written >= (int)sizeof(AsyncEventIPC), "Too short async msg %i",written);

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
			auto statusEvt = static_cast<const SequenceStatusEventIPC *>(event);
			auto player = findPlayerById(statusEvt->playerId);
			player->sequenceEnded();
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

	std::int32_t MusicEngine::registerPlayer(SequencePlayer *player) {
		registeredPlayers.push_back({ player, nextPlayerId });
		return nextPlayerId++;
	}
	
	void MusicEngine::unregisterPlayer(SequencePlayer *player) {
		for(auto itr = registeredPlayers.begin(); itr != registeredPlayers.end(); ++itr) {
			if(itr->player == player) {
				registeredPlayers.erase(itr);
				return;
			}
		}
		sassert(false, "Tried to unregister unregistered player");
	}

	SequencePlayer *MusicEngine::findPlayerById(std::int32_t id) {
		assert(id > 0);
		for(auto &regPlayer : registeredPlayers) {
			if(regPlayer.id == id) {
				return regPlayer.player;
			}
		}
		return nullptr;
	}

	void MusicEngine::SetMainVolume(std::uint8_t volume) {

		std::unique_ptr<SetMainVolumeIPC> buff = std::make_unique<SetMainVolumeIPC>();
		buff->command = BaseIPC::CommandType::SetMainVolume;
		buff->volume = volume;

		bool success = fifoSendDatamsg(FIFO_NITRO_COMPOSER, sizeof(SetMainVolumeIPC), (u8 *)buff.get());
		assert(success);
	}

	FifoMutexLock::FifoMutexLock() {
		mutex.aquire(FIFO_NITRO_COMPOSER);
	}
	FifoMutexLock::~FifoMutexLock() {
		mutex.release();
	}
}