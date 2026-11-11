#include "MapObject/Fire/FireMaterialTags.h"

#include "GameFramework/Actor.h"

namespace
{
	const FString CanonicalPrefix(TEXT("FireMaterial."));
	const FString LegacyPrefix(TEXT("Material."));

	FString NormalizeMaterialSuffix(const FString& Suffix)
	{
		if (Suffix.Equals(TEXT("Oil"), ESearchCase::IgnoreCase) ||
			Suffix.Equals(TEXT("CookingOil"), ESearchCase::IgnoreCase))
		{
			return TEXT("CookingOil");
		}
		if (Suffix.Equals(TEXT("Electro"), ESearchCase::IgnoreCase) ||
			Suffix.Equals(TEXT("Electrical"), ESearchCase::IgnoreCase) ||
			Suffix.Equals(TEXT("ElectricalEnergized"), ESearchCase::IgnoreCase))
		{
			return TEXT("ElectricalEnergized");
		}

		static const TCHAR* KnownSuffixes[] =
		{
			TEXT("Fabric"),
			TEXT("Wood"),
			TEXT("Paper"),
			TEXT("Plastic"),
			TEXT("Trash"),
			TEXT("Metal"),
			TEXT("NonCombustible"),
			TEXT("FlammableLiquid")
		};
		for (const TCHAR* KnownSuffix : KnownSuffixes)
		{
			if (Suffix.Equals(KnownSuffix, ESearchCase::IgnoreCase))
			{
				return KnownSuffix;
			}
		}

		return Suffix;
	}

	bool TryExtractMaterialSuffix(const FString& TagString, FString& OutSuffix)
	{
		if (TagString.StartsWith(CanonicalPrefix, ESearchCase::IgnoreCase))
		{
			OutSuffix = TagString.RightChop(CanonicalPrefix.Len());
			return !OutSuffix.IsEmpty();
		}
		if (TagString.StartsWith(LegacyPrefix, ESearchCase::IgnoreCase))
		{
			OutSuffix = TagString.RightChop(LegacyPrefix.Len());
			return !OutSuffix.IsEmpty();
		}

		const FString Normalized = NormalizeMaterialSuffix(TagString);
		if (!Normalized.Equals(TagString, ESearchCase::CaseSensitive) ||
			TagString.Equals(TEXT("Fabric"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("Wood"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("Paper"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("Plastic"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("Trash"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("Metal"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("NonCombustible"), ESearchCase::IgnoreCase) ||
			TagString.Equals(TEXT("FlammableLiquid"), ESearchCase::IgnoreCase))
		{
			OutSuffix = TagString;
			return true;
		}

		return false;
	}
}

FName FireMaterialTags::Canonicalize(FName MaterialTag)
{
	if (MaterialTag.IsNone())
	{
		return NAME_None;
	}

	FString Suffix;
	const FString TagString = MaterialTag.ToString();
	if (!TryExtractMaterialSuffix(TagString, Suffix))
	{
		// Data Assets can still use project-specific material keys. Preserve them.
		return MaterialTag;
	}

	Suffix = NormalizeMaterialSuffix(Suffix);
	return Suffix.IsEmpty() ? NAME_None : FName(*(CanonicalPrefix + Suffix));
}

bool FireMaterialTags::IsActorMaterialTag(FName ActorTag)
{
	if (ActorTag.IsNone())
	{
		return false;
	}

	FString Suffix;
	return TryExtractMaterialSuffix(ActorTag.ToString(), Suffix);
}

EFireMaterialTagResolveResult FireMaterialTags::ResolveActorMaterialTag(
	const AActor& Actor, FName& OutMaterialTag)
{
	OutMaterialTag = NAME_None;
	TSet<FName> MaterialTags;
	for (const FName ActorTag : Actor.Tags)
	{
		if (IsActorMaterialTag(ActorTag))
		{
			MaterialTags.Add(Canonicalize(ActorTag));
		}
	}

	if (MaterialTags.IsEmpty())
	{
		return EFireMaterialTagResolveResult::Missing;
	}
	if (MaterialTags.Num() > 1)
	{
		return EFireMaterialTagResolveResult::Ambiguous;
	}

	OutMaterialTag = *MaterialTags.CreateConstIterator();
	return EFireMaterialTagResolveResult::Found;
}
