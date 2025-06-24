#include "testMode.h"

TestMode::TestMode() {}
TestMode::~TestMode() {}

void TestMode::Load() {
	sdat = std::make_unique<SDatFile>("sound_data.sdat");
	seqPlayer.SetSdat(sdat.get());
	printf("Sdat loaded ok.\n");

	LoadSequence("BGM02DS_REQ");
}

void TestMode::LoadSequence(const std::string &sequenceName) {
	seqPlayer.LoadSequence(sequenceName);
	printf("Loaded sequence %s.\n", sequenceName.c_str());
}

void TestMode::Unload() {
	sdat.reset();
}

void TestMode::Update() {
}