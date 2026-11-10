// Copyright Epic Games, Inc. All Rights Reserved.

#include "FireFightSumulGameMode.h"
#include "Player/FireFighter.h"

AFireFightSumulGameMode::AFireFightSumulGameMode()
{
	DefaultPawnClass = AFireFighter::StaticClass();
}
