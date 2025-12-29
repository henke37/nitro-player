#ifndef TESTNDS_H
#define TESTNDS_H

#include "gameMode.h"
#include "ndsfile.h"

#include <memory>
#include <string>
#include <vector>

#include "sdatentry.h"

class TestNDS : public GameMode {
public:
	TestNDS();
	~TestNDS();

	void Load() override;
	void Unload() override;
	void Update() override;
private:
	std::unique_ptr<NDSFile> ndsFile;

	void scanFileSystems();
	void scanFolder(const std::string &path);

	void scanNDSFile(const std::string &path);
	void scanSDatFile(std::unique_ptr<BinaryReadStream> &&sdatStream);

	static std::vector<SDatEntry> sdatEntries;

	size_t selectedEntry = 0;
	void redrawUI();
};

#endif