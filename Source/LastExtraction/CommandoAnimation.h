#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CommandoAnimation.generated.h"
class UBlendSpace;
class UAnimSequence;

UCLASS()
class UCommandoAnimation : public UAnimInstance
{
 GENERATED_BODY()
public:
 UCommandoAnimation();
 virtual void NativeInitializeAnimation() override;
 virtual void NativeUpdateAnimation(float DeltaSeconds) override;
 virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
 virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
 void Trigger(FName Action);
 void SetCivilian(bool Value);
 bool Civilian=false;
 UPROPERTY() UBlendSpace* Locomotion=nullptr;
 UPROPERTY() TMap<FName,UAnimSequence*> Actions;
 UPROPERTY() UAnimSequence* CurrentAction=nullptr;
 FVector Input=FVector::ZeroVector,PreviousPosition=FVector::ZeroVector;
 float ActionTime=0,ActionLength=0,ActionWeight=0,Phase=0;
 bool FullBody=false,Fallen=false,PositionReady=false;
};

UCLASS()
class UCommandoAnimationTools : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Production") static bool BuildLocomotion();
 UFUNCTION(BlueprintCallable,Category="Production") static bool BuildCivilianLocomotion();
};
