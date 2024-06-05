#ifndef _CMOGO_H_
#define _CMOGO_H_

//=================================================================
//A Class for loading and displaying CMO as GameObjects
//=================================================================

#include "gameobject.h"
#include "Model.h"
#include <string>

using namespace std;
using namespace DirectX;

struct GameData;

namespace MathConst
{
	const double pi = 3.14159265358979323846;
}

class CMOGO : public GameObject
{
public:
	CMOGO(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);
	virtual ~CMOGO();

	virtual void Tick(GameData* _GD) override;
	virtual void Draw(DrawData* _DD) override;

	virtual bool Intersects(const CMOGO& other) const;
	bool grounded = false;
	Vector3 last_pos = { 0.0f, 0.0f, 0.0f };

	BoundingOrientedBox&		getCollider()		noexcept { return m_collider; }
	const BoundingOrientedBox&	getCollider() const noexcept { return m_collider; }

	const float GRAVITY_CONST = 2.0f;
	float gravity = 0.0f;
	bool isGravityEnabled() { return gravity_enabled; }
	void updateGravity();
	void stopGravity();

protected:
	unique_ptr<Model>  m_model;
	BoundingOrientedBox m_collider;

	//needs a slightly different raster state that the VBGOs so create one and let them all use it
	static ID3D11RasterizerState*  s_pRasterState;
	static int m_count;

	bool gravity_enabled = false;
	float getMagnitude(Vector3 v3);
	
};

#endif
