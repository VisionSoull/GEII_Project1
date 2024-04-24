// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "GEII_Project1Character.h"
#include "GEII_Project1Projectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"

// Define custom trace channels
#define ECC_PortalTraceChannel ECC_GameTraceChannel2

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(2.0f, 2.0f, 2.0f);

	ObjectTypes.Empty();
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PortalTraceChannel));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
}


void UTP_WeaponComponent::Fire(TSubclassOf<class AGEII_Project1Projectile> Projectile)
{
	// Perform the line trace 
	bool SuitableWall = UTP_WeaponComponent::PerformLineTrace();

	if (SuitableWall)
	{

		if (Character == nullptr || Character->GetController() == nullptr)
		{
			return;
		}

		// Try and fire a projectile
		if (Projectile != nullptr)
		{
			UWorld* const World = GetWorld();
			if (World != nullptr)
			{

				APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
				const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// Spawn Projectile
				World->SpawnActor<AGEII_Project1Projectile>(Projectile, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}

		// Try and play the sound if specified
		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
		}

		// Try and play a firing animation if specified
		if (FireAnimation != nullptr)
		{
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
			if (AnimInstance != nullptr)
			{
				AnimInstance->Montage_Play(FireAnimation, 1.f);
			}
		}
	}
}

void UTP_WeaponComponent::FireBlueProjectile()
{
	Fire(BlueProjectile);
}

void UTP_WeaponComponent::FireOrangeProjectile()
{
	Fire(OrangeProjectile);
}

bool UTP_WeaponComponent::PerformLineTrace()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return false;
	}

	// Get player controller
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController == nullptr)
	{
		return false;
	}

	// Get camera location and rotation
	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// Calculate end location for line trace
	FVector EndLocation = CameraLocation + (CameraRotation.Vector() * 4000.0f);

	// Perform line trace using custom trace channel 
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
		bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, CameraLocation, EndLocation, FCollisionObjectQueryParams(ObjectTypes), CollisionParams);
		
	// Check if hit something
	if (bHit)
	{

		if (HitResult.GetActor() && HitResult.GetActor()->GetRootComponent()->GetCollisionObjectType() == ECC_GameTraceChannel2)
		{
			// It's a Portal Wall
			DrawDebugLine(GetWorld(), CameraLocation, HitResult.Location, FColor::Green, false, 1.0f, 0, 1.0f);
			return true;
		}

		else 
		{
			// It's Not a Portal Wall
			DrawDebugLine(GetWorld(), CameraLocation, EndLocation, FColor::Red, false, 1.0f, 0, 1.0f);
			return false;
		}
	
	}
	else
	{
		// Didn't Hit Anything at All
		DrawDebugLine(GetWorld(), CameraLocation, EndLocation, FColor::Red, false, 1.0f, 0, 1.0f);
		return false;
	}
}

void UTP_WeaponComponent::AttachWeapon(AGEII_Project1Character* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no rifle yet
	if (Character == nullptr || Character->GetHasRifle())
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));
	
	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasRifle(true);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire Left
			EnhancedInputComponent->BindAction(LeftFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::FireBlueProjectile);

			// Fire Right
			EnhancedInputComponent->BindAction(RightFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::FireOrangeProjectile);
		}
		
	}
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}