// Fill out your copyright notice in the Description page of Project Settings.


#include "OrangeProjectile.h"

void AOrangeProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("OrangeHit"));
	WeaponThatShotMe->SpawnOrangePortal();
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
