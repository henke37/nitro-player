#ifndef NITROCOMPOSER_SEQUENCEPLAYER_H
#define NITROCOMPOSER_SEQUENCEPLAYER_H

#include "nitroComposer/bank.h"
#include "nitroComposer/wave.h"

#include <cstdint>
#include <memory>

namespace NitroComposer {

	class LoadedWaveArchive;
	class StreamPlayer;

	class SequencePlayer {
	public:
		void Init();

		void ReserveChannel(std::uint8_t hwChannel);
		void UnreserveChannel(std::uint8_t hwChannel);
	private:
		void Update();
		void UpdateVoices();

		class PlayingSequence;
		class Track;
		class Voice;

		PlayingSequence *GetPlayingSequence(std::int32_t playerId) { return &playingSequence; }
		const PlayingSequence *GetPlayingSequence(std::int32_t playerId) const { return &playingSequence; }

		enum class VoiceState : std::uint8_t {
			Free,
			Attacking,
			Decaying,
			Sustaining,
			Releasing
		};


		enum class ModulationMode : std::uint8_t {
			Vibrato = 0,//pitch
			Tremolo = 1,//volume
			Pan = 2
		};

		class Voice {
		public:
			Voice(std::uint8_t voiceIndex);

			void StartNote(Track *track, const InstrumentBank::LeafInstrument *instrument, std::uint8_t note, std::uint8_t velocity, unsigned int length);
			void NextTieNote(std::uint8_t note, std::uint8_t velocity);

			void Tick();
			void Update();

			bool IsPulseVoice() const { return voiceIndex >= 8 && voiceIndex <= 13; }
			bool IsNoiseVoice() const { return voiceIndex >= 14; }

			const Track *GetTrack() const { return track; }
			Track *GetTrack() { return track; }
			const std::uint8_t GetVoiceIndex() const { return voiceIndex; }

			void Release();
			void Kill();

			VoiceState GetState() const { return state; }

			int ComputeVolume() const;
			std::uint8_t ComputePan() const;

		private:
			void ConfigureControlRegisters();
			void ConfigureVolumeRegister();
			void ConfigureTimerRegister();

			bool IsHWChannelActive() const;

			const InstrumentBank::LeafInstrument *currentInstrument;
			SequencePlayer *player;
			Track *track;

			int amplitude;

			unsigned int length;
			std::uint8_t note;
			std::uint8_t velocity;

			VoiceState state = VoiceState::Free;

			const std::uint8_t voiceIndex;

			void SetupPortamento();
			void UpdatePitchSweep();
			std::uint32_t sweepCounter;
			std::uint32_t sweepLength;
			std::int16_t sweepPitch;
			bool IsPitchSweeping() const;

			void UpdateModulation();
			bool IsModulationActive(ModulationMode mode) const;
			int GetModulationValue() const;
			std::uint16_t modCounter;
			std::uint16_t modDelayCounter;

			std::uint8_t GetAttack() const;
			std::uint8_t GetDecay() const;
			std::uint8_t GetSustain() const;
			std::uint8_t GetRelease() const;
		};

		class Track {
		public:
			Track();

			void Init(PlayingSequence *sequence);
			void Reset();

			void Tick();

			void StartPlaying(std::ptrdiff_t offset);
			void StopPlaying();
			bool GetIsPlaying() const { return isPlaying; }

			void NoteOn(std::uint8_t note, std::uint16_t velocity, std::uint16_t durration);

			enum class MuteMode {
				Clear,
				Mute,
				MuteAndRelease,
				MuteAndKill
			};
			void SetMute(MuteMode mode);

			std::uint8_t GetPriority() const;

			const PlayingSequence *GetSequence() const { return sequence; }
			PlayingSequence *GetSequence() { return sequence; }

		private:
			PlayingSequence *sequence;
			std::uint32_t instrumentId;
			Voice *tieVoice;

			const std::uint8_t *nextCommand;

			int waitCounter;

			struct {
				bool isPlaying : 1;
				bool noteWait : 1;
				bool tieMode : 1;
				bool muted : 1;
				bool lastComparisonResult : 1;
				bool portamento : 1;
				bool waitVoiceComplete : 1;
			};

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

			std::uint8_t modDepth;
			std::uint8_t modSpeed;
			std::uint16_t modDelay;
			ModulationMode modMode;
			std::uint8_t modRange;

			std::int16_t sweepPitch;
			std::uint8_t lastPlayedNote;
			std::uint8_t portaTime;

			std::int8_t transpose;

			enum class StackEntryType : std::uint8_t {
				Call,
				Loop
			};

			struct StackEntry {
				const std::uint8_t *nextCommand;
				unsigned int loopCounter;
				StackEntryType type;
			};

			std::uint8_t stackPointer;
			StackEntry stack[4];

			void ExecuteNextCommand();
			void ExecuteNextRandomCommand();
			void ExecuteNextVarCommand();

			void skipCommandArgs(std::uint8_t command);
			void skipCommandRandomArgs(std::uint8_t command);
			void skipCommandVarArgs(std::uint8_t command);

			void NoteOn(std::uint8_t note, std::uint8_t velocity, unsigned int length);
			Voice *NoteOnReal(std::uint8_t note, std::uint8_t velocity, unsigned int length);

			std::uint8_t GetTransposedNote(std::uint8_t note) const;

			const InstrumentBank::LeafInstrument *ResolveInstrumentForNote(std::uint8_t note) const;

			std::uint8_t GetId() const;

			void ReleaseAllVoices();
			void KillAllVoices();

			void voiceCompleted(const Voice *voice);

			void SetNextCommand(std::ptrdiff_t offset);

			std::uint8_t readByteCommand();
			std::uint16_t readShortCommand();
			std::uint32_t readTriByteCommand();
			unsigned int readMidiVarLen();

			size_t getCommandBytesLeft() const;

			friend class Voice;
		};
		static constexpr unsigned int voiceCount = 16;

		static constexpr unsigned int localVariableCount = 16;
		static constexpr unsigned int globalVariableCount = 16;
		static constexpr unsigned int numVariables = localVariableCount + globalVariableCount;

		std::int16_t globalVariables[globalVariableCount];

		class PlayingSequence {
		public:
			void Init();

			void SetVar(std::uint8_t var, std::int16_t val);
			std::int16_t GetVar(std::uint8_t var) const;

			void PlaySequence(const std::uint8_t *sequenceData, size_t length, std::ptrdiff_t startPos=0);
			void AbortSequence(bool killVoices);

			void ReleaseAllVoices();
			void KillAllVoices();

			void Update();

			void SetTrackMute(std::uint8_t trackId, Track::MuteMode mode);

			bool isVoiceAllowed(std::uint8_t voiceIndex) const;

		private:
			void StartTrack(std::uint8_t trackId, std::ptrdiff_t offset);

			const LoadedWave &GetWave(unsigned int archiveSlot, unsigned int waveIndex);

			Voice *allocateVoice(InstrumentBank::InstrumentType type, const Track *track);

			void stoppedPlaying(Track *track);

			void TickVoices();
			void TickTracks();

			void ResetLocalVars();
			void ResetTracks();

			std::uint16_t tempo;
			std::uint16_t tempoTimer;
			std::uint8_t sequenceVolume;
			std::uint8_t priority;

			std::uint16_t allowedChannels;

			const std::uint8_t *sequenceData;
			size_t sequenceDataLength;

			const InstrumentBank *bank;
			static constexpr unsigned int numWaveArchs = 4;
			const LoadedWaveArchive *waveArchs[numWaveArchs];

			Voice *voices[voiceCount] = { nullptr };

			std::int16_t localVariables[localVariableCount];

			static constexpr unsigned int trackCount = 16;
			Track tracks[trackCount];
			unsigned int IdForTrack(const Track *) const;

			friend class Track;
			friend class SequencePlayer;
		};

		void setupFifo();
		static void fifoDatagramHandler(int num_bytes, void *userdata);
		void fifoDatagramHandler(int num_bytes);

		void sendFifoSequenceStatus(const PlayingSequence &sequence);

		void setupTimer();
		static void ISR();

		std::uint8_t mainVolume;

		std::uint16_t externalChannelReservations;

		Voice voices[voiceCount] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

		PlayingSequence playingSequence;

		signed int FindFreeVoice(InstrumentBank::InstrumentType type, const Track *track);

		bool isVoiceAllowed(std::uint8_t voiceIndex, const PlayingSequence *sequence) const;
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