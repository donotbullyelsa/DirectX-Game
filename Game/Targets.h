#pragma once

#include "CMOGO.h"
#include "math.h"

class Targets: public CMOGO
{
public:
	Targets(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);
	Targets(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF, float speed);
	const float SPEED;
	int health = 100;
			
	virtual void Tick(GameData* _GD, Vector2 v2_goto);

private:
	void init();
};