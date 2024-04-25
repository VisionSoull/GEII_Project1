// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GEII_Project1Projectile.h"
#include "BlueProjectile.generated.h"

/**
 * 
 */
UCLASS()
class GEII_PROJECT1_API ABlueProjectile : public AGEII_Project1Projectile
{
	GENERATED_BODY()
	
public:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
