#include "CommandoAnimation.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

struct FCommandoAnimationProxy : FAnimInstanceProxy
{
 FAnimNode_BlendSpacePlayer_Standalone Move;
 FAnimNode_SequenceEvaluator_Standalone Action;
 FAnimNode_LayeredBoneBlend Upper;
 FAnimNode_TwoWayBlend Full;
 FCommandoAnimationProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
 virtual void Initialize(UAnimInstance* Instance) override
 {
  Move.SetLoop(true);
  Upper.BasePose.SetLinkNode(&Move);
  if(Upper.BlendPoses.IsEmpty())Upper.AddPose();
  Upper.BlendPoses[0].SetLinkNode(&Action);Upper.BlendWeights[0]=0;
  if(Upper.LayerSetup.IsEmpty())Upper.LayerSetup.AddDefaulted();
  FBranchFilter Filter;Filter.BoneName=TEXT("spine");Filter.BlendDepth=3;
  Upper.LayerSetup[0].BranchFilters={Filter};Upper.bMeshSpaceRotationBlend=true;
  Full.A.SetLinkNode(&Upper);Full.B.SetLinkNode(&Action);Full.Alpha=0;
  FAnimInstanceProxy::Initialize(Instance);
 }
 virtual FAnimNode_Base* GetCustomRootNode() override{return &Full;}
 virtual void GetCustomNodes(TArray<FAnimNode_Base*>& Nodes) override{Nodes.Append({&Move,&Action,&Upper,&Full});}
 virtual void PreUpdate(UAnimInstance* Instance,float Delta) override
 {
  FAnimInstanceProxy::PreUpdate(Instance,Delta);
  auto* A=CastChecked<UCommandoAnimation>(Instance);
  Move.SetBlendSpace(A->Locomotion);Move.SetPosition(A->Input);
  Action.SetSequence(A->CurrentAction?A->CurrentAction:A->Actions.FindRef(TEXT("Idle")));
  Action.SetExplicitTime(FMath::Min(A->ActionTime,FMath::Max(0.f,A->ActionLength-.001f)));
  Upper.BlendWeights[0]=A->FullBody?0:A->ActionWeight;
  Full.Alpha=A->FullBody?A->ActionWeight:0;
 }
};
UCommandoAnimation::UCommandoAnimation(){bUseMultiThreadedAnimationUpdate=false;RootMotionMode=ERootMotionMode::NoRootMotionExtraction;}
void UCommandoAnimation::NativeInitializeAnimation()
{
 Super::NativeInitializeAnimation();Phase=FMath::FRand();PositionReady=false;Fallen=false;CurrentAction=nullptr;ActionWeight=ActionTime=0;Input=FVector::ZeroVector;
 SetCivilian(Civilian);
 for(const TCHAR* Name:{TEXT("Idle"),TEXT("Fire"),TEXT("Reload"),TEXT("Hit"),TEXT("Dodge"),TEXT("Death")})
  Actions.Add(FName(Name),LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Production/Rigged/Motions/Motion_CommandoRiggedRig_%s.Motion_CommandoRiggedRig_%s"),Name,Name)));
 Actions.Add(TEXT("Sit"),LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Production/Rigged/Civilian/CivilianSit.CivilianSit")));
 const TCHAR* Falls[]={TEXT("FallBack07"),TEXT("FallForward07"),TEXT("FallSide07"),TEXT("FallLeft07")};
 const TCHAR* Keys[]={TEXT("Death"),TEXT("DeathForward"),TEXT("DeathRight"),TEXT("DeathLeft")};
 for(int I=0;I<4;I++){
  auto* Clip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Production/Rigged/Animation07/%s.%s"),Falls[I],Falls[I]));
  if(Clip)Actions.Add(FName(Keys[I]),Clip);else UE_LOG(LogTemp,Error,TEXT("FALL_MOTION_MISSING %s"),Falls[I]);
 }
 const FString Role=GetNameSafe(GetSkelMeshComponent()->GetSkeletalMeshAsset());
 const TCHAR* Contact=Role==TEXT("Heavy06")?TEXT("FallBackHeavy07"):Role==TEXT("Scout06")?TEXT("FallForwardScout07"):Role==TEXT("Worker06")?TEXT("FallBackWorker07"):nullptr;
 if(Contact){
  auto* Clip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Production/Rigged/Animation07/%s.%s"),Contact,Contact));
  if(Clip)Actions.Add(Role==TEXT("Scout06")?TEXT("DeathForward"):TEXT("Death"),Clip);else UE_LOG(LogTemp,Error,TEXT("FALL_MOTION_MISSING %s"),Contact);
 }
 if(!Locomotion)UE_LOG(LogTemp,Error,TEXT("RIG_LOCOMOTION_MISSING"));
}
void UCommandoAnimation::NativeUpdateAnimation(float Delta)
{
 Super::NativeUpdateAnimation(Delta);if(Delta<=0||!GetOwningActor())return;
 FVector P=GetOwningActor()->GetActorLocation();if(auto* Parent=GetOwningActor()->GetAttachParentActor())P=Parent->GetActorTransform().InverseTransformPosition(P);FVector Velocity=PositionReady?(P-PreviousPosition)/Delta:FVector::ZeroVector;PreviousPosition=P;PositionReady=true;
 if(Velocity.Size2D()>1200)Velocity=FVector::ZeroVector; // Spawn / checkpoint is not locomotion.
 USceneComponent* AimParent=GetSkelMeshComponent()->GetAttachParent();FRotator Facing=AimParent?AimParent->GetComponentRotation():GetOwningActor()->GetActorRotation();
 FVector Local=Facing.UnrotateVector(Velocity);Local.Z=0;
 Input=FMath::VInterpTo(Input,Fallen?FVector::ZeroVector:Local,Delta,11);
 if(CurrentAction){ActionTime+=Delta;float In=FMath::Clamp(ActionTime/.075f,0.f,1.f);float Out=Fallen?1.f:FMath::Clamp((ActionLength-ActionTime)/.14f,0.f,1.f);ActionWeight=In*Out;if(ActionTime>=ActionLength&&!Fallen){CurrentAction=nullptr;ActionWeight=0;}}
}
void UCommandoAnimation::Trigger(FName Name)
{
 if(Fallen)return;UAnimSequence* Clip=Actions.FindRef(Name);if(!Clip)return;
 if(Name==TEXT("Fire")&&CurrentAction==Actions.FindRef(TEXT("Reload")))return;
 const bool Death=Name.ToString().StartsWith(TEXT("Death"));
 CurrentAction=Clip;ActionTime=0;ActionLength=Clip->GetPlayLength();FullBody=Name==TEXT("Dodge")||Death||Name==TEXT("Sit");Fallen=Death||Name==TEXT("Sit");
}
FAnimInstanceProxy* UCommandoAnimation::CreateAnimInstanceProxy(){return new FCommandoAnimationProxy(this);}
void UCommandoAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}

bool UCommandoAnimationTools::BuildLocomotion()
{
#if WITH_EDITOR
 const FString PackageName=TEXT("/Game/Production/Rigged/BS_Commando");auto* Package=CreatePackage(*PackageName);
 UBlendSpace* Blend=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Production/Rigged/BS_Commando.BS_Commando"));bool New=Blend==nullptr;
 if(!Blend)Blend=NewObject<UBlendSpace>(Package,TEXT("BS_Commando"),RF_Public|RF_Standalone);
 auto* Skeleton=LoadObject<USkeleton>(nullptr,TEXT("/Game/Production/Rigged/CommandoRigged_Skeleton.CommandoRigged_Skeleton"));if(!Skeleton)return false;
 Blend->SetSkeleton(Skeleton);while(Blend->GetNumberOfBlendSamples()>0)Blend->DeleteSample(0);
 for(int Axis=0;Axis<2;Axis++){auto* Param=FindFProperty<FStructProperty>(UBlendSpace::StaticClass(),TEXT("BlendParameters"))->ContainerPtrToValuePtr<FBlendParameter>(Blend,Axis);Param->DisplayName=Axis==0?TEXT("Forward speed"):TEXT("Side speed");Param->Min=-680;Param->Max=680;Param->GridNum=8;Blend->InterpolationParam[Axis].InterpolationTime=.12f;}
 auto Add=[&](const TCHAR* Name,FVector Position,float Rate){auto* Clip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Production/Rigged/Motions/Motion_CommandoRiggedRig_%s.Motion_CommandoRiggedRig_%s"),Name,Name));if(!Clip)return false;int I=Blend->AddSample(Clip,Position);if(I<0){UE_LOG(LogTemp,Error,TEXT("BLEND_ADD_FAILED clip=%s skeleton=%s target=%s samples=%d"),Name,*GetNameSafe(Clip->GetSkeleton()),*GetNameSafe(Skeleton),Blend->GetNumberOfBlendSamples());return false;}auto* Samples=FindFProperty<FArrayProperty>(UBlendSpace::StaticClass(),TEXT("SampleData"));FScriptArrayHelper Helper(Samples,Samples->ContainerPtrToValuePtr<void>(Blend));reinterpret_cast<FBlendSample*>(Helper.GetRawPtr(I))->RateScale=Rate;return true;};
 if(!Add(TEXT("Idle"),FVector::ZeroVector,1))return false;
 for(int Ring=0;Ring<3;Ring++){
  float Speed=Ring==0?170:Ring==1?340:680;float Rate=Ring==0?1.15f:Speed/355.f;
  const TCHAR* Names[4]={Ring==0?TEXT("WalkForward"):TEXT("RunForward"),Ring==0?TEXT("WalkBack"):TEXT("RunBack"),Ring==0?TEXT("WalkLeft"):TEXT("RunLeft"),Ring==0?TEXT("WalkRight"):TEXT("RunRight")};
  FVector Positions[4]={FVector(Speed,0,0),FVector(-Speed,0,0),FVector(0,-Speed,0),FVector(0,Speed,0)};
  for(int Dir=0;Dir<4;Dir++)if(!Add(Names[Dir],Positions[Dir],Rate))return false;
 }
 Blend->TargetWeightInterpolationSpeedPerSec=8;Blend->ValidateSampleData();Blend->ResampleData();Blend->PostEditChange();Package->MarkPackageDirty();if(New)FAssetRegistryModule::AssetCreated(Blend);
 FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
 return UPackage::SavePackage(Package,Blend,*FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension()),Args);
#else
 return false;
#endif
}

void UCommandoAnimation::SetCivilian(bool Value){
 Civilian=Value;Locomotion=LoadObject<UBlendSpace>(nullptr,Civilian?TEXT("/Game/Production/Rigged/BS_Civilian.BS_Civilian"):TEXT("/Game/Production/Rigged/BS_Commando.BS_Commando"));
 CurrentAction=nullptr;ActionWeight=ActionTime=0;
}
bool UCommandoAnimationTools::BuildCivilianLocomotion(){
#if WITH_EDITOR
 const FString PackageName=TEXT("/Game/Production/Rigged/BS_Civilian");auto* Package=CreatePackage(*PackageName);
 auto* Blend=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Production/Rigged/BS_Civilian.BS_Civilian"));bool New=Blend==nullptr;if(!Blend)Blend=NewObject<UBlendSpace>(Package,TEXT("BS_Civilian"),RF_Public|RF_Standalone);
 auto* Skeleton=LoadObject<USkeleton>(nullptr,TEXT("/Game/Production/Rigged/CommandoRigged_Skeleton.CommandoRigged_Skeleton"));if(!Skeleton)return false;Blend->SetSkeleton(Skeleton);while(Blend->GetNumberOfBlendSamples())Blend->DeleteSample(0);
 for(int Axis=0;Axis<2;Axis++){auto* Param=FindFProperty<FStructProperty>(UBlendSpace::StaticClass(),TEXT("BlendParameters"))->ContainerPtrToValuePtr<FBlendParameter>(Blend,Axis);Param->Min=-680;Param->Max=680;Param->GridNum=8;Blend->InterpolationParam[Axis].InterpolationTime=.16f;}
 for(int Ring=0;Ring<4;Ring++){
  const TCHAR* Name=Ring==0?TEXT("CivilianIdle"):Ring==1?TEXT("CivilianWalk"):TEXT("CivilianRun");auto* Clip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Production/Rigged/Civilian/%s.%s"),Name,Name));if(!Clip){UE_LOG(LogTemp,Error,TEXT("CIVILIAN_CLIP_MISSING %s"),Name);return false;}
  float Speed=Ring==0?0:Ring==1?170:Ring==2?355:680;
  for(int Dir=0;Dir<(Ring==0?1:4);Dir++){FVector P=FVector(Speed,0,0).RotateAngleAxis(Dir*90,FVector::UpVector);int I=Blend->AddSample(Clip,P);if(I<0)return false;auto* Samples=FindFProperty<FArrayProperty>(UBlendSpace::StaticClass(),TEXT("SampleData"));FScriptArrayHelper Helper(Samples,Samples->ContainerPtrToValuePtr<void>(Blend));reinterpret_cast<FBlendSample*>(Helper.GetRawPtr(I))->RateScale=Ring==3?680.f/355.f:1;}
 }
 Blend->TargetWeightInterpolationSpeedPerSec=8;Blend->ValidateSampleData();Blend->ResampleData();Blend->PostEditChange();Package->MarkPackageDirty();if(New)FAssetRegistryModule::AssetCreated(Blend);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;return UPackage::SavePackage(Package,Blend,*FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension()),Args);
#else
 return false;
#endif
}
