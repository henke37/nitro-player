#include "testSTRM.h"

TestSTRM::TestSTRM() {}

TestSTRM::~TestSTRM() {}

void TestSTRM::Unload() {}

void TestSTRM::Load() {
	sdat = std::make_unique<NitroComposer::SDatFile>("sound_data.sdat");

	auto &info = sdat->GetStreamInfo("STRM_BGM03DS_REQ");
	auto strm = sdat->OpenStream(info);
	puts("Loaded STRM.");
}

void TestSTRM::Update() {}
