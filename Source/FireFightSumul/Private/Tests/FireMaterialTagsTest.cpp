#if WITH_DEV_AUTOMATION_TESTS

#include "MapObject/Fire/FireMaterialTags.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireMaterialTagsCanonicalizationTest,
	"FireFightSumul.Fire.MaterialTags.Canonicalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFireMaterialTagsCanonicalizationTest::RunTest(const FString& Parameters)
{
	struct FMaterialTagCase
	{
		const TCHAR* Input;
		const TCHAR* Expected;
	};

	const FMaterialTagCase Cases[] =
	{
		{ TEXT("oil"), TEXT("FireMaterial.CookingOil") },
		{ TEXT("fabric"), TEXT("FireMaterial.Fabric") },
		{ TEXT("wood"), TEXT("FireMaterial.Wood") },
		{ TEXT("paper"), TEXT("FireMaterial.Paper") },
		{ TEXT("electro"), TEXT("FireMaterial.ElectricalEnergized") },
		{ TEXT("Material.Plastic"), TEXT("FireMaterial.Plastic") },
		{ TEXT("FireMaterial.NonCombustible"), TEXT("FireMaterial.NonCombustible") }
	};

	for (const FMaterialTagCase& TestCase : Cases)
	{
		TestEqual(FString::Printf(TEXT("Canonicalize %s"), TestCase.Input),
			FireMaterialTags::Canonicalize(FName(TestCase.Input)), FName(TestCase.Expected));
		TestTrue(FString::Printf(TEXT("Recognize %s as a material Actor Tag"), TestCase.Input),
			FireMaterialTags::IsActorMaterialTag(FName(TestCase.Input)));
	}

	TestFalse(TEXT("Scenario start tags are not material tags"),
		FireMaterialTags::IsActorMaterialTag(FName(TEXT("ScenarioStart.KitchenPan"))));
	return true;
}

#endif
