// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "GEII_Project1Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Portal.h"
#include "GEII_Project1Projectile.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

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

	bLastTraceHitPortalWall = false;



}

void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Perform line trace on tick
	PerformLineTrace();
}


void UTP_WeaponComponent::Fire(TSubclassOf<class AGEII_Project1Projectile> Projectile)
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Check if the last trace hit a portal wall
	if (!bLastTraceHitPortalWall)
	{
		return; // Don't fire if the last trace didn't hit a portal wall
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
			AGEII_Project1Projectile* ShotProjectile = World->SpawnActor<AGEII_Project1Projectile>(Projectile, SpawnLocation, SpawnRotation, ActorSpawnParams);
			ShotProjectile->SetWeaponThatShot(this);
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
	if (Character == nullptr)
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

	// Update the member variable based on the result
	bLastTraceHitPortalWall = bHit && HitResult.GetActor() && HitResult.GetActor()->GetRootComponent()->GetCollisionObjectType() == ECC_PortalTraceChannel;
	
	LastTraceHit = HitResult;
	return bLastTraceHitPortalWall;
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

void UTP_WeaponComponent::SpawnPortal(TSubclassOf<class APortal> PortalToSpawn, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("Call Place Portal"));
	UWorld* const World = GetWorld();
	if (World)
	{
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		FVector LocationToSpawn = LastTraceHit.Location + LastTraceHit.Normal * 2;
		if (PortalToSpawn == BluePortal)
		{
			SpawnedBluePortal = World->SpawnActor<APortal>(PortalToSpawn, LocationToSpawn, UKismetMathLibrary::MakeRotFromX(Hit.Normal), ActorSpawnParams);
			if (SpawnedOrangePortal->IsValidLowLevel())
			{
				SpawnedBluePortal->SetPortalToLink(SpawnedOrangePortal);
				SpawnedOrangePortal->SetPortalToLink(SpawnedBluePortal);
				SpawnedBluePortal->SetupLinkedPortal();
				SpawnedOrangePortal->SetupLinkedPortal();
				SpawnedBluePortal->EnableTicking();
				SpawnedOrangePortal->EnableTicking();
			}
		}
		else if (PortalToSpawn == OrangePortal)
		{
			SpawnedOrangePortal = World->SpawnActor<APortal>(PortalToSpawn, LocationToSpawn, UKismetMathLibrary::MakeRotFromX(Hit.Normal), ActorSpawnParams);
			if (SpawnedBluePortal->IsValidLowLevel())
			{
				
				SpawnedBluePortal->SetPortalToLink(SpawnedOrangePortal);
				SpawnedOrangePortal->SetupLinkedPortal();
				SpawnedBluePortal->SetupLinkedPortal();
				SpawnedBluePortal->EnableTicking();
				SpawnedOrangePortal->EnableTicking();
			}
		}

	}
}

void UTP_WeaponComponent::ChangePortalLocation(APortal* PortalToChangeLocation, FVector NewLocation, FRotator NewRotation)
{
	PortalToChangeLocation->PlacePortal(NewLocation, NewRotation);
}

void UTP_WeaponComponent::SpawnBluePortal()
{
	if (SpawnedBluePortal == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Try Spawn Blue Portal"));
		SpawnPortal(BluePortal, LastTraceHit);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Try Change Location Blue Portal"));
		ChangePortalLocation(SpawnedBluePortal, LastTraceHit.Location, UKismetMathLibrary::MakeRotFromX(LastTraceHit.Normal));
	}
}

void UTP_WeaponComponent::SpawnOrangePortal()
{
	if (SpawnedOrangePortal == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Try Spawn Orange Portal"));
		SpawnPortal(OrangePortal, LastTraceHit);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Try Change Location Orange Portal"));
		ChangePortalLocation(SpawnedOrangePortal, LastTraceHit.Location, UKismetMathLibrary::MakeRotFromX(LastTraceHit.Normal));
	}
}
