#include "testMode.h"
#include <nds/arm9/input.h>

TestMode::TestMode() {}
TestMode::~TestMode() {}

void TestMode::Load() {
	sdat = std::make_unique<NitroComposer::SDatFile>("sound_data.sdat");
	NitroComposer::sequencePlayer.SetSdat(sdat.get());
	printf("Sdat loaded ok.\n");

	PlaySequence("BGM02DS_REQ");

	printf("Getvar: %hi\n", NitroComposer::sequencePlayer.GetVar(1));
}

void TestMode::PlaySequence(const std::string &sequenceName) {
	isplaying = true;
	NitroComposer::sequencePlayer.PlaySequence(sequenceName);
	printf("Loaded sequence %s.\n", sequenceName.c_str());
}

void TestMode::PlaySequence(unsigned int sequenceId) {
	isplaying = true;
	NitroComposer::sequencePlayer.PlaySequence(sequenceId);
	printf("Loaded sequence %s.\n", sdat->GetNameForSequence(sequenceId).c_str());
}

void TestMode::Unload() {
	sdat.reset();
}

void TestMode::Update() {
	if(isplaying) {
		if(keysDown() & KEY_B) {
			isplaying = false;
			NitroComposer::sequencePlayer.AbortSequence();
		}
	} else {
		if(keysDown() & KEY_A) {
			PlaySequence(sequenceId);
		}

		if(keysDown() & KEY_UP) {
			if(sequenceId >= sdat->GetSequenceCount() - 1) {
				sequenceId = 0;
			} else {
				++sequenceId;
			}
			printf("%d: %s\n", sequenceId, sdat->GetNameForSequence(sequenceId).c_str());
		} else if(keysDown() & KEY_DOWN) {
			if(sequenceId == 0) {
				sequenceId = sdat->GetSequenceCount() - 1;
			} else {
				--sequenceId;
			}
			printf("%d: %s\n", sequenceId, sdat->GetNameForSequence(sequenceId).c_str());
		}


	}
}