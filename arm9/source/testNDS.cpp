#include "testNDS.h"

#include "nitroComposer/sdatFile.h"

TestNDS::TestNDS() {}

TestNDS::~TestNDS() {}

void TestNDS::Unload() {
	ndsFile.reset();
}

void TestNDS::Update() {}

void TestNDS::Load() {
	ndsFile = std::make_unique<NDSFile>("game.nds");
	puts("Loaded NDS File");

	for(auto itr = ndsFile->getFileSystemIterator(); !itr.atEnd(); ++itr) {
		if(!itr->name.ends_with(".sdat")) continue;
		printf("%s\n",itr->name.c_str());
		auto sdatStream = ndsFile->OpenFile(itr->fileId);
		auto sdat = std::make_unique<NitroComposer::SDatFile>(std::move(sdatStream));
	}
	puts("That's all!");

	//auto sdat = ndsFile->OpenFile("data/MPDS_sound.sdat");
	//puts("Loaded SDAT");
}
