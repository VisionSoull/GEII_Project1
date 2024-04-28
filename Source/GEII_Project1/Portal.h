// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GEII_Project1Character.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Portal.generated.h"

UCLASS()
class GEII_PROJECT1_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortal();

private:
	// Default scene root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneRoot;

	// Static mesh component for the portal
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* PortalMesh;

	// Scene capture component 2D of the portal
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	USceneCaptureComponent2D* PortalCamera;

	// Back facing scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	USceneComponent* BackFacingScene;

	// Collision to detect player to pass through the portal
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Variable for the portal material 
	UPROPERTY(EditAnywhere, Category = "Portal")
	UMaterial* PortalMaterial;

	// Variable for linked portal
	UPROPERTY(EditAnywhere, BlueprintReadONly, Category="Portal")
	APortal* LinkedPortal;

	// Variable to set the portal color
	UPROPERTY(EditAnywhere, BlueprintReadONly, Category="Portal")
	FLinearColor PortalColor;

	// Variable for referencing the Portal Mesh Material
	UPROPERTY(BlueprintReadOnly, Category = "Portal")
	UMaterialInstanceDynamic* Portal_MAT;

	// Variable for Capture Render Target
	UPROPERTY(BlueprintReadOnly, Category = "Portal")
	UTextureRenderTarget2D* Portal_RT;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Function to give a reference of the Portal Camera to the Linked Portal requesting it
	USceneCaptureComponent2D* GetSceneCapture() const;

	// Function to receive reference of a portal to link
	void SetPortalToLink(APortal* PortalToLink);

	void PlacePortal(FVector NewLocation, FRotator NewRotation);

	APortal* GetLinkedPortal();

public:

	void SetupLinkedPortal();

	void SetCurrentWall(AActor* NewWall);

private:
	// Function to update the Linked Portal's Camera
	void UpdateSceneCapture();

	// Begin Overlap
    UFUNCTION()
	void BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// End Overlap
	UFUNCTION()
	void EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void CheckPlayerCanTeleport(AGEII_Project1Character* Player);

	UFUNCTION()
	void TeleportPlayer(AGEII_Project1Character* Player);

	USceneCaptureComponent2D* LinkedPortalCamera;

private:
	AGEII_Project1Character* PlayerInPortal;

protected:

	FTimerHandle TimerHandle;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	AActor* CurrentWall;

	void CheckPortalBounds();

	bool CheckPointToMovePortal(UWorld* World, FVector Point);
};
