#pragma once

#include "CoreMinimal.h"

class AActor;

enum class EFireMaterialTagResolveResult : uint8
{
	Found,
	Missing,
	Ambiguous
};

namespace FireMaterialTags
{
	FIREFIGHTSUMUL_API FName Canonicalize(FName MaterialTag);
	FIREFIGHTSUMUL_API bool IsActorMaterialTag(FName ActorTag);
	FIREFIGHTSUMUL_API EFireMaterialTagResolveResult ResolveActorMaterialTag(
		const AActor& Actor, FName& OutMaterialTag);
}
