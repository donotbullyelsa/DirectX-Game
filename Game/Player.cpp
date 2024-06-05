#include "pch.h"
#include "Player.h"
#include <dinput.h>
#include "GameData.h"
#include <iostream>

Player::Player(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF) : CMOGO(_fileName, _pd3dDevice, _EF)
{
	//any special set up for Player goes here
	m_fudge = Matrix::CreateRotationY(XM_PI);

	m_pos.y = 10.0f;

	SetDrag(0.7);
	SetPhysicsOn(true);

	gravity_enabled = true;
}

Player::~Player()
{
	//tidy up anything I've created
}


void Player::Tick(GameData* _GD)
{
	switch (_GD->m_GS)
	{
		case GS_PLAY_MAIN_CAM:
		case GS_PLAY_TPS_CAM:
		{
			//TURN AND FORWARD CONTROL HERE
			Vector3 forwardMove = 200.0f * Vector3::Forward;
			Matrix rotMove = Matrix::CreateRotationY(m_yaw);
			forwardMove = Vector3::Transform(forwardMove, rotMove);

			bool keyboard_movement = false;
			if (_GD->m_KBS.W)
			{
				m_acc += forwardMove;
				keyboard_movement = true;
			}
			if (_GD->m_KBS.S)
			{
				m_acc -= forwardMove;
				keyboard_movement = true;
			}

			if (!keyboard_movement)
			{
				//MOUSE CONTROL SCHEME HERE
				float speed = 10.0f;
				m_acc.x += speed * _GD->m_MS.x;
				m_acc.z += speed * _GD->m_MS.y;
			}

			break;
		}
	}

	//change orinetation of player
	float rotSpeed = 2.0f * _GD->m_dt;
	if (_GD->m_KBS.A)
	{
		m_yaw += rotSpeed;
	}
	if (_GD->m_KBS.D)
	{
		m_yaw -= rotSpeed;
	}

	/**
	//move player up and down
	if (_GD->m_KBS.R)
	{
		m_acc.y += 40.0f;
		gravity = -GRAVITY_CONST;
	}

	if (_GD->m_KBS.F)
	{
		m_acc.y -= 40.0f;
	}

	//jump
	if (_GD->m_KBS.K)
	{
		if (grounded)
		{
			m_vel.y += 100.0f;
			gravity = -GRAVITY_CONST;
		}
	}
	**/

	//limit motion of the player
	float length = m_pos.Length();
	float maxLength = 500.0f;
	if (length > maxLength)
	{
		m_pos.Normalize();
		m_pos *= maxLength;
		m_vel *= -0.9; //VERY simple bounce back
	}

	//destroy bullets that goes too far
	for (int i = 0; i < bullet.size(); i++)
	{
		Vector3 v3_dif = bullet[i]->GetPos() - GetPos();
		if (getMagnitude(v3_dif) > 1000.0f)
		{
			bullet[i].reset();
			bullet.erase(bullet.begin() + i);
		}
	}

	//if reloading is done
	if ((weapon_cooldown <= _GD->m_dt) && (weapon_cooldown > 0))
	{
		//text update
		ammo_text.lock()->SetText(to_string(ammo) + " / 6");
	}

	//update weapon cooldown clock
	weapon_cooldown -= _GD->m_dt;

	//apply my base behaviour
	CMOGO::Tick(_GD);
}

//return the string of ammo text
void Player::Shoot(std::shared_ptr<Bullet> _bullet)
{
	bullet.push_back(_bullet);
	bullet[bullet.size() - 1]->SetVelocity(Vector3(0.0f, 0.0f, -200.0f), Vector3(GetPitch(), GetYaw(), GetRoll()));

	ammo--;
	//text update
	ammo_text.lock()->SetText("Ammo: " + to_string(ammo) + " / " + to_string(AMMO_LIMIT));
}

void Player::Reload()
{
	//reset cooldown timer
	weapon_cooldown = COOLDOWN_TIME;

	//reload
	ammo = AMMO_LIMIT;
	
	//text update
	ammo_text.lock()->SetText("Reloading...");
}

bool Player::canSpawnBullet()
{
	return ((weapon_cooldown <= 0) && (!isAmmoRunOut()));
}

bool Player::isAmmoRunOut()
{
	return (ammo <= 0);
}
