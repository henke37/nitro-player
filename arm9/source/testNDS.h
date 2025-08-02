#ifndef TESTNDS_H
#define TESTNDS_H

#include "gameMode.h"
#include "ndsfile.h"

#include <memory>

class TestNDS : public GameMode {
public:
	TestNDS();
	~TestNDS();

	void Load() override;
	void Unload() override;
	void Update() override;
private:
	std::unique_ptr<NDSFile> ndsFile;
};

#endif