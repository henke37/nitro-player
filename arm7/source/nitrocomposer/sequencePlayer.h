#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include "nitroComposer/bank.h"

#include <cstdint>
#include <memory>

namespace NitroComposer {

	class LoadedWaveArchive;

	class SequencePlayer {
	public:
		void Init();

		void SetVar(std::uint8_t var, std::int16_t val);
		std::int16_t GetVar(std::uint8_t var) const;

		void PlaySequence(const std::uint8_t *sequenceData);

		unsigned int FindFreeVoice();

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
			void Init(unsigned int voiceIndex);

			void Tick();

			bool IsPulseVoice() const { return voiceIndex >= 8 && voiceIndex <= 13; }
			bool IsNoiseVoice() const { return voiceIndex >= 14; }

			VoiceState state = VoiceState::Free;
		private:
			unsigned int voiceIndex;
			void ConfigureControlRegisters();
			void ConfigureVolumeRegister();
			void ConfigurePanRegister();
			void ConfigureTimerRegister();

			std::unique_ptr<InstrumentBank::BaseInstrument> currentInstrument;
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

		const InstrumentBank *bank;
		const LoadedWaveArchive *waveArchs[4];

		const std::uint8_t *sequenceData;
	};

	extern SequencePlayer sequencePlayer;

}

#endif