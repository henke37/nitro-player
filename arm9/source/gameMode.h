#ifndef GAMEMODE_H
#define GAMEMODE_H

#include <memory>

class GameMode {
public:
	virtual ~GameMode();
	GameMode();
	
	virtual void Load() = 0;
	virtual void Unload() = 0;
	
	virtual void Update() = 0;
};


void setNextGameMode(std::unique_ptr<GameMode>);

#endif