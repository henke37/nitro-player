#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

class SequencePlayer {
public:
	void Init();

private:
	void Tick();

	enum class VoiceState {
		Free,
		Attacking,
		Decaying,
		Sustaining,
		Releasing
	};

	class Voice {
	public:
		Voice();

		void Tick();

		VoiceState state = VoiceState::Free;
	};

	Voice voices[16];

	class Track {
	public:
		Track();

		void Tick();

	private:
		bool isPlaying;
	};

	Track tracks[16];

	void setupFifo();
	static void fifoDatagramHandler(int num_bytes, void *userdata);
	void fifoDatagramHandler(int num_bytes);

	void setupTimer();
	static void ISR();
};

extern SequencePlayer sequencePlayer;

#endif