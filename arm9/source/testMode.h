#ifndef TESTMODE_H
#define TESTMODE_H

#include "gameMode.h"

#include "nitroComposer.h"

class TestMode : public GameMode {
public:
	TestMode();
	~TestMode();

	void Load() override;
	void Unload() override;
	void Update() override;
private:

	void PlaySequence(const std::string &sequenceName);
	void PlaySequence(unsigned int sequenceId);

	std::unique_ptr<NitroComposer::SDatFile> sdat;

	bool isplaying = false;
	unsigned int sequenceId = 0;

	NitroComposer::SequencePlayer sequencePlayer;
};

#endif