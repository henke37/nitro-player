#ifndef TESTSTRM_H
#define TESTSTRM_H


#include "ndsfile.h"

#include "gameMode.h"
#include "nitroComposer.h"
#include "nitroComposer/streamPlayer.h"

#include <memory>

class TestSTRM : public GameMode {
public:
	TestSTRM();
	TestSTRM(std::unique_ptr<NDSFile> &&nds, std::unique_ptr<NitroComposer::SDatFile> &&sdat);
	~TestSTRM();

	void Load() override;
	void Unload() override;
	void Update() override;
private:
	std::unique_ptr<NDSFile> nds;
	std::unique_ptr<NitroComposer::SDatFile> sdat;

	unsigned int streamId;

	NitroComposer::StreamPlayer player;
};

#endif