#include "gameMode.h"

#include <cassert>

std::unique_ptr<GameMode> g_mode;
std::unique_ptr<GameMode> g_nextMode;

GameMode::GameMode() {
}

GameMode::~GameMode() {
}

void setNextGameMode(std::unique_ptr<GameMode> nextMode) {
	assert(!g_nextMode);
	g_nextMode=std::move(nextMode);
}