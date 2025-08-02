#include "testNDS.h"

TestNDS::TestNDS() {}

TestNDS::~TestNDS() {}

void TestNDS::Unload() {
	ndsFile.reset();
}

void TestNDS::Update() {}

void TestNDS::Load() {
	ndsFile = std::make_unique<NDSFile>("game.nds");
	puts("Loaded NDS File");

	auto sdat = ndsFile->OpenFile("data/MPDS_sound.sdat");
	puts("Loaded SDAT");
}
