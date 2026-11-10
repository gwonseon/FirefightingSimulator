#include "MapObjects/MapObject.h"

#include "Components/SceneComponent.h"

AMapObject::AMapObject()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
