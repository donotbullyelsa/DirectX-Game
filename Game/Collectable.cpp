#include "pch.h"
#include "Collectable.h"
#include <iostream>

Collectable::Collectable(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF): CMOGO(_fileName, _pd3dDevice, _EF)
{

}

void Collectable::Collect()
{
	std::cout << "test";
}