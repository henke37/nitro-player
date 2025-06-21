#ifndef BUTTONMAN_H
#define BUTTONMAN_H

#include <cstdint>
#include <vector>

class ButtonManager {
public:
	ButtonManager();

	bool claimButton(uint32_t button);
	uint32_t claimAnyButton(uint32_t buttons);
	bool claimButton(uint32_t button, unsigned int maxPressAge);
	uint32_t claimAnyButton(uint32_t buttons, unsigned int maxPressAge);

	void Update();

	unsigned int maxPressAge = 5;

private:
	struct ButtonPress {
		ButtonPress(uint32_t);
		unsigned int age;
		uint32_t button;
	};

	std::vector<ButtonPress> presses;

	void agePresses();
};

#endif