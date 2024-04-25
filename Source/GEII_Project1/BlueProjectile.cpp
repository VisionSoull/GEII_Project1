// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueProjectile.h"

void ABlueProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("BlueHit"));
	WeaponThatShotMe->SpawnBluePortal();
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
