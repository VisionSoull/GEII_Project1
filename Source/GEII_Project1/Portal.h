// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Variable for the portal material 
	UPROPERTY(EditAnywhere, Category = "Portal")
	UMaterial* PortalMaterial;

	// Variable for linked portal
	UPROPERTY(EditAnywhere, BlueprintReadONly, Category="Portal")
	APortal* LinkedPortal;

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

protected:
	// Function to update the Linked Portal's Camera
	void UpdateSceneCapture();

	USceneCaptureComponent2D* LinkedPortalCamera;

protected:
	// Variable to hold the tick group
	UPROPERTY(EditAnywhere, Category = "Tick")
	TEnumAsByte<ETickingGroup> TickGroup;

};
