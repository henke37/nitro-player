#include "testSTRM.h"

#include "globals.h"
#include "buttonMan.h"

#include <nds/input.h>

TestSTRM::TestSTRM() : 
	streamId(0),
	player(4092, 1, 2, 3) {}

TestSTRM::~TestSTRM() {}

void TestSTRM::Unload() {}

void TestSTRM::Load() {
	sdat = std::make_unique<NitroComposer::SDatFile>("sound_data.sdat");

	player.SetSdat(sdat.get());

	puts("Loaded SDAT.");
}

void TestSTRM::Update() {
	if(player.IsPlaying()) {
		if(buttonMan.claimButton(KEY_B)) {
			player.StopStream(false);
		}
	} else {
		if(buttonMan.claimButton(KEY_A)) {
			player.PlayStream(streamId);
		}

		if(buttonMan.claimButton(KEY_UP)) {
			if(streamId >= sdat->GetStreamCount() - 1) {
				streamId = 0;
			} else {
				++streamId;
			}
			printf("%d: %s\n", streamId, sdat->GetNameForStream(streamId).c_str());
		} else if(buttonMan.claimButton(KEY_DOWN)) {
			if(streamId == 0) {
				streamId = sdat->GetStreamCount() - 1;
			} else {
				--streamId;
			}
			printf("%d: %s\n", streamId, sdat->GetNameForStream(streamId).c_str());
		}
	}
}
