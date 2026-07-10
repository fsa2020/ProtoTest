// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PbTestActor.generated.h"

UCLASS()
class PROTOTEST_API APbTestActor : public AActor
{
	GENERATED_BODY()

public:
	APbTestActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
