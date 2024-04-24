// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GEII_Project1GameMode.generated.h"

UCLASS(minimalapi)
class AGEII_Project1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGEII_Project1GameMode();

	void BeginPlay();


protected:

	/** The Widget class to use for our HUD screen */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Power", Meta = (BlueprintProtected = true))
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	/** The instance of the HUD */
	UPROPERTY()
	class UUserWidget* CurrentWidget;


};