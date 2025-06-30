#include "testMode.h"

TestMode::TestMode() {}
TestMode::~TestMode() {}

void TestMode::Load() {
	sdat = std::make_unique<SDatFile>("sound_data.sdat");
	seqPlayer.SetSdat(sdat.get());
	printf("Sdat loaded ok.\n");

	LoadSequence(2);// "BGM02DS_REQ");

	printf("Getvar: %hi\n", seqPlayer.GetVar(1));
}

void TestMode::LoadSequence(const std::string &sequenceName) {
	seqPlayer.LoadSequence(sequenceName);
	printf("Loaded sequence %s.\n", sequenceName.c_str());
}

void TestMode::LoadSequence(unsigned int sequenceId) {
	seqPlayer.LoadSequence(sequenceId);
	printf("Loaded sequence %s.\n", sdat->GetNameForSequence(sequenceId).c_str());
}

void TestMode::Unload() {
	sdat.reset();
}

void TestMode::Update() {
}