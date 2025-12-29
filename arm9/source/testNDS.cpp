#include "testNDS.h"

#include "nitroComposer/sdatFile.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <nds/arm9/console.h>
#include <nds/input.h>

#include "globals.h"
#include "buttonMan.h"
#include "testMode.h"

std::vector<SDatEntry> TestNDS::sdatEntries;

TestNDS::TestNDS() {}

TestNDS::~TestNDS() {}

void TestNDS::Unload() {
	ndsFile.reset();
}

void TestNDS::Update() {
	if(buttonMan.claimButton(KEY_UP)) {
		selectNextSdat();
		redrawUI();
	} else if(buttonMan.claimButton(KEY_DOWN)) {
		selectPrevSdat();
		redrawUI();
	} else if(buttonMan.claimButton(KEY_LEFT)) {
		selectPrevGame();
		redrawUI();
	} else if(buttonMan.claimButton(KEY_RIGHT)) {
		selectNextGame();
		redrawUI();
	}

	if(buttonMan.claimButton(KEY_A)) {
		setNextGameMode(std::make_unique<TestMode>(sdatEntries[selectedEntry]));
	}
}

void TestNDS::selectPrevSdat() {
	if(selectedEntry == 0) {
		selectedEntry = sdatEntries.size() - 1;
	} else {
		selectedEntry--;
	}
}

void TestNDS::selectNextSdat() {
	if(selectedEntry >= sdatEntries.size() - 1) {
		selectedEntry = 0;
	} else {
		selectedEntry++;
	}
}

void TestNDS::selectPrevGame() {
	std::string currentGameCode = sdatEntries[selectedEntry].gameCode;
	size_t originalEntry = selectedEntry;

	do {
		selectPrevSdat();
	} while(sdatEntries[selectedEntry].gameCode == currentGameCode && selectedEntry != originalEntry);
}

void TestNDS::selectNextGame() {

	std::string currentGameCode = sdatEntries[selectedEntry].gameCode;
	size_t originalEntry = selectedEntry;

	do {
		selectNextSdat();
	} while(sdatEntries[selectedEntry].gameCode == currentGameCode && selectedEntry != originalEntry);
}

void TestNDS::Load() {
	if(sdatEntries.size() > 0) return;
	scanFileSystems();

	printf("Found %d sdats\n",sdatEntries.size());
}

void TestNDS::scanFileSystems() {
	std::string roots[] = {
		"fat:/",
		"sd:/",
		"nitro:/",
		"nand:/",
		"nand2:/"
	};

	for(const auto &root : roots) {
		int a=access(root.c_str(), 0);
		if(a != 0) continue;
		scanFolder(root);
	}
	sdatEntries.shrink_to_fit();
}

void TestNDS::scanFolder(const std::string &path) {
	auto dir = opendir(path.c_str());
	if(!dir) return;

	struct dirent *entry;
	while((entry = readdir(dir)) != nullptr) {
		std::string fullPath = path + entry->d_name;
		if(entry->d_type == DT_DIR) {
			if(entry->d_name[0] == '.') continue;
			fullPath += "/";
			scanFolder(fullPath);
		} else if(entry->d_type == DT_REG) {
			if(entry->d_name[0] == '.') continue;
			if(!fullPath.ends_with(".nds")) continue;

			scanNDSFile(fullPath);
		}
	}
	closedir(dir);
}

void TestNDS::scanNDSFile(const std::string &ndsPath) {
	ndsFile = std::make_unique<NDSFile>(ndsPath);

	auto banner = ndsFile->GetBanner();

	auto enTitle = banner.titles[(int)NDSFile::Banner::TitleLanguage::English];

	// truncate to first line
	size_t newlinePos = enTitle.find(u'\n');
	if(newlinePos != std::u16string::npos) {
		enTitle = enTitle.substr(0, newlinePos);
	}

	for(auto itr = ndsFile->getFileSystemIterator(); !itr.atEnd(); ++itr) {
		if(!itr->name.ends_with(".sdat")) continue;

		sdatEntries.push_back({ ndsPath, itr.getFullPath(), ndsFile->getGameCode(), enTitle });

		//auto sdatStream = ndsFile->OpenFile(itr->fileId);
		//scanSDatFile(std::move(sdatStream));
	}
}

void TestNDS::scanSDatFile(std::unique_ptr<BinaryReadStream> &&sdatStream) {
	auto sdat = std::make_unique<NitroComposer::SDatFile>(std::move(sdatStream));
}

void TestNDS::redrawUI() {
	consoleClear();
	auto &entry = sdatEntries[selectedEntry];
	puts(entry.ndsFile.c_str());
	puts(entry.sdatFile.c_str());
}
