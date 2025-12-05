#include "testSTRM.h"

TestSTRM::TestSTRM() : player(4092, 1, 2, 3) {}

TestSTRM::~TestSTRM() {}

void TestSTRM::Unload() {}

void TestSTRM::Load() {
	sdat = std::make_unique<NitroComposer::SDatFile>("sound_data.sdat");

	player.SetSdat(sdat.get());
	player.PlayStream("STRM_BGM03DS_REQ");

	puts("Loaded STRM.");
}

void TestSTRM::Update() {}
