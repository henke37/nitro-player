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

	void LoadSequence(const std::string &sequenceName);
	void LoadSequence(unsigned int sequenceId);

	std::unique_ptr<SDatFile> sdat;

	SequencePlayer seqPlayer;
};

#endif