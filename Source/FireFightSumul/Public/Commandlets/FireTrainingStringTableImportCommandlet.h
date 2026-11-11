#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FireTrainingStringTableImportCommandlet.generated.h"

/** Imports the training CSV into its String Table asset and saves the package. */
UCLASS()
class FIREFIGHTSUMUL_API UFireTrainingStringTableImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFireTrainingStringTableImportCommandlet(const FObjectInitializer& ObjectInitializer);
	virtual int32 Main(const FString& Params) override;
};
