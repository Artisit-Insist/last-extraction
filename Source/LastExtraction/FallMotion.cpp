#include "Extraction.h"
#include "CommandoAnimation.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Engine/World.h"
#include "UnrealClient.h"

void AExtractionMode::StartCharacterFall(AActor* Actor,FVector ImpactDirection){
 if(!Actor)return;
 auto* Mesh=Actor->FindComponentByClass<USkeletalMeshComponent>();if(!Mesh)return;
 USceneComponent* Facing=Mesh->GetAttachParent();
 FVector Direction=ImpactDirection.GetSafeNormal2D();if(Direction.IsNearlyZero())Direction=-(Facing?Facing->GetForwardVector():Actor->GetActorForwardVector());
 FVector Local=Facing?Facing->GetComponentTransform().InverseTransformVectorNoScale(Direction):Actor->GetActorTransform().InverseTransformVectorNoScale(Direction);
 FName Action=FMath::Abs(Local.Y)>.6f?(Local.Y>0?TEXT("DeathRight"):TEXT("DeathLeft")):Local.X>0?TEXT("DeathForward"):TEXT("Death");
 if(auto* Anim=Cast<UCommandoAnimation>(Mesh->GetAnimInstance()))Anim->Trigger(Action);
 TInlineComponentArray<UStaticMeshComponent*> Parts;Actor->GetComponents(Parts);
 for(auto* Part:Parts)if(Part->ComponentTags.Contains(TEXT("CharacterWeapon"))&&Part->IsVisible()){
  FPendingWeaponDrop Drop;Drop.Weapon=Part;Drop.Direction=Direction;PendingWeaponDrops.Add(Drop);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("fall-preview")))UE_LOG(LogTemp,Display,TEXT("FALL_ACTION action=%s local=(%.2f,%.2f)"),*Action.ToString(),Local.X,Local.Y);
}

void AExtractionMode::UpdateWeaponDrops(float Delta){
 for(int I=PendingWeaponDrops.Num()-1;I>=0;I--){
  auto& Drop=PendingWeaponDrops[I];Drop.Delay-=Delta;if(Drop.Delay>0)continue;
  if(IsValid(Drop.Weapon)&&Drop.Weapon->IsVisible()){
   auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Production/Meshes/FieldRifleDrop07.FieldRifleDrop07"));
   if(Mesh){
    const FTransform Transform=Drop.Weapon->GetComponentTransform();
    auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(Transform.GetLocation(),Transform.Rotator());auto* Part=Actor->GetStaticMeshComponent();
    Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Mesh);Actor->SetActorTransform(Transform);
    for(int M=0;M<Drop.Weapon->GetNumMaterials();M++)Part->SetMaterial(M,Drop.Weapon->GetMaterial(M));
    Part->SetCollisionProfileName(TEXT("PhysicsActor"));Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Part->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Part->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);Part->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    Part->SetGenerateOverlapEvents(false);Part->BodyInstance.bUseCCD=true;Part->SetMassOverrideInKg(NAME_None,9.f,true);Part->SetLinearDamping(1.4f);Part->SetAngularDamping(1.8f);
    Part->SetSimulatePhysics(true);WeaponDropKicks.Add(Actor,Drop.Direction);
    Drop.Weapon->SetVisibility(false,false);Actor->SetLifeSpan(8);WorldActors.Add(Actor);DroppedWeapons.Add(Actor);WeaponDrops++;
    if(WeaponDrops==1||WeaponDrops%40==0||FParse::Param(FCommandLine::Get(),TEXT("fall-preview")))UE_LOG(LogTemp,Display,TEXT("WEAPON_PHYSICS_DROP count=%d"),WeaponDrops);
   }else UE_LOG(LogTemp,Error,TEXT("WEAPON_DROP_MESH_MISSING"));
  }
  PendingWeaponDrops.RemoveAtSwap(I);
 }
 DroppedWeapons.RemoveAll([](AActor* Actor){return !IsValid(Actor);});
 // Chaos can create the body after this game tick. Apply the release velocity once it exists.
 for(auto It=WeaponDropKicks.CreateIterator();It;++It){
  if(!IsValid(It.Key())){It.RemoveCurrent();continue;}
  auto* Part=Cast<AStaticMeshActor>(It.Key())->GetStaticMeshComponent();
  if(Part->IsSimulatingPhysics()){
   Part->SetPhysicsLinearVelocity(It.Value()*85+FVector(0,0,35));Part->SetPhysicsAngularVelocityInDegrees(FVector(90,130,WeaponDrops%2?90:-90));It.RemoveCurrent();
  }
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("fall-preview"))&&DroppedWeapons.Num()&&int(FallPreviewTime*30)%15==0){
  auto* Part=Cast<AStaticMeshActor>(DroppedWeapons[0])->GetStaticMeshComponent();
  UE_LOG(LogTemp,Display,TEXT("WEAPON_DROP_SAMPLE time=%.3f simulate=%d position=%s velocity=%s"),FallPreviewTime,Part->IsSimulatingPhysics()?1:0,*Part->GetComponentLocation().ToString(),*Part->GetPhysicsLinearVelocity().ToString());
 }
}

void AExtractionMode::BuildFallPreview(){
 if(!FParse::Param(FCommandLine::Get(),TEXT("fall-preview")))return;
 FallPreviewTime=0;FallStage=-1;FallFrame=0;WeaponDrops=0;
 Hero->SetActorLocation(Checkpoint);Hero->SetActorHiddenInGame(true);Hero->Invincible=100;
 for(auto* Actor:WorldActors)if(IsValid(Actor)&&FVector::Dist2D(Actor->GetActorLocation(),Checkpoint)<2400){Actor->SetActorHiddenInGame(true);Actor->SetActorEnableCollision(false);Canopies.Remove(Actor);}
 for(auto& Enemy:Enemies)if(IsValid(Enemy.Actor)){Enemy.Actor->SetActorHiddenInGame(true);Enemy.Actor->SetActorEnableCollision(false);}
 int Case=0;FParse::Value(FCommandLine::Get(),TEXT("FallCase="),Case);Case=FMath::Clamp(Case,0,6);
 Followers.Reset();AActor* Person=nullptr;
 if(Case==4){Person=Hero;Hero->SetActorHiddenInGame(false);Hero->Visual->SetRelativeRotation(FRotator::ZeroRotator);}
 else{Person=Shape(Checkpoint,FVector(1),FLinearColor::White,1,false);Cast<AStaticMeshActor>(Person)->GetStaticMeshComponent()->SetVisibility(false,false);if(Case==6)Person->Tags.Add(TEXT("WorkerWardrobe"));MakeSoldier(Person,false,Case==5?2:Case<3?Case:-2);}
 Followers.Add(Person);Hero->Camera->SetAbsolute(true,true,false);FVector Offset(620,-470,410),Target=Checkpoint+FVector(-15,0,-15);Hero->Camera->SetWorldLocation(Target+Offset);Hero->Camera->SetWorldRotation((-Offset).Rotation());Hero->Camera->FieldOfView=38;
 IFileManager::Get().MakeDirectory(*(FPaths::ProjectDir()/TEXT("../../work/animation_07/frames")),true);
}

bool AExtractionMode::UpdateFallPreview(float Delta){
 if(!FParse::Param(FCommandLine::Get(),TEXT("fall-preview")))return false;
 FallPreviewTime+=Delta;int Case=0;FParse::Value(FCommandLine::Get(),TEXT("FallCase="),Case);Case=FMath::Clamp(Case,0,6);
 if(FallPreviewTime>=.8f&&FallStage<0){
  const FVector Directions[]={FVector(-1,0,0),FVector(1,0,0),FVector(0,1,0),FVector(0,-1,0),FVector(-1,0,0),FVector(-1,0,0),FVector(-1,0,0)};
  StartCharacterFall(Followers[0],Directions[Case]);FallStage=0;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("record-fall"))&&FallPreviewTime<6){
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/animation_07/frames/case%d_%05d.png"),Case,FallFrame++),false,false);
 }else if(FallPreviewTime>4&&FallPreviewTime<4+Delta*1.5f)FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/animation_07/fall-case-%d.png"),Case),false,false);
 if(FallPreviewTime>=6&&FallStage==0){
  auto* Part=DroppedWeapons.Num()?Cast<AStaticMeshActor>(DroppedWeapons[0])->GetStaticMeshComponent():nullptr;
  float Speed=Part?Part->GetPhysicsLinearVelocity().Size():-1;float Bottom=Part?Part->Bounds.Origin.Z-Part->Bounds.BoxExtent.Z-GroundHeight(Part->Bounds.Origin):-999;
  float HullClearance=MAX_flt;
  if(Part&&Part->GetBodySetup())for(const auto& Hull:Part->GetBodySetup()->AggGeom.ConvexElems)for(const auto& V:Hull.VertexData){
   FVector World=Part->GetComponentTransform().TransformPosition(Hull.GetTransform().TransformPosition(V));HullClearance=FMath::Min(HullClearance,float(World.Z-GroundHeight(World)));
  }
  bool OriginalVisible=false;TInlineComponentArray<UStaticMeshComponent*> Parts;Followers[0]->GetComponents(Parts);for(auto* P:Parts)if(P->ComponentTags.Contains(TEXT("CharacterWeapon")))OriginalVisible|=P->IsVisible();
  UE_LOG(LogTemp,Display,TEXT("FALL_PREVIEW_RESULT case=%d drops=%d simulate=%d speed=%.3f bottom=%.3f hull_clearance=%.3f original_visible=%d frames=%d"),Case,WeaponDrops,Part&&Part->IsSimulatingPhysics()?1:0,Speed,Bottom,HullClearance,OriginalVisible?1:0,FallFrame);
  FallStage=1;
 }
 if(FallPreviewTime>=6.5f)GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));return true;
}
