#include "testMode.h"

#include "globals.h"
#include "buttonMan.h"

#include <nds/arm9/input.h>

#include "testSTRM.h"
#include "testNDS.h"

TestMode::TestMode(const SDatEntry &sdatEntry) {
	LoadSDat(sdatEntry);
}
TestMode::~TestMode() {}

void TestMode::Load() {
	InitSDat();
}

void TestMode::LoadSDat(const SDatEntry &sdatEntry) {
	nds = std::make_unique<NDSFile>(sdatEntry.ndsFile);
	sdat = std::make_unique<NitroComposer::SDatFile>(nds->OpenFile(sdatEntry.sdatFile));
}

void TestMode::LoadSDat(const std::string &fileName) {
	sdat = std::make_unique<NitroComposer::SDatFile>(fileName);
}

void TestMode::InitSDat() {
	sequencePlayer.SetSdat(sdat.get());
	printf("Sdat loaded ok.\n");

	if(sdat->GetSequenceCount()==0) {
		printf("No sequences in sdat!\n");
		setNextGameMode(std::make_unique<TestNDS>());
		return;
	}

	sequenceId = 0;
	while(!sdat->GetSequenceInfo(sequenceId)) {
		nextSequence();
	}
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

		if(buttonMan.claimButton(KEY_B)) {
			sequencePlayer.KillSequence();
		}

		if(buttonMan.claimButton(KEY_UP)) {
			nextSequence();
			printf("%d: %s\n", sequenceId, sdat->GetNameForSequence(sequenceId).c_str());
		} else if(buttonMan.claimButton(KEY_DOWN)) {
			prevSequence();
			printf("%d: %s\n", sequenceId, sdat->GetNameForSequence(sequenceId).c_str());
		}

		if(buttonMan.claimButton(KEY_START)) {
			setNextGameMode(std::make_unique<TestNDS>());
		}

		if(buttonMan.claimButton(KEY_SELECT)) {
			setNextGameMode(std::make_unique<TestSTRM>());
		}
	}
}

void TestMode::prevSequence() {
	do {
		if(sequenceId == 0) {
			sequenceId = sdat->GetSequenceCount() - 1;
		} else {
			--sequenceId;
		}
	} while(!sdat->IsValidSequence(sequenceId));
}

void TestMode::nextSequence() {
	do {
		if(sequenceId >= sdat->GetSequenceCount() - 1) {
			sequenceId = 0;
		} else {
			++sequenceId;
		}
	} while(!sdat->IsValidSequence(sequenceId));
}
