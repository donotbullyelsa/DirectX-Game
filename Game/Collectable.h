#pragma once
#include "CMOGO.h"

class Collectable: public CMOGO
{
	

public:
	Collectable(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);

	void Collect();

};

