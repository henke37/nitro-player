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


	private:
		void Update();

		void UpdateVoices();

		enum class VoiceState {
			Free,
			Attacking,
			Decaying,
			Sustaining,
			Releasing
		};

		class PlayingSequence;

		class Track;

		class Voice {
		public:
			void Init(unsigned int voiceIndex);

			void StartNote(const Track *track, const InstrumentBank::LeafInstrument *instrument, std::uint8_t note, std::uint8_t velocity, unsigned int length);

			void Tick();
			void Update();

			bool IsPulseVoice() const { return voiceIndex >= 8 && voiceIndex <= 13; }
			bool IsNoiseVoice() const { return voiceIndex >= 14; }

			const Track *GetTrack() const { return track; }

			void Release();
			void Kill();

			VoiceState state = VoiceState::Free;
		private:
			unsigned int voiceIndex;
			void ConfigureControlRegisters();
			void ConfigureVolumeRegister();
			void ConfigureTimerRegister();

			int ComputeVolume() const;
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

			int amplitude;
		};
		static constexpr unsigned int voiceCount = 16;
		Voice voices[voiceCount];

		class Track {
		public:
			Track(std::uint8_t id);

			void Init(PlayingSequence *sequence);
			void Reset();

			void Tick();

			void StartPlaying(std::uint32_t offset);
			void StopPlaying();
			bool GetIsPlaying() const { return isPlaying; }

			void SetInstrument(unsigned int instrumentId);

			void NoteOn(std::uint8_t note, std::uint16_t velocity, std::uint16_t durration);

		private:
			PlayingSequence *sequence;
			const InstrumentBank::BaseInstrument *currentInstrument;

			bool isPlaying;

			bool noteWait;
			bool tieMode;

			const std::uint8_t id;

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

			void ReleaseAllVoices();
			void KillAllVoices();

			void SetNextCommand(std::uint32_t offset);

			std::uint8_t readByteCommand();
			std::uint16_t readShortCommand();
			std::uint32_t readTriByteCommand();
			unsigned int readMidiVarLen();

			friend class Voice;
		};


		static constexpr unsigned int localVariableCount = 16;
		static constexpr unsigned int globalVariableCount = 16;
		static constexpr unsigned int numVariables = localVariableCount + globalVariableCount;

		std::int16_t globalVariables[globalVariableCount];

		class PlayingSequence {
		public:
			void Init();

			void SetVar(std::uint8_t var, std::int16_t val);
			std::int16_t GetVar(std::uint8_t var) const;


			void PlaySequence(const std::uint8_t *sequenceData);
			void AbortSequence();

			void ReleaseAllVoices();
			void KillAllVoices();

			void Update();

			bool isVoiceAllowed(std::uint8_t voiceIndex) const;

		private:
			void StartTrack(std::uint8_t trackId, std::uint32_t offset);

			const LoadedWave &GetWave(unsigned int archiveSlot, unsigned int waveIndex);

			Voice *allocateVoice(InstrumentBank::InstrumentType type);

			void stoppedPlaying(Track *track);

			void TickVoices();
			void TickTracks();

			static constexpr unsigned int trackCount = 16;
			Track tracks[trackCount] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
			Voice *voices[voiceCount] = { nullptr };

			void ResetLocalVars();

			std::int16_t localVariables[localVariableCount];

			std::uint16_t tempo;
			std::uint16_t tempoTimer;
			std::uint8_t sequenceVolume;
			const std::uint8_t *sequenceData;

			const InstrumentBank *bank;
			static constexpr unsigned int numWaveArchs = 4;
			const LoadedWaveArchive *waveArchs[numWaveArchs];

			friend class Track;
			friend class SequencePlayer;
		};

		void setupFifo();
		static void fifoDatagramHandler(int num_bytes, void *userdata);
		void fifoDatagramHandler(int num_bytes);

		void sendFifoSequenceStatus(const PlayingSequence &sequence);

		void setupTimer();
		static void ISR();

		PlayingSequence playingSequence;

		std::uint8_t mainVolume;

		signed int FindFreeVoice(InstrumentBank::InstrumentType type, const PlayingSequence *);

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

	const int AMPL_K = 723;
	const int AMPL_MIN = -AMPL_K;
	const int AMPLITUDE_THRESHOLD = AMPL_MIN << 7;

	extern const uint8_t volumeTable[];

	extern SequencePlayer sequencePlayer;

}

#endif