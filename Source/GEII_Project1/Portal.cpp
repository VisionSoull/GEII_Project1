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

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set default tick group
	TickGroup = TG_PostUpdateWork;

	// Create default scene root
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefautlSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Create the Portal Static Mesh component
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);

	// Create the Scene Capture 2D component
	PortalCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalCamera"));
	PortalCamera->SetupAttachment(RootComponent);

	// Create the "BackFacingScene" Scene Component
	BackFacingScene = CreateDefaultSubobject<USceneComponent>(TEXT("BackFacingScene"));
	BackFacingScene->SetupAttachment(RootComponent);

	// Set 180º rotation on Z-axis for the "BackFacingScene" component
	BackFacingScene->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));

	PortalMesh->SetRelativeRotation(FRotator(0.f, -90.f, 90.f));
	PortalMesh->SetRelativeScale3D(FVector(2.3f, 2.3f, 2.3f));
}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();

	// Create the dynamic material instance
	Portal_MAT = UMaterialInstanceDynamic::Create(PortalMaterial, this);

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

	if (LinkedPortal)
	{
		LinkedPortalCamera = LinkedPortal->GetSceneCapture();
		if (LinkedPortalCamera)
		{
			// Set the texture target for the linked portal's camera
			LinkedPortalCamera->TextureTarget = Portal_RT;

			// Enable custom near clipping plane.
			LinkedPortalCamera->CustomNearClippingPlane = true;
		}
	}
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set the tick group
	SetTickGroup(TickGroup);

	UpdateSceneCapture();

}

USceneCaptureComponent2D* APortal::GetSceneCapture() const
{
	return PortalCamera;
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

	// Calculate the distance between the player camera manager and the portal
	float Distance = FVector::Distance(PlayerCameraManager->GetCameraLocation(), GetActorLocation());

	// Add 1 to the distance for better result
	Distance += 1.f;

	// Set the custom near clipping plane value for the linked portal camera
	LinkedPortalCamera->CustomNearClippingPlane = Distance;
}

