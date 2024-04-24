// Copyright Epic Games, Inc. All Rights Reserved.

#include "GEII_Project1GameMode.h"
#include "GEII_Project1Character.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"


AGEII_Project1GameMode::AGEII_Project1GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void AGEII_Project1GameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (HUDWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}
}