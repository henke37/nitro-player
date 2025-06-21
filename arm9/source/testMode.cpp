#include "testMode.h"

TestMode::TestMode() {}
TestMode::~TestMode() {}

void TestMode::Load() {
	sdat = std::make_unique<SDatFile>("sound_data.sdat");
	printf("Sdat loaded ok.");
}

void TestMode::Unload() {
	sdat.reset();
}

void TestMode::Update() {
}