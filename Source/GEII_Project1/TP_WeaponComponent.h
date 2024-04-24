// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Portal.h"
#include "GEII_Project1Projectile.h"
#include "TP_WeaponComponent.generated.h"


class AGEII_Project1Character;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GEII_PROJECT1_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** BlueProjectile To Spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AGEII_Project1Projectile> BlueProjectile;

	/** OrangeProjectile To spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AGEII_Project1Projectile> OrangeProjectile;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Left Click Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* LeftFireAction;

	/** Right Click Input Action **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* RightFireAction;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void AttachWeapon(AGEII_Project1Character* TargetCharacter);

	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire(TSubclassOf<class AGEII_Project1Projectile> Projectile);

	/** Make the weapon Fire Blue Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireBlueProjectile();

	/** Make the weapon Fire Orange Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireOrangeProjectile();

	/** Perform a LineTrace */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool PerformLineTrace();

	/** Returns the Object that is currently being hit **/
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bLastTraceHitPortalWall;


protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

private:
	/** The Character holding this weapon*/
	AGEII_Project1Character* Character;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Portal", meta =(AllowPrivateAccess = "true"))
	APortal* BluePortal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Portal", meta =(AllowPrivateAccess = "true"))
	APortal* OrangePortal;

	APortal* SpawnedBluePortal;
	 
	APortal* SpawnedOrangePortal;
};
