#include "buttonMan.h"

#include <nds.h>

ButtonManager buttonMan;

ButtonManager::ButtonManager() {}

bool ButtonManager::claimButton(uint32_t button) {
	for(auto itr = presses.begin(); itr != presses.end();) {
		auto &elm = *itr;

		if(elm.button == button) {
			itr = presses.erase(itr);
			return true;
		} else {
			++itr;
		}
	}
	return false;
}

uint32_t ButtonManager::claimAnyButton(uint32_t buttons) {
	for(auto itr = presses.begin(); itr != presses.end();) {
		auto &elm = *itr;

		if(elm.button & buttons) {
			itr = presses.erase(itr);
			return elm.button;
		} else {
			++itr;
		}
	}
	return 0;
}

void ButtonManager::Update() {
	agePresses();

	int newButtons = keysDown();
	if(newButtons) {
		presses.emplace_back(newButtons);
	}
}

void ButtonManager::agePresses() {
	for(auto itr = presses.begin(); itr != presses.end();) {
		auto &elm = *itr;

		++elm.age;

		if(elm.age >= maxPressAge) {
			itr = presses.erase(itr);
		} else {
			++itr;
		}
	}

}

ButtonManager::ButtonPress::ButtonPress(uint32_t button) : age(0), button(button) {}
