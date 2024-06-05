#include "CMOGO.h"

#pragma once

class Bullet: public CMOGO
{
public:
	Bullet(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);
	Bullet(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF, float damage);
	void Tick(GameData* _GD) override;

	void SetVelocity(Vector3 v3, Vector3 r3);
	const float DAMAGE;

private:
	void init();

	Vector3 RotateV3(Vector3 v3, Vector3 r3);
	Vector3 velocity;
	float getAngleFromVector(Vector3 v3);
};

