#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include "nitroComposer/bank.h"
#include "nitroComposer/wave.h"

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
		void AbortSequence();

		void StartTrack(std::uint8_t trackId, std::uint32_t offset);
		unsigned int FindFreeVoice(InstrumentBank::InstrumentType type);

		const LoadedWave &GetWave(unsigned int archiveSlot, unsigned int waveIndex);

	private:
		void Tick();

		void TickTracks();

		enum class VoiceState {
			Free,
			Attacking,
			Decaying,
			Sustaining,
			Releasing
		};

		class Track;

		class Voice {
		public:
			void Init(unsigned int voiceIndex);

			void StartNote(const Track *track, const InstrumentBank::LeafInstrument *instrument, std::uint8_t note, std::uint8_t velocity, unsigned int length);

			void Tick();

			bool IsPulseVoice() const { return voiceIndex >= 8 && voiceIndex <= 13; }
			bool IsNoiseVoice() const { return voiceIndex >= 14; }

			VoiceState state = VoiceState::Free;
		private:
			unsigned int voiceIndex;
			void ConfigureControlRegisters();
			void ConfigureVolumeRegister();
			void ConfigureTimerRegister();

			std::uint8_t ComputeVolume() const;
			std::uint8_t ComputePan() const;

			std::uint8_t note;
			std::uint8_t velocity;
			unsigned int length;

			const InstrumentBank::LeafInstrument *currentInstrument;
			SequencePlayer *player;
			const Track *track;
		};
		static constexpr unsigned int voiceCount = 16;
		Voice voices[voiceCount];

		class Track {
		public:
			Track();

			void Init(SequencePlayer *player);

			void Tick();

			void StartPlaying(std::uint32_t offset);
			void StopPlaying();

			void SetInstrument(unsigned int instrumentId);

			void NoteOn(std::uint8_t note, std::uint16_t velocity, std::uint16_t durration);

		private:
			SequencePlayer *player;
			const InstrumentBank::BaseInstrument *currentInstrument;

			bool isPlaying;

			bool noteWait;
			bool tieMode;

			std::uint8_t attack;
			std::uint8_t decay;
			std::uint8_t sustain;
			std::uint8_t release;

			const std::uint8_t *nextCommand;

			int wait;

			void ExecuteNextCommand();
			void skipCommandArgs(std::uint8_t command);

			void NoteOn(std::uint8_t note, std::uint8_t velocity, unsigned int length);
			void NoteOnReal(std::uint8_t note, std::uint8_t velocity, unsigned int length);
			void NoteOnTie(std::uint8_t note, std::uint8_t velocity);

			const InstrumentBank::LeafInstrument *ResolveInstrumentForNote(std::uint8_t note) const;

			void SetNextCommand(std::uint32_t offset);

			std::uint8_t readByteCommand();
			std::uint16_t readShortCommand();
			std::uint32_t readTriByteCommand();
			unsigned int readMidiVarLen();

			friend class Voice;
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
		std::uint8_t tempoTimer;

		std::uint8_t mainVolume;

		const InstrumentBank *bank;
		static constexpr unsigned int numWaveArchs = 4;
		const LoadedWaveArchive *waveArchs[numWaveArchs];

		const std::uint8_t *sequenceData;
	};

	extern SequencePlayer sequencePlayer;

}

#endif