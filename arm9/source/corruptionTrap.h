#ifndef CORRUPTION_TRAP_H
#define CORRUPTION_TRAP_H

#include <cstdint>
#include <cassert>

class CorruptionTrap {
public:
	CorruptionTrap() {
		checkedThis = ~(uintptr_t)this;

		for(unsigned i = 0; i < 32; ++i) {
			bigOldBuffer[i] = checkedThis << i | checkedThis >> i;
		}
	}

	~CorruptionTrap() {
		validate();
	}

	void validate() const {
		assert(~(uintptr_t)this == checkedThis);

		for(unsigned i = 0; i < 32; ++i) {
			assert(bigOldBuffer[i] == (checkedThis << i | checkedThis >> i));
		}
	}
private:
	uintptr_t bigOldBuffer[32];
	uintptr_t checkedThis;
};

#endif