// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include <Kismet/KismetMathLibrary.h>
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GEII_Project1Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"


// Define custom trace channels
#define ECC_PortalTraceChannel ECC_GameTraceChannel2

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ObjectTypes.Empty();
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PortalTraceChannel));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));

	// Create default scene root
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefautlSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Create the Portal Static Mesh component
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);

	// Create the Scene Capture 2D component
	PortalCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalCamera"));
	PortalCamera->SetupAttachment(RootComponent);

	// Enable custom near clipping plane.
	PortalCamera->bOverride_CustomNearClippingPlane = true;

	// Create the "BackFacingScene" Scene Component
	BackFacingScene = CreateDefaultSubobject<USceneComponent>(TEXT("BackFacingScene"));
	BackFacingScene->SetupAttachment(RootComponent);

	// Set 180º rotation on Z-axis for the "BackFacingScene" component
	BackFacingScene->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));

	// Set rotation and scale of the mesh
	PortalMesh->SetRelativeRotation(FRotator(0.f, -90.f, 90.f));
	PortalMesh->SetRelativeScale3D(FVector(1.5f, 2.3f, 2.3f));

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetupAttachment(RootComponent);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();

	CheckPortalBounds();

	// Get the size of the portal mesh
	FBoxSphereBounds Bounds = PortalMesh->GetStaticMesh()->GetBounds();
	FVector BoxExtent = Bounds.BoxExtent;

	// Set the size of the collision box
	CollisionComponent->SetBoxExtent(BoxExtent);
	CollisionComponent->UpdateBounds();

	// Bind overlap functions to the portal
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &APortal::BeginOverlap);
	CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &APortal::EndOverlap);

	// Create the dynamic material instance
	Portal_MAT = UMaterialInstanceDynamic::Create(PortalMaterial, this);
	Portal_MAT->SetVectorParameterValue("PortalColor", PortalColor);

	// Assign the dynamic material instance to Portal_MAT
	PortalMesh->SetMaterial(0, Portal_MAT);

	// Get the game's viewport
	UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();

	if (ViewportClient)
	{
		// Get the viewport size
		FVector2D ViewportSize;
		ViewportClient->GetViewportSize(ViewportSize);

		// Create the Render Target 2D
		Portal_RT = NewObject<UTextureRenderTarget2D>(this);
		Portal_RT->InitAutoFormat(ViewportSize.X, ViewportSize.Y);

		// Set the Render Target 2D to the texture parameter value of the Portal_MAT
		Portal_MAT->SetTextureParameterValue("Texture", Portal_RT);
	}

	SetupLinkedPortal();
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (LinkedPortal && LinkedPortal->IsValidLowLevel())
	{
		UpdateSceneCapture();
		if (PlayerInPortal)
		{
			CheckPlayerCanTeleport(PlayerInPortal);
		}
	}
}

USceneCaptureComponent2D* APortal::GetSceneCapture() const
{
	return PortalCamera;
}

void APortal::SetPortalToLink(APortal* PortalToLink)
{
	if (PortalToLink && PortalToLink->IsValidLowLevel())
	{
		LinkedPortal = PortalToLink;
	}
}

void APortal::PlacePortal(FVector NewLocation, FRotator NewRotation)
{
	FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(5.f);
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.AddIgnoredActor(this);
	TArray<FOverlapResult> Overlaps;

	bool bHit = GetWorld()->OverlapMultiByObjectType(Overlaps, NewLocation, FQuat::Identity, ECC_WorldStatic, CollisionSphere, QueryParams);

	if (bHit)
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			if (Cast<APortal>(Result.GetActor()))
			{
				return;
			}
		}
	}
	SetActorLocationAndRotation(NewLocation, NewRotation);
	CheckPortalBounds();
}

APortal* APortal::GetLinkedPortal()
{
	return LinkedPortal;
}

void APortal::SetupLinkedPortal()
{
	if (LinkedPortal)
	{
		LinkedPortalCamera = LinkedPortal->GetSceneCapture();
		if (LinkedPortalCamera)
		{
			// Set the texture target for the linked portal's camera
			LinkedPortalCamera->TextureTarget = Portal_RT;
		}
	}
}

void APortal::UpdateSceneCapture()
{
	// Get the player camera manager
	APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);

	// Get the world transform of the player's camera
	FTransform PlayerCameraTransform = PlayerCameraManager->GetTransform();

	// Get the world transform of the back facing scene
	FTransform BackFacingSceneTransform = BackFacingScene->GetComponentTransform();

	// Calculate the relative transform between the player camera and the back facing scene
	FTransform RelativeTransform = UKismetMathLibrary::MakeRelativeTransform(PlayerCameraTransform, BackFacingSceneTransform);

	// Set the relative transform to the linked portal's camera
	LinkedPortalCamera->SetRelativeLocationAndRotation(RelativeTransform.GetLocation(), RelativeTransform.GetRotation());

	// Calculate the distance between the p	layer camera manager and the portal
	float Distance = FVector::Distance(PlayerCameraManager->GetCameraLocation(), GetActorLocation());

	// Add 1 to the distance for better result
	Distance += 1.f;

	// Set the custom near clipping plane value for the linked portal camera
	LinkedPortalCamera->CustomNearClippingPlane = Distance;
}

void APortal::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGEII_Project1Character* Character = Cast<AGEII_Project1Character>(OtherActor);
	if (LinkedPortal && LinkedPortal->GetLinkedPortal())
	{
		if (Character)
		{
			PlayerInPortal = Character;

			if (UCapsuleComponent* CapsuleComponent = PlayerInPortal->GetCapsuleComponent())
			{
				CapsuleComponent->SetCollisionProfileName(TEXT("PortalPawn"));
				CapsuleComponent->UpdateCollisionProfile();
			}
		}
	}
}

void APortal::EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGEII_Project1Character* Character = Cast<AGEII_Project1Character>(OtherActor);
	if (Character)
	{
		if (PlayerInPortal)
		{
			UCapsuleComponent* CapsuleComponent = PlayerInPortal->GetCapsuleComponent();
			CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
			CapsuleComponent->UpdateCollisionProfile();
		}
		PlayerInPortal = nullptr;
	}
}

void APortal::CheckPlayerCanTeleport(AGEII_Project1Character* Player)
{
	// Get the player's current location
	FVector PlayerLocation = Player->GetActorLocation();

	// get the player's current velocity
	FVector PlayerVelocity = Player->GetVelocity();

	// Get the world's delta seconds
	float DeltaTime = GetWorld()->GetDeltaSeconds();

	// Determine the future position of the player
	FVector FuturePlayerLocation = PlayerLocation + (PlayerVelocity * DeltaTime);

	// Determine the direction from the portal to the player's future location
	FVector DirectionToFuturePlayer = (FuturePlayerLocation - this->GetActorLocation());

	DirectionToFuturePlayer = DirectionToFuturePlayer.GetSafeNormal();

	float DotProductForPlayerBehindPortal = FVector::DotProduct(DirectionToFuturePlayer, GetActorForwardVector());

	float DotProductForPlayerGoingAgainstPortal = FVector::DotProduct(Player->GetLastMovementInputVector().GetSafeNormal(), GetActorForwardVector());

	// Check if player is behind the portal and going against the portal
	if (DotProductForPlayerBehindPortal <= 0.f && DotProductForPlayerGoingAgainstPortal < 0.f)
	{
		TeleportPlayer(Player);
	}
}

void APortal::TeleportPlayer(AGEII_Project1Character* Player)
{
	// Get the current transform of the player
	FTransform PlayerTransform = Player->GetActorTransform();

	// Get the player's current velocity
	FVector PlayerVelocity = Player->GetVelocity();

	// Convert the player's velocity from world to relative
	FVector RelativeVelocity = UKismetMathLibrary::InverseTransformDirection(PlayerTransform, PlayerVelocity);

	// Get the transform of the back facing scene component
	FTransform BackFacingSceneTransform = BackFacingScene->GetComponentTransform();

	// get the transform of the player's camera component
	FTransform PlayerCameraTransform = Player->GetFirstPersonCameraComponent()->GetComponentTransform();

	// Calculate the relative transform of the player's camera in consideration to the back facing scene component
	FTransform ConvertedTransform = UKismetMathLibrary::MakeRelativeTransform(PlayerCameraTransform, BackFacingSceneTransform);

	// Calculate the player's relative transform with the linked portal's world transform
	FTransform ComposedTransform = UKismetMathLibrary::ComposeTransforms(ConvertedTransform, LinkedPortal->GetActorTransform());

	// Calculate the new location for the player, offset slightly in front of the linked portal by its forward vector for a more smooth transition
	FVector NewLocation = (LinkedPortal->GetActorForwardVector() * 10) + (ComposedTransform.GetLocation() - Player->GetFirstPersonCameraComponent()->GetRelativeLocation());

	// Calculate the new rotation for the player, ensuring that the roll is set to 0 to avoid unwanted tilting
	FRotator NewRotation = FRotator(ComposedTransform.Rotator().Pitch, ComposedTransform.Rotator().Yaw, 0.f);

	// Set the player's new location
	Player->SetActorLocation(NewLocation);

	// Update the player controller's rotation to match 
	Player->GetController()->SetControlRotation(NewRotation);
	
	// Create a transform with the updated location and control rotation.
	FTransform NewTransform = UKismetMathLibrary::MakeTransform(Player->GetActorLocation(), Player->GetController()->GetControlRotation());

	// Update the player's velocity with the new transform
	Player->GetMovementComponent()->Velocity = UKismetMathLibrary::TransformDirection(NewTransform, RelativeVelocity);
}


void APortal::CheckPortalBounds()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FBoxSphereBounds Bounds = PortalMesh->CalcBounds(PortalMesh->GetComponentTransform());
	FVector BoxCenter = Bounds.Origin;
	FVector BoxExtent = Bounds.BoxExtent;

	FVector TopLimit = BoxCenter + GetActorUpVector() * BoxExtent.Z;
	FVector BottomLimit = BoxCenter - GetActorUpVector() * BoxExtent.Z;
	FVector RightLimit = BoxCenter + GetActorRightVector() * BoxExtent.X;
	FVector LeftLimit = BoxCenter - GetActorRightVector() * BoxExtent.X;

	DrawDebugSphere(World, TopLimit, 5.f, 16, FColor::Magenta, false, 1.f, 0, 1.f);
	DrawDebugSphere(World, BottomLimit, 5.f, 16, FColor::Magenta, false, 1.f, 0, 1.f);
	DrawDebugSphere(World, RightLimit, 5.f, 16, FColor::Magenta, false, 1.f, 0, 1.f);
	DrawDebugSphere(World, LeftLimit, 5.f, 16, FColor::Magenta, false, 1.f, 0, 1.f);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<FHitResult> HitResults;

	FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(5.f);
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.AddIgnoredActor(this);

	// Top Limit Check

	bool bTopHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		World,
		BoxCenter,
		TopLimit,
		5.f,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		0.5f
	);

	bool bAnyTopObjectHit = false;

	if (bTopHit)
	{
		for (const FHitResult& Result : HitResults)
		{
			if (Result.GetActor() != CurrentWall || Result.GetActor()->IsA(APortal::StaticClass()))
			{
				DrawDebugSphere(World, Result.Location, 10.f, 16, FColor::Yellow, false, 0.5f, 0, 1.f);
				// Found an object that is not the current wall
				float DistanceToLimit = (TopLimit - Result.Location).Size();
				FVector NewLocation = GetActorLocation() - GetActorUpVector() * DistanceToLimit;
				SetActorLocationAndRotation(NewLocation, GetActorRotation());
				bAnyTopObjectHit = true;
				break;
			}
		}
	} 
	if(!bAnyTopObjectHit)
	{
		// Verify if the Top is Overlapping with anything
		bool bIsOverlapping = World->OverlapBlockingTestByChannel(
			TopLimit,
			FQuat::Identity,
			ECC_PortalTraceChannel,
			CollisionSphere,
			QueryParams
		);

		if (!bIsOverlapping)
		{
			// Sphere Trace from limit to the center of the Portal
			FHitResult HitResult;
			bool bOutsideHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
				World,
				TopLimit,
				BoxCenter,
				5.f,
				ObjectTypes,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::ForDuration,
				HitResult,
				true,
				FLinearColor::Yellow,
				FLinearColor::Blue,
				10.f
			);

			if (bOutsideHit && HitResult.GetActor() == CurrentWall)
			{
				if (HitResult.GetActor() == CurrentWall)
				{
					// The portal wall was hit, adjust the portal position
					float DistanceToWall = (HitResult.Location - TopLimit).Size();
					// Move the portal the distance to the wall
					FVector NewPosition = GetActorLocation() - GetActorUpVector() * DistanceToWall;
					SetActorLocationAndRotation(NewPosition, GetActorRotation());
				}
			}
		}
	}

	// Bottom Limit Check

	bool bBottomHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		World,
		BoxCenter,
		BottomLimit,
		5.f,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		0.5f
	);

	bool bAnyBottomObjectHit = false;

	if (bBottomHit)
	{
		for (const FHitResult& Result : HitResults)
		{
			if (Result.GetActor() != CurrentWall || Result.GetActor()->IsA(APortal::StaticClass()))
			{
				DrawDebugSphere(World, Result.Location, 10.f, 16, FColor::Yellow, false, 0.5f, 0, 1.f);
				// Found an object that is not the current wall
				float DistanceToLimit = (BottomLimit - Result.Location).Size();
				FVector NewLocation = GetActorLocation() + GetActorUpVector() * DistanceToLimit;
				SetActorLocationAndRotation(NewLocation, GetActorRotation());
				bAnyBottomObjectHit = true;
				break;
			}
		}
	}
	if (!bAnyBottomObjectHit)
	{
			// Verify if the Top is Overlapping with anything
			bool bIsOverlapping = World->OverlapBlockingTestByChannel(
				BottomLimit,
				FQuat::Identity,
				ECC_PortalTraceChannel,
				CollisionSphere,
				QueryParams
			);

		if (!bIsOverlapping)
		{

				// Sphere Trace from limit to the center of the Portal
				FHitResult HitResult;
			bool bOutsideHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
				World,
				BottomLimit,
				BoxCenter,
				5.f,
				ObjectTypes,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::ForDuration,
				HitResult,
				true,
				FLinearColor::Yellow,
				FLinearColor::Blue,
				10.f
			);

			if (bOutsideHit && HitResult.GetActor() == CurrentWall)
			{
					if (HitResult.GetActor() == CurrentWall)
					{
						// The portal wall was hit, adjust the portal position
						float DistanceToWall = (HitResult.Location - BottomLimit).Size();
						// Move the portal the distance to the wall
						FVector NewPosition = GetActorLocation() + GetActorUpVector() * DistanceToWall;
						SetActorLocationAndRotation(NewPosition, GetActorRotation());
					}
			}
		}
	}

	// Right Limit Check

	bool bRightHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		World,
		BoxCenter,
		RightLimit,
		5.f,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		0.5f
	);

	bool bAnyRightObjectHit = false;

	if (bRightHit)
	{
		for (const FHitResult& Result : HitResults)
		{
			if (Result.GetActor() != CurrentWall || Result.GetActor()->IsA(APortal::StaticClass()))
			{
				DrawDebugSphere(World, Result.Location, 10.f, 16, FColor::Yellow, false, 0.5f, 0, 1.f);
				// Found an object that is not the current wall
				float DistanceToLimit = (RightLimit - Result.Location).Size();
				FVector NewLocation = GetActorLocation() - GetActorRightVector() * DistanceToLimit;
				SetActorLocationAndRotation(NewLocation, GetActorRotation());
				bAnyRightObjectHit = true;
				break;
			}
		}
	}
	if (!bAnyRightObjectHit)
	{
			// Verify if the Top is Overlapping with anything
			bool bIsOverlapping = World->OverlapBlockingTestByChannel(
				RightLimit,
				FQuat::Identity,
				ECC_PortalTraceChannel,
				CollisionSphere,
				QueryParams
			);

		if (!bIsOverlapping)
		{
			// Sphere Trace from limit to the center of the Portal
			FHitResult HitResult;
			bool bOutsideHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
				World,
				RightLimit,
				BoxCenter,
				5.f,
				ObjectTypes,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::ForDuration,
				HitResult,
				true,
				FLinearColor::Yellow,
				FLinearColor::Blue,
				10.f
			);

			if (bOutsideHit && HitResult.GetActor() == CurrentWall)
			{
					if (HitResult.GetActor() == CurrentWall)
					{
						// The portal wall was hit, adjust the portal position
						float DistanceToWall = (HitResult.Location - RightLimit).Size();
						// Move the portal the distance to the wall
						FVector NewPosition = GetActorLocation() - GetActorRightVector() * DistanceToWall;
						SetActorLocationAndRotation(NewPosition, GetActorRotation());
					}
			}
		}
	}

	// Left Limit Check

	bool bLeftHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		World,
		BoxCenter,
		LeftLimit,
		5.f,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		0.5f
	);

	bool bAnyLeftObjectHit = false;

	if (bLeftHit)
	{
		for (const FHitResult& Result : HitResults)
		{
			if (Result.GetActor() != CurrentWall || Result.GetActor()->IsA(APortal::StaticClass()))
			{
				DrawDebugSphere(World, Result.Location, 10.f, 16, FColor::Yellow, false, 0.5f, 0, 1.f);
				// Found an object that is not the current wall
				float DistanceToLimit = (LeftLimit - Result.Location).Size();
				FVector NewLocation = GetActorLocation() + GetActorRightVector() * DistanceToLimit;
				SetActorLocationAndRotation(NewLocation, GetActorRotation());
				bAnyLeftObjectHit = true;
				break;
			}
		}
	}
	if (!bAnyLeftObjectHit)
	{
		// Verify if the Top is Overlapping with anything
			bool bIsOverlapping = World->OverlapBlockingTestByChannel(
				LeftLimit,
				FQuat::Identity,
				ECC_PortalTraceChannel,
				CollisionSphere,
				QueryParams
			);

		if (!bIsOverlapping)
		{
				// Sphere Trace from limit to the center of the Portal
				FHitResult HitResult;
			bool bOutsideHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
				World,
				LeftLimit,
				BoxCenter,
				5.f,
				ObjectTypes,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::ForDuration,
				HitResult,
				true,
				FLinearColor::Yellow,
				FLinearColor::Blue,
				10.f
			);

			if (bOutsideHit && HitResult.GetActor() == CurrentWall)
			{
					if (HitResult.GetActor() == CurrentWall)
					{
							// The portal wall was hit, adjust the portal position
							float DistanceToWall = (HitResult.Location - LeftLimit).Size();
						// Move the portal the distance to the wall
						FVector NewPosition = GetActorLocation() + GetActorRightVector() * DistanceToWall;
						SetActorLocationAndRotation(NewPosition, GetActorRotation());
					}
			}
		}
	}
}

void APortal::SetCurrentWall(AActor* NewWall)
{
	CurrentWall = NewWall;
}
