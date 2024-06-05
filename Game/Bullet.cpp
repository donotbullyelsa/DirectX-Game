#include "pch.h"
#include "Bullet.h"
#include <cmath>
#include <string>
#include <iostream>

Bullet::Bullet(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF): CMOGO(_fileName, _pd3dDevice, _EF), DAMAGE(10)
{
	init();
}

Bullet::Bullet(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF, float damage): CMOGO(_fileName, _pd3dDevice, _EF), DAMAGE(damage)
{
	init();
}

void Bullet::init()
{

	SetDrag(0.7);
	SetPhysicsOn(true);

	gravity_enabled = false;
}

void Bullet::Tick(GameData* _GD)
{
	m_vel = velocity;
	m_acc = Vector3::Zero;
	CMOGO::Tick(_GD);
}

void Bullet::SetVelocity(Vector3 v3, Vector3 r3)
{
	SetYaw(r3.y);
	//SetYaw(getAngleFromVector(m_vel) - 3.14 / 2);
	//std::cout << '\n' << getAngleFromVector(m_vel) << '\n';
	//std::cout << m_vel.z << '\n';
	//std::cout << m_vel.x << '\n';
	velocity = RotateV3(v3, r3);
}

Vector3 Bullet::RotateV3(Vector3 v3, Vector3 r3)
{
	v3 = Vector3(getMagnitude(v3) * cos(-r3.y + getAngleFromVector(v3)), v3.y, getMagnitude(v3) * sin(-r3.y + getAngleFromVector(v3)));

	return v3;
}

//in radian
float Bullet::getAngleFromVector(Vector3 v3)
{
	float x = atan(v3.z / v3.x);
	return x;
}