#include "testMode.h"

TestMode::TestMode() {}
TestMode::~TestMode() {}

void TestMode::Load() {
	sdat = std::make_unique<NitroComposer::SDatFile>("sound_data.sdat");
	seqPlayer.SetSdat(sdat.get());
	printf("Sdat loaded ok.\n");

	PlaySequence("BGM02DS_REQ");

	printf("Getvar: %hi\n", seqPlayer.GetVar(1));
}

void TestMode::PlaySequence(const std::string &sequenceName) {
	seqPlayer.PlaySequence(sequenceName);
	printf("Loaded sequence %s.\n", sequenceName.c_str());
}

void TestMode::PlaySequence(unsigned int sequenceId) {
	seqPlayer.PlaySequence(sequenceId);
	printf("Loaded sequence %s.\n", sdat->GetNameForSequence(sequenceId).c_str());
}

void TestMode::Unload() {
	sdat.reset();
}

void TestMode::Update() {
}