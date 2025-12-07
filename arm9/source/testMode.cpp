#include "testMode.h"

#include "globals.h"
#include "buttonMan.h"

#include <nds/arm9/input.h>

#include "testSTRM.h"

TestMode::TestMode() {}
TestMode::~TestMode() {}

void TestMode::Load() {
	LoadSDat("MPDS_sound.sdat");

	printf("Getvar: %hi\n", sequencePlayer.GetVar(1));
}

void TestMode::LoadSDat(const std::string &fileName) {
	sdat = std::make_unique<NitroComposer::SDatFile>(fileName);
	sequencePlayer.SetSdat(sdat.get());
	sequenceId = 0;
	printf("Sdat loaded ok.\n");
}

void TestMode::PlaySequence(const std::string &sequenceName) {
	sequencePlayer.PlaySequence(sequenceName);
	printf("Loaded sequence %s.\n", sequenceName.c_str());
}

void TestMode::PlaySequence(unsigned int sequenceId) {
	sequencePlayer.PlaySequence(sequenceId);
	printf("Loaded sequence %s.\n", sdat->GetNameForSequence(sequenceId).c_str());
}

void TestMode::Unload() {
	sdat.reset();
}

void TestMode::Update() {
	if(sequencePlayer.IsPlaying()) {
		if(buttonMan.claimButton(KEY_B)) {
			sequencePlayer.AbortSequence();
		}
	} else {
		if(buttonMan.claimButton(KEY_A)) {
			PlaySequence(sequenceId);
		}

		if(buttonMan.claimButton(KEY_UP)) {
			if(sequenceId >= sdat->GetSequenceCount() - 1) {
				sequenceId = 0;
			} else {
				++sequenceId;
			}
			printf("%d: %s\n", sequenceId, sdat->GetNameForSequence(sequenceId).c_str());
		} else if(buttonMan.claimButton(KEY_DOWN)) {
			if(sequenceId == 0) {
				sequenceId = sdat->GetSequenceCount() - 1;
			} else {
				--sequenceId;
			}
			printf("%d: %s\n", sequenceId, sdat->GetNameForSequence(sequenceId).c_str());
		}

		if(buttonMan.claimButton(KEY_SELECT)) {
			setNextGameMode(std::make_unique<TestSTRM>());
		}
	}
}