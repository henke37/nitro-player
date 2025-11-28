#ifndef TESTSTRM_H
#define TESTSTRM_H

#include "gameMode.h"
#include "nitroComposer.h"
#include "nitroComposer/streamPlayer.h"

#include <memory>

class TestSTRM : public GameMode {
public:
	TestSTRM();
	~TestSTRM();

	void Load() override;
	void Unload() override;
	void Update() override;
private:
	std::unique_ptr<NitroComposer::SDatFile> sdat;

	NitroComposer::StreamPlayer player;
};

#endif