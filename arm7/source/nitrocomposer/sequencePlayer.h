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
		signed int FindFreeVoice(InstrumentBank::InstrumentType type);

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

			void Release();
			void Kill();

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

			std::uint8_t GetAttack() const;
			std::uint8_t GetDecay() const;
			std::uint8_t GetSustain() const;
			std::uint8_t GetRelease() const;
		};
		static constexpr unsigned int voiceCount = 16;
		Voice voices[voiceCount];

		class Track {
		public:
			Track();

			void Init(SequencePlayer *player);
			void Reset();

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

			std::uint8_t priority;

			std::uint8_t attack = 0xFF;
			std::uint8_t decay = 0xFF;
			std::uint8_t sustain = 0xFF;
			std::uint8_t release = 0xFF;

			std::uint8_t volume;
			std::uint8_t expression;
			std::int8_t pan;

			std::int8_t pitchBend;
			std::uint8_t pitchBendRange;

			enum class ModulationMode {
				Vibrato = 0,//pitch
				Tremolo = 1,//volume
				Pan = 2
			};

			std::uint8_t modDepth;
			std::uint8_t modSpeed;
			ModulationMode modMode;
			std::uint8_t modRange;
			std::uint16_t modDelay;

			std::int8_t transpose;

			bool portamento;
			std::uint8_t lastPlayedNote;
			std::uint8_t portaTime;

			bool lastComparisonResult;

			const std::uint8_t *nextCommand;

			enum class StackEntryType {
				Call,
				Loop
			};

			struct StackEntry {
				StackEntryType type;
				const std::uint8_t *nextCommand;
				unsigned int loopCounter;
			};

			StackEntry stack[4];
			unsigned int stackPointer;

			int waitCounter;

			void ExecuteNextCommand();
			void ExecuteNextRandomCommand();
			void ExecuteNextVarCommand();

			void skipCommandArgs(std::uint8_t command);
			void skipCommandRandomArgs(std::uint8_t command);
			void skipCommandVarArgs(std::uint8_t command);

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

		std::uint8_t externalChannelReservations;
		std::uint8_t allowedChannels;

		bool isVoiceAllowed(std::uint8_t voiceIndex) const;
	};

	int Cnv_Attack(int attk);
	int Cnv_Fall(int fall);
	int Cnv_Scale(int scale);
	int Cnv_Sust(int sust);
	int Cnv_Sine(int arg);
	uint16_t Timer_Adjust(uint16_t basetmr, int pitch);

	int calcVolDivShift(int x);
	int32_t muldiv7(int32_t val, uint8_t mul);

	extern SequencePlayer sequencePlayer;

}

#endif