#ifndef TESTMODE_H
#define TESTMODE_H

#include "gameMode.h"

#include "ndsfile.h"
#include "nitroComposer.h"
#include "sdatentry.h"

class TestMode : public GameMode {
public:
	TestMode(const SDatEntry &sdatEntry);
	~TestMode();

	void Load() override;
	void Unload() override;
	void Update() override;
private:


	void LoadSDat(const SDatEntry &sdatEntry);
	void LoadSDat(const std::string &fileName);
	void InitSDat();

	void PlaySequence(const std::string &sequenceName);
	void PlaySequence(unsigned int sequenceId);

	std::unique_ptr<NitroComposer::SDatFile> sdat;
	std::unique_ptr<NDSFile> nds;

	unsigned int sequenceId = 0;
	void prevSequence();
	void nextSequence();

	NitroComposer::SequencePlayer sequencePlayer;
};

#endif