#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include <cstdint>

namespace NitroComposer {

	class SequencePlayer {
	public:
		void Init();

		void SetVar(std::uint8_t var, std::int16_t val);
		std::int16_t GetVar(std::uint8_t var) const;

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
		static constexpr unsigned int voiceCount = 16;
		Voice voices[voiceCount];

		class Track {
		public:
			Track();

			void Tick();

			void SetInstrument(unsigned int instrumentId);

			void NoteOn(std::uint8_t note, std::uint16_t velocity, std::uint16_t durration);

		private:
			bool isPlaying;

			bool noteWait;
			bool tieMode;

			std::uint8_t attack;
			std::uint8_t decay;
			std::uint8_t sustain;
			std::uint8_t release;

			std::uint8_t *nextCommand;
		};
		static constexpr unsigned int trackCount = 16;
		Track tracks[trackCount];

		static constexpr unsigned int localVariableCount = 16;
		static constexpr unsigned int globalVariableCount = 16;
		static constexpr unsigned int numVariables = localVariableCount + globalVariableCount;
		std::int16_t variables[numVariables];

		void setupFifo();
		static void fifoDatagramHandler(int num_bytes, void *userdata);
		void fifoDatagramHandler(int num_bytes);

		void setupTimer();
		static void ISR();

		std::uint8_t tempo;
		std::uint8_t mainVolume;

		std::uint8_t *commandBuffer;
	};

	extern SequencePlayer sequencePlayer;

}

#endif