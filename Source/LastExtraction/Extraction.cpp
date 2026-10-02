#include "Extraction.h"
#include "CommandoAnimation.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Fonts/CompositeFont.h"
#include "Engine/Texture2D.h"
#include "UnrealClient.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundWave.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"

static const FLinearColor Olive(.12f,.19f,.075f), Skin(.43f,.26f,.15f), Dark(.025f,.034f,.03f), Amber(1,.5f,.12f), Teal(.07f,.85f,.67f);
static void CharacterAction(AActor* Actor,FName Name){if(!Actor)return;if(auto* Mesh=Actor->FindComponentByClass<USkeletalMeshComponent>())if(auto* Anim=Cast<UCommandoAnimation>(Mesh->GetAnimInstance()))Anim->Trigger(Name);}
static AExtractionMode* GM(UWorld* W){return Cast<AExtractionMode>(W->GetAuthGameMode());}
AExtractionPawn::AExtractionPawn(){
 PrimaryActorTick.bCanEverTick=false;
 Body=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision")); RootComponent=Body; Body->InitCapsuleSize(36,90); Body->SetCollisionProfileName(TEXT("Pawn"));
 Visual=CreateDefaultSubobject<USceneComponent>(TEXT("Soldier")); Visual->SetupAttachment(RootComponent);
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); Camera->SetupAttachment(RootComponent); Camera->SetRelativeLocation(FVector(-1450,0,1550)); Camera->SetRelativeRotation(FRotator(-47,0,0)); Camera->FieldOfView=57;
 AutoPossessPlayer=EAutoReceiveInput::Player0;
}
void AExtractionPawn::BeginPlay(){Super::BeginPlay();}
void AExtractionPawn::Animate(float T,float Speed){
 if(CharacterMesh){if(auto* Mode=GM(GetWorld()))Mode->AnimateCharacter(CharacterMesh,Speed);return;}
 if(Parts.Num()<2)return;
 float V=FMath::Sin(T*13)*FMath::Clamp(Speed/440.f,0.f,1.f)*24;
 Parts[0]->SetRelativeRotation(FRotator(V,0,0)); Parts[1]->SetRelativeRotation(FRotator(-V,0,0));
}
AExtractionMode::AExtractionMode(){
 PrimaryActorTick.bCanEverTick=true; DefaultPawnClass=AExtractionPawn::StaticClass(); HUDClass=AExtractionHUD::StaticClass();
 ChapterNames={TEXT("01  /  녹색 장막"),TEXT("02  /  강을 건너"),TEXT("03  /  잊힌 마을"),TEXT("04  /  검은 채석장"),TEXT("05  /  산 위의 요새"),TEXT("06  /  마지막 탈출")};
 Briefs={TEXT("정글에 침투해 정찰대를 구출하고 첫 번째 통신망을 끊으십시오."),TEXT("기관사를 구출하고 전력을 차단한 뒤 교량을 가동하십시오. 강 건너 포로를 구출하십시오."),TEXT("점령된 마을에서 생존자를 찾고 적의 작전 문서를 확보하십시오."),TEXT("채석장의 발전기를 파괴하고 노동 수용소에서 포로를 구출하십시오."),TEXT("요새의 방공망과 지휘 통신을 차례로 제거하십시오."),TEXT("남은 포로를 구출하고 무전기로 헬기를 유도해 모두 함께 탈출하십시오.")};
}
UMaterialInstanceDynamic* AExtractionMode::Material(FLinearColor C,bool Glow,float R,float M){
 // Reuse exact material colors rather than creating a material for every leaf.
 FString Key=FString::Printf(TEXT("%s_%d_%d_%d_%d_%d"),Glow?TEXT("G"):TEXT("S"),int(C.R*1000),int(C.G*1000),int(C.B*1000),int(R*100),int(M*100));
 for(auto* Mat:Materials)if(Mat&&Mat->GetName().StartsWith(Key))return Mat;
 auto* Base=LoadObject<UMaterialInterface>(nullptr,Glow?TEXT("/Game/Materials/M_Glow.M_Glow"):TEXT("/Game/Materials/M_SurfaceSoft.M_SurfaceSoft"));
 if(!Base)Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 auto* Mat=UMaterialInstanceDynamic::Create(Base,this,FName(*Key)); Mat->SetVectorParameterValue(TEXT("Tint"),C); Mat->SetScalarParameterValue(TEXT("Roughness"),R); Mat->SetScalarParameterValue(TEXT("Metallic"),M); Materials.Add(Mat);return Mat;
}
UStaticMeshComponent* AExtractionMode::Part(AActor* A,USceneComponent* Parent,FVector P,FVector S,FLinearColor C,int Mesh){
 auto* Comp=NewObject<UStaticMeshComponent>(A); A->AddInstanceComponent(Comp); Comp->SetStaticMesh(Mesh>=4&&Detailed.IsValidIndex(Mesh-4)&&Detailed[Mesh-4]?Detailed[Mesh-4]:Mesh==1?Sphere:Mesh==2?Cylinder:Mesh==3?Cone:Cube); Comp->SetMaterial(0,Material(C)); Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision); Comp->SetupAttachment(Parent); Comp->RegisterComponent(); Comp->SetRelativeLocation(P); Comp->SetRelativeScale3D(S); return Comp;
}
AActor* AExtractionMode::Shape(FVector P,FVector S,FLinearColor C,int Mesh,bool Coll,FRotator Rot,bool Track){
 if(BuildingSector)P.Z+=GroundHeight(P);
 auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(P,Rot); auto* Comp=A->GetStaticMeshComponent(); Comp->SetMobility(EComponentMobility::Movable); Comp->SetStaticMesh(Mesh>=4&&Detailed.IsValidIndex(Mesh-4)&&Detailed[Mesh-4]?Detailed[Mesh-4]:Mesh==1?Sphere:Mesh==2?Cylinder:Mesh==3?Cone:Cube); Comp->SetMaterial(0,Material(C)); Comp->SetCollisionEnabled(Coll?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision); Comp->SetCollisionProfileName(Coll?TEXT("BlockAll"):TEXT("NoCollision")); A->SetActorScale3D(S); if(Track)WorldActors.Add(A);if(Mesh==4)Canopies.Add(A);return A;
}
AActor* AExtractionMode::Asset(const FString& Name,FVector P,FVector Scale,FRotator Rotation,bool Collision){
 UStaticMesh* Mesh=ProductionMeshes.FindRef(Name);if(!Mesh){Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Production/Meshes/%s.%s"),*Name,*Name));if(Mesh)ProductionMeshes.Add(Name,Mesh);}if(!Mesh){UE_LOG(LogTemp,Warning,TEXT("Missing production asset %s"),*Name);return nullptr;}
 if(BuildingSector)P.Z+=GroundHeight(P);
 auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(P,Rotation);auto* C=A->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));A->SetActorScale3D(Scale);
 if(Name==TEXT("MarketStall")||Name==TEXT("VillageStore")){
  const int Variant=FMath::Abs(FMath::RoundToInt(P.X/300.f)+FMath::RoundToInt(P.Y/500.f))%4;
  const FLinearColor CanopyColors[4]={{.18,.27,.11},{.36,.10,.045},{.32,.24,.10},{.15,.20,.07}};
  const FLinearColor WallColors[4]={{.43,.32,.20},{.33,.36,.29},{.49,.40,.29},{.30,.25,.19}};
  for(int I=0;I<C->GetNumMaterials();I++){const FString MaterialName=GetNameSafe(C->GetMaterial(I));
   if(Name==TEXT("MarketStall")&&MaterialName.Contains(TEXT("Produce_Green")))C->SetMaterial(I,Material(CanopyColors[Variant],false,.9));
   if(Name==TEXT("VillageStore")&&MaterialName.Contains(TEXT("Village_Plaster"))){
    if(MaterialName.StartsWith(TEXT("M04_"))){auto* Plaster=UMaterialInstanceDynamic::Create(C->GetMaterial(I),this);Plaster->SetVectorParameterValue(TEXT("Tint"),WallColors[Variant]*2.4f);C->SetMaterial(I,Plaster);Materials.Add(Plaster);}
    else C->SetMaterial(I,Material(WallColors[Variant],false,.98));
   }
  }
 }
 WorldActors.Add(A);return A;
}
void AExtractionMode::AnimateCharacter(USkeletalMeshComponent* Mesh,float Speed){
 if(!Mesh||Cast<UCommandoAnimation>(Mesh->GetAnimInstance())||CharacterAnimations.Num()<3)return;int Index=Speed<12?0:Speed>480?2:1;auto* Clip=CharacterAnimations[Index];
 if(Clip&&Mesh->AnimationData.AnimToPlay!=Clip)Mesh->PlayAnimation(Clip,true);Mesh->SetPlayRate(Speed<12?1:FMath::Clamp(Speed/(Index==2?620.f:300.f),.65f,1.8f));
}
void AExtractionMode::MakeSoldier(AActor* A,bool Player,int Type){
 auto* Parent=Player?Hero->Visual:A->GetRootComponent();
 const TCHAR* Role=Type<0?(A->Tags.Contains(TEXT("WorkerWardrobe"))?TEXT("Worker06"):TEXT("Civilian06")):Type==1?TEXT("Scout06"):Type==2?TEXT("Heavy06"):TEXT("Guard06");
 FString MeshPath=Player?TEXT("/Game/Production/Rigged/CommandoVisual03.CommandoVisual03"):FString::Printf(TEXT("/Game/Production/Rigged/Characters06/%s.%s"),Role,Role);
 auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*MeshPath);
 if(!Mesh)UE_LOG(LogTemp,Error,TEXT("CHARACTER_WARDROBE_MISSING %s"),*MeshPath);
 if(Mesh){
  auto* C=NewObject<USkeletalMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetSkeletalMeshAsset(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetRelativeLocation(FVector(0,0,-100));C->SetRelativeRotation(FRotator(0,-90,0));
  C->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;C->SetAnimInstanceClass(UCommandoAnimation::StaticClass());if(Type==-1)if(auto* Anim=Cast<UCommandoAnimation>(C->GetAnimInstance()))Anim->SetCivilian(true);C->TickAnimation(.001f,false);C->RefreshBoneTransforms();if(Player)Hero->CharacterMesh=C;
  if(!Player){for(int i=0;i<C->GetNumMaterials();i++){FString N=C->GetMaterial(i)?C->GetMaterial(i)->GetName():TEXT("");if(N.Contains(TEXT("Red")))C->SetMaterial(i,Material(Type==-2?FLinearColor(.03,.34,.28):FLinearColor(.12,.16,.06)));if(Type==-1&&N.Contains(TEXT("Camo")))C->SetMaterial(i,Material(FLinearColor(.20,.16,.10)));}}
 }
 if(Type==-1)return;
 auto* Rifle=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Production/Meshes/FieldRifle.FieldRifle"));
 auto* Gun=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Gun);Gun->ComponentTags.Add(TEXT("CharacterWeapon"));Gun->SetupAttachment(Parent);Gun->SetStaticMesh(Rifle?Rifle:Cube);Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);Gun->RegisterComponent();Gun->SetRelativeLocation(FVector(20,12,38.5));Gun->SetRelativeRotation(FRotator(0,-90,0));if(!Rifle)Gun->SetRelativeScale3D(FVector(.8,.1,.12));if(auto* SkinMesh=A->FindComponentByClass<USkeletalMeshComponent>())Gun->AttachToComponent(SkinMesh,FAttachmentTransformRules::KeepWorldTransform,TEXT("hand_r"));if(Player)Hero->RifleParts.Add(Gun);
 if(Player){for(int j=0;j<18;j++){float T0=-1.3f+j*(2.6f/18),T1=T0+(2.6f/18);FVector A0(43+FMath::Cos(T0)*18,-12,25+FMath::Sin(T0)*55),A1(43+FMath::Cos(T1)*18,-12,25+FMath::Sin(T1)*55);auto* B=Part(A,Parent,(A0+A1)*.5,FVector((A1-A0).Size()/100,.025,.025),FLinearColor(.12,.045,.015));B->SetRelativeRotation((A1-A0).Rotation());B->SetVisibility(false);Hero->BowParts.Add(B);}auto* String=Part(A,Parent,FVector(48,-12,25),FVector(.007,.007,1.06),FLinearColor(.55,.48,.29));String->SetVisibility(false);Hero->BowParts.Add(String);}
}
void AExtractionMode::BeginPlay(){
 Super::BeginPlay(); Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")); Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));Cone=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone"));
 for(auto N:{TEXT("Palm"),TEXT("Rock"),TEXT("Head"),TEXT("Torso"),TEXT("Leg")})Detailed.Add(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Meshes/%s.%s"),N,N)));
 KeyArt=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/JungleKeyArt.JungleKeyArt"));CharacterArt=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/CharacterDesign.CharacterDesign"));Equipment=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/EquipmentAtlas.EquipmentAtlas"));
 for(auto S:{TEXT("Rifle"),TEXT("Arrow"),TEXT("Explosion"),TEXT("Pickup"),TEXT("Hit"),TEXT("Forest"),TEXT("JungleScore"),TEXT("Infiltration"),TEXT("Contact"),TEXT("Siege"),TEXT("Homeward")})Sounds.Add(LoadObject<USoundWave>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),S,S)));
 Hero=Cast<AExtractionPawn>(UGameplayStatics::GetPlayerPawn(this,0)); if(!Hero){Hero=GetWorld()->SpawnActor<AExtractionPawn>();GetWorld()->GetFirstPlayerController()->Possess(Hero);}
 for(auto N:{TEXT("Idle"),TEXT("Walk"),TEXT("Run")})CharacterAnimations.Add(LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Production/Character/Commando_Anim_CommandoRig_%s.Commando_Anim_CommandoRig_%s"),N,N)));
 MakeSoldier(Hero,true); Hero->SetActorLocation(Checkpoint); PrevHero=Checkpoint;
 auto* PC=GetWorld()->GetFirstPlayerController();PC->bShowMouseCursor=true; PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,4000),FRotator(-47,-35,0)); SectorSun=Sun; Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sun->GetLightComponent()->SetIntensity(4.0);Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1);Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetAtmosphereSunLight(true);GetWorld()->SpawnActor<ASkyAtmosphere>();Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.83,.62));
 auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,4000),FRotator(-62,140,0));Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);Fill->GetLightComponent()->SetIntensity(.55);Fill->GetLightComponent()->SetCastShadows(false);Fill->GetLightComponent()->SetLightColor(FLinearColor(.42,.65,.73));
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>(); SectorSky=Sky; Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable); Sky->GetLightComponent()->SetIntensity(1.1);Sky->GetLightComponent()->SourceType=ESkyLightSourceType::SLS_SpecifiedCubemap;Sky->GetLightComponent()->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Game/Production/Textures/ForestSky.ForestSky"))); Sky->GetLightComponent()->SetLightColor(FLinearColor(.78,.9,1));
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();SectorFog=Fog;Fog->GetComponent()->SetFogDensity(.014);Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.11,.23,.22)); Fog->GetComponent()->SetStartDistance(500);Fog->GetComponent()->SetVolumetricFog(true);Fog->GetComponent()->SetVolumetricFogDistance(14000);Fog->GetComponent()->SetVolumetricFogScatteringDistribution(.55);
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();Post->bUnbound=true;Post->Settings.bOverride_AutoExposureMethod=true;Post->Settings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;Post->Settings.bOverride_AutoExposureBias=true;Post->Settings.AutoExposureBias=0;Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Post->Settings.AutoExposureApplyPhysicalCameraExposure=false; Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.30;Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.45;
 for(int i=0;i<4;i++)MusicLayers.Add(Sounds.IsValidIndex(7+i)&&Sounds[7+i]?UGameplayStatics::SpawnSound2D(this,Sounds[7+i],.0001f):nullptr);
 if(Sounds[5])AmbiencePlayer=UGameplayStatics::SpawnSound2D(this,Sounds[5],EffectsVolume*.5f);
 Playtest=FParse::Param(FCommandLine::Get(),TEXT("route-test"));
 Autotest=FParse::Param(FCommandLine::Get(),TEXT("campaign-test"));
 ReadSave();
 if(FParse::Param(FCommandLine::Get(),TEXT("play-now"))||Autotest||Playtest||FParse::Param(FCommandLine::Get(),TEXT("capture-preview")))Start(Playtest&&HasSave&&FParse::Param(FCommandLine::Get(),TEXT("resume-route-test")));
}
void AExtractionMode::Notify(const FString& S,float T){Toast=S;ToastTime=T;}
void AExtractionMode::ReadSave(){
 HasSave=false;SavedStages.Empty();SavedEscortPositions.Empty();SavedHeroPositionValid=false;
 FString S;if(!FFileHelper::LoadFileToString(S,*(FPaths::ProjectSavedDir()/((Autotest||Playtest||FParse::Param(FCommandLine::Get(),TEXT("capture-preview")))?TEXT("CampaignTest.json"):TEXT("Campaign.json")))))return;
 TSharedPtr<FJsonObject> J;auto Reader=TJsonReaderFactory<>::Create(S);if(!FJsonSerializer::Deserialize(Reader,J)||!J.IsValid())return;
 SavedChapter=FMath::Clamp(int(J->GetNumberField(TEXT("chapter"))),0,5);SavedMask=int(J->GetNumberField(TEXT("mask")));HasSave=true;
 const TArray<TSharedPtr<FJsonValue>>* Stages=nullptr;
 if(J->TryGetArrayField(TEXT("objective_stages"),Stages))for(auto& V:*Stages)SavedStages.Add(FMath::Clamp(int(V->AsNumber()),0,1));
 auto ReadPosition=[](const TArray<TSharedPtr<FJsonValue>>& V,FVector& Out){if(V.Num()!=3)return false;Out=FVector(V[0]->AsNumber(),V[1]->AsNumber(),V[2]->AsNumber());return !Out.ContainsNaN()&&FMath::Abs(Out.X)<=10000&&FMath::Abs(Out.Y)<=7000;};
 const TArray<TSharedPtr<FJsonValue>>* Positions=nullptr;
 if(J->TryGetArrayField(TEXT("escort_positions"),Positions))for(auto& V:*Positions){FVector P=FVector::ZeroVector;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;if(V->TryGetArray(Values)&&!ReadPosition(*Values,P))P=FVector::ZeroVector;SavedEscortPositions.Add(P);}
 const TArray<TSharedPtr<FJsonValue>>* HeroPosition=nullptr;if(J->TryGetArrayField(TEXT("hero_position"),HeroPosition))SavedHeroPositionValid=ReadPosition(*HeroPosition,SavedHeroPosition);
}
void AExtractionMode::Save(){
 int Mask=0;for(int i=0;i<Objectives.Num();i++)if(Objectives[i].Done)Mask|=(1<<i);
 auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("save_version"),2);J->SetNumberField(TEXT("chapter"),Chapter);J->SetNumberField(TEXT("mask"),Mask);J->SetNumberField(TEXT("kills"),Kills);J->SetNumberField(TEXT("elapsed"),Elapsed);J->SetNumberField(TEXT("difficulty"),Difficulty);J->SetNumberField(TEXT("rescued"),RescuedTotal);
 auto PositionValues=[](FVector P){TArray<TSharedPtr<FJsonValue>> Values;for(double V:{P.X,P.Y,P.Z})Values.Add(MakeShared<FJsonValueNumber>(V));return Values;};
 TArray<TSharedPtr<FJsonValue>> Stages,Positions;SavedStages.Empty();SavedEscortPositions.Empty();
 for(const auto& O:Objectives){int Stage=O.Done?0:O.Stage;FVector P=Stage==1&&IsValid(O.Prop)?O.Prop->GetActorLocation():FVector::ZeroVector;SavedStages.Add(Stage);SavedEscortPositions.Add(P);Stages.Add(MakeShared<FJsonValueNumber>(Stage));Positions.Add(MakeShared<FJsonValueArray>(PositionValues(P)));}
 J->SetArrayField(TEXT("objective_stages"),Stages);J->SetArrayField(TEXT("escort_positions"),Positions);SavedHeroPosition=Hero->GetActorLocation();SavedHeroPositionValid=true;J->SetArrayField(TEXT("hero_position"),PositionValues(SavedHeroPosition));
 FString S;auto W=TJsonWriterFactory<>::Create(&S);FJsonSerializer::Serialize(J,W);FFileHelper::SaveStringToFile(S,*(FPaths::ProjectSavedDir()/((Autotest||Playtest||FParse::Param(FCommandLine::Get(),TEXT("capture-preview")))?TEXT("CampaignTest.json"):TEXT("Campaign.json"))));SavedChapter=Chapter;SavedMask=Mask;HasSave=true;
}
void AExtractionMode::Start(bool Continue){
 Menu=false;Dead=false;Victory=false;Ending=false;Paused=false;Elapsed=0;Kills=0;RescuedTotal=0;Hero->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);Hero->SetActorRotation(FRotator::ZeroRotator);Hero->SetActorHiddenInGame(false);Hero->Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Hero->Camera->SetAbsolute(false,false,false);Hero->Camera->SetRelativeLocation(FVector(-1450,0,1550));Hero->Camera->SetRelativeRotation(FRotator(-47,0,0));Hero->Camera->FieldOfView=57;GetWorld()->GetFirstPlayerController()->bShowMouseCursor=true;if(Hero->CharacterMesh)if(auto* A=Cast<UCommandoAnimation>(Hero->CharacterMesh->GetAnimInstance()))A->SetCivilian(false);Chapter=Continue?SavedChapter:0;
 if(Continue){FString S;FFileHelper::LoadFileToString(S,*(FPaths::ProjectSavedDir()/((Autotest||Playtest||FParse::Param(FCommandLine::Get(),TEXT("capture-preview")))?TEXT("CampaignTest.json"):TEXT("Campaign.json"))));TSharedPtr<FJsonObject>J;auto R=TJsonReaderFactory<>::Create(S);if(FJsonSerializer::Deserialize(R,J)&&J.IsValid()){double SavedRescued=0;if(J->TryGetNumberField(TEXT("rescued"),SavedRescued))RescuedTotal=int(SavedRescued);else{const int Counts[6]={2,2,2,2,1,3},Masks[6]={10,34,6,18,8,37};for(int I=0;I<Chapter;I++)RescuedTotal+=Counts[I];for(int I=0;I<7;I++)if((SavedMask&Masks[Chapter])&(1<<I))RescuedTotal++;}Elapsed=J->GetNumberField(TEXT("elapsed"));Kills=J->GetNumberField(TEXT("kills"));Difficulty=FMath::Clamp(int(J->GetNumberField(TEXT("difficulty"))),0,2);}}
 if(FParse::Param(FCommandLine::Get(),TEXT("capture-preview"))){int PreviewChapter=0;FParse::Value(FCommandLine::Get(),TEXT("PreviewChapter="),PreviewChapter);Chapter=FMath::Clamp(PreviewChapter,0,5);}
 if(Playtest){int TestChapter=0;if(FParse::Value(FCommandLine::Get(),TEXT("TestChapter="),TestChapter))Chapter=FMath::Clamp(TestChapter,0,5);}
 int TestMask=0;if(Playtest)FParse::Value(FCommandLine::Get(),TEXT("TestMask="),TestMask);
 if(FParse::Param(FCommandLine::Get(),TEXT("ending-preview"))){Chapter=5;TestMask=127;RescuedTotal=12;}
 BuildChapter(Continue?SavedMask:TestMask,Continue); Save();if(FParse::Param(FCommandLine::Get(),TEXT("ending-preview")))BeginExtraction();
}
void AExtractionMode::BuildChapter(int Mask,bool RestoreStages){
 PendingWeaponDrops.Reset();DroppedWeapons.Reset();WeaponDropKicks.Reset();BuildingSector=false;ConfigureSector();if(Chapter==1&&(Mask&(1<<3)))BridgeOpen=true;
 for(auto& B:Blasts)if(IsValid(B.Actor))B.Actor->Destroy();Blasts.Empty();
 for(auto* A:WorldActors)if(IsValid(A))A->Destroy();WorldActors.Empty();Canopies.Empty();
 for(auto& E:Enemies)if(IsValid(E.Actor))E.Actor->Destroy(); Enemies.Empty();
 for(auto& S:Shots)if(IsValid(S.Visual))S.Visual->Destroy();Shots.Empty();
 for(auto& T:Thrown)if(IsValid(T.Actor))T.Actor->Destroy();Thrown.Empty();Followers.Empty();AllyRoutes.Empty();AllyRepaths.Empty();AllyCooldowns.Empty();ClickMoving=false;Route.Empty();TestRepath=0;TestLastCompleted=-1;TestLastStage=-1;Objectives.Empty();Hero->Health=100;Hero->Stamina=100;DashTime=0;if(Hero->CharacterMesh)Hero->CharacterMesh->InitAnim(true);Hero->Invincible=3;Ammo=30;Reserve=210;Arrows=24;Grenades=5;Medkits=3;Weapon=0;Reload=FireCD=Alarm=Interact=Defense=0;Completed=0;Focused=-1;LocalTime=0;Checkpoint=SectorStart;Checkpoint.Z=GroundHeight(Checkpoint)+100;Hero->SetActorLocation(Checkpoint);Random.Initialize(1985+Chapter*137);
 ConfigureAtmosphere();BuildTerrain();BuildingSector=true;
 // Kinds: rescue, sabotage, intelligence, radio defense, heavy armor.
 const int Kinds[6][7]={{2,0,1,0,2,1,3},{2,0,1,5,4,0,3},{2,0,0,1,2,4,3},{1,0,1,2,0,4,3},{2,1,4,0,1,4,3},{0,1,0,4,1,0,3}};
 const TCHAR* Names[6][7]={
  {TEXT("정찰 기록 회수"),TEXT("수색대원 구출"),TEXT("정글 중계기 파괴"),TEXT("실종 정찰대 구출"),TEXT("보급로 지도 확보"),TEXT("감시 발전기 파괴"),TEXT("정글 무전기 방어")},
  {TEXT("강변 순찰표 회수"),TEXT("교량 기관사 구출"),TEXT("통제문 전력 차단"),TEXT("교량 통제문 개방"),TEXT("강 건너 장갑차 격파"),TEXT("선착장 포로 구출"),TEXT("강변 송신소 방어")},
  {TEXT("점령군 명부 회수"),TEXT("주민 안내원 구출"),TEXT("시장 생존자 구출"),TEXT("검문소 발전기 파괴"),TEXT("수용소 위치 확보"),TEXT("광장 장갑차 격파"),TEXT("마을 교신소 방어")},
  {TEXT("채굴 설비 파괴"),TEXT("갱도 노동자 구출"),TEXT("운반 시설 파괴"),TEXT("노동자 명부 회수"),TEXT("작업반 포로 구출"),TEXT("채석장 장갑차 격파"),TEXT("작업 통신소 방어")},
  {TEXT("요새 배치도 확보"),TEXT("남쪽 방공망 차단"),TEXT("외성 장갑차 격파"),TEXT("연락 장교 구출"),TEXT("북쪽 방공망 차단"),TEXT("사령부 장갑차 격파"),TEXT("지휘 통신소 점령")},
  {TEXT("지상 유도원 구출"),TEXT("연료 공급망 차단"),TEXT("격납고 포로 구출"),TEXT("활주로 장갑차 격파"),TEXT("비행장 방공망 차단"),TEXT("마지막 포로 구출"),TEXT("구조 헬기 유도")}
 };
 for(int n=0;n<7;n++){
  FVector P=SectorNodes[n];
  FExtractionObjective O;O.Position=P;O.Position.Z=GroundHeight(P);O.Origin=O.Position;O.Rally=RescueRallyPoint(n);O.Kind=Kinds[Chapter][n];O.Name=FString::Printf(TEXT("%02d  %s"),n+1,Names[Chapter][n]);O.Done=(Mask&(1<<n))!=0;
  O.Description=O.Kind==0?TEXT("주변 적 제압 후 E · 포로를 풀어주고 안전 지점까지 엄호"):O.Kind==1?TEXT("E를 길게 눌러 폭약 설치"):O.Kind==2?TEXT("E를 길게 눌러 문서 회수"):O.Kind==3?TEXT("E로 교신 개시 · 50초간 무전기 방어"):O.Kind==4?TEXT("장갑차를 사격하거나 수류탄으로 파괴"):TEXT("기관사 구출 · 전력 차단 후 E로 통제문 개방");
  O.Marker=Shape(P+FVector(0,0,14),FVector(.28,.28,.06),O.Done?Teal:Amber,2,false);
  Cast<AStaticMeshActor>(O.Marker)->GetStaticMeshComponent()->SetMaterial(0,Material(O.Done?Teal:Amber,true));
  BuildObjectiveSite(P,n,O.Kind);
  if(O.Kind==0){
   O.Prop=Shape(P+FVector(0,0,100),FVector(1),Skin,1,false);Cast<AStaticMeshActor>(O.Prop)->GetStaticMeshComponent()->SetVisibility(false,false);if((Chapter==1&&n==1)||Chapter==3||(Chapter==5&&n==0))O.Prop->Tags.Add(TEXT("WorkerWardrobe"));MakeSoldier(O.Prop,false,-1);
   for(int j=0;j<7;j++)Shape(P+FVector(-170+j*55,120,100),FVector(.025,.025,2.0),Dark,2,false);
  }
  else if(O.Kind==1)O.Prop=Asset(TEXT("FieldGenerator"),P,FVector(1),FRotator::ZeroRotator,true);
  else if(O.Kind==2)O.Prop=Asset(TEXT("IntelDesk"),P,FVector(1),FRotator::ZeroRotator,true);
  else if(O.Kind==3)O.Prop=Asset(TEXT("RadioStation"),P,FVector(1),FRotator::ZeroRotator,true);
  else if(O.Kind==5)O.Prop=Asset(TEXT("BridgeWinch"),P,FVector(1),FRotator::ZeroRotator,true);
  else O.Prop=nullptr; // The live armored vehicle is the target; no duplicate placeholder.
  if(O.Done){Completed++;if(IsValid(O.Prop)){O.Prop->SetActorHiddenInGame(true);O.Prop->SetActorEnableCollision(false);}Checkpoint=P+FVector(-300,-180,0);Checkpoint.Z=GroundHeight(Checkpoint)+100;}
  Objectives.Add(O);
  const bool PrisonCleared=RestoreStages&&O.Kind==0&&SavedStages.IsValidIndex(n)&&SavedStages[n]==1;
  if(!O.Done&&!PrisonCleared)SpawnSiteGuards(P,n,O.Kind);
 }
 // Keep scenery identical when completed mission groups no longer spawn guards.
 Random.Initialize(31985+Chapter*137);
 // Scanned jungle layers: tall canopy, fern understory, moss-covered rocks and fallen trunks.
 const int LargePlants[6]={740,430,130,80,130,70};
 for(int i=0;i<LargePlants[Chapter];i++){
  FVector P(Random.FRandRange(-10200,10200),Random.FRandRange(-7300,7300),0);bool Clear=FVector::Dist2D(P,SectorStart)<750||!TerrainWalkable(P)||(Chapter==1&&FMath::Abs(P.X-RiverCenter(P.Y))<RiverHalfWidth(P.Y)+700)||(Chapter==2)||(Chapter==4&&FMath::Abs(P.X)<4900)||(Chapter==5&&FMath::Abs(P.Y)<5200)||RouteDistance(P)<450;
  for(auto& O:Objectives)if(FVector::Dist2D(P,O.Position)<1450)Clear=true;
  if(Clear)continue;
  float R=Random.FRandRange(.6,1.1);
  if(i%3==0){Asset(FString::Printf(TEXT("rock_moss_set_01_%d"),i%6),P,FVector(R*1.7),FRotator(0,Random.FRandRange(0,360),0),true);continue;}
  if(i%2==0){auto* Tree=Asset(TEXT("island_tree_02"),P,FVector(R*2.3),FRotator(0,Random.FRandRange(0,360),0));if(Tree)Canopies.Add(Tree);}
  else Asset(TEXT("dead_tree_trunk"),P,FVector(R),FRotator(0,Random.FRandRange(0,360),0),true);
 }
 const int SmallPlants[6]={2400,1600,350,180,300,170};
 for(int i=0;i<SmallPlants[Chapter];i++){
  FVector P(Random.FRandRange(-10000,10000),Random.FRandRange(-7000,7000),0);bool Clear=!TerrainWalkable(P)||(Chapter==1&&FMath::Abs(P.X-RiverCenter(P.Y))<RiverHalfWidth(P.Y)+40)||RouteDistance(P)<180||(Chapter==5&&FMath::Abs(P.Y)<1300);for(auto& O:Objectives)if(FVector::Dist2D(P,O.Position)<450)Clear=true;if(Clear)continue;
  float R=Random.FRandRange(1.0,2.5);Asset(FString::Printf(TEXT("fern_02_%d"),i%4),P,FVector(R),FRotator(0,Random.FRandRange(0,360),0));
 }
 // Scanned rock silhouettes replace the faceted placeholder mountain ring.
 for(int i=0;i<38;i++){float A=i*2*PI/38;FVector P(FMath::Cos(A)*14500,FMath::Sin(A)*11200,-350);if(auto* Rock=Asset(FString::Printf(TEXT("rock_moss_set_01_%d"),i%6),P,FVector(8,8,6+Chapter*.5),FRotator(0,i*37,0)))Cast<AStaticMeshActor>(Rock)->GetStaticMeshComponent()->SetCastShadow(false);}
 Shape(Exit+FVector(0,0,4),FVector(Chapter==5?17:8,Chapter==5?17:8,.04),FLinearColor(.10,.21,.17),2,false);Shape(Exit+FVector(0,0,8),FVector(3,.3,.02),Teal,0,false);Shape(Exit+FVector(-135,0,8),FVector(.3,3,.02),Teal,0,false);Shape(Exit+FVector(135,0,8),FVector(.3,3,.02),Teal,0,false);
 BuildSectorLandmarks();BuildingSector=false;Exit.Z=GroundHeight(Exit);BuildEscortSites();BuildEnemyCover();
 if(RestoreStages&&SavedHeroPositionValid&&TerrainWalkable(SavedHeroPosition)){Checkpoint=SavedHeroPosition;Checkpoint.Z=GroundHeight(Checkpoint)+100;}
 Hero->SetActorLocation(Checkpoint);PrevHero=Checkpoint;
 for(const auto& O:Objectives)if(O.Done&&O.Kind==0){FVector P=Checkpoint+FVector(-140-Followers.Num()*110,-130,0);P.Z=GroundHeight(P)+100;auto* Survivor=Shape(P,FVector(1),Skin,1,false);Cast<AStaticMeshActor>(Survivor)->GetStaticMeshComponent()->SetVisibility(false,false);if(IsValid(O.Prop))Survivor->Tags=O.Prop->Tags;MakeSoldier(Survivor,false,-2);Followers.Add(Survivor);AllyRoutes.AddDefaulted();AllyRepaths.Add(0);AllyCooldowns.Add(.5f);}

 if(RestoreStages)for(int I=0;I<Objectives.Num();I++)if(SavedStages.IsValidIndex(I)&&SavedStages[I]==1&&!Objectives[I].Done&&Objectives[I].Kind==0)BeginEscort(I,true);
 if(FParse::Param(FCommandLine::Get(),TEXT("capture-preview"))){Hero->SetActorLocation(FVector(-7000,-3700,100));Hero->Invincible=100;if(FParse::Param(FCommandLine::Get(),TEXT("capture-props"))){int Node=0;FParse::Value(FCommandLine::Get(),TEXT("PreviewNode="),Node);FVector P=Objectives[FMath::Clamp(Node,0,6)].Position;Hero->SetActorLocation(P+FVector(-420,-320,100));Hero->Camera->SetRelativeLocation(FVector(-580,-230,640));Hero->Camera->SetWorldRotation((P+FVector(0,0,100)-Hero->Camera->GetComponentLocation()).Rotation());Hero->Camera->FieldOfView=46;}if(FParse::Param(FCommandLine::Get(),TEXT("capture-character"))){Hero->Camera->SetRelativeLocation(FVector(600,-430,210));Hero->Camera->SetRelativeRotation((FVector(0,0,0)-FVector(600,-430,210)).Rotation());Hero->Camera->FieldOfView=35;if(FParse::Param(FCommandLine::Get(),TEXT("rig-preview"))){Hero->SetActorLocation(Checkpoint);for(auto* A:WorldActors)if(IsValid(A)&&FVector::Dist2D(A->GetActorLocation(),Checkpoint)<2500){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);Canopies.Remove(A);}SpawnEnemy(Checkpoint+FVector(0,-170,0));}}}
 if(FParse::Param(FCommandLine::Get(),TEXT("site-preview"))){
  int Node=0;FParse::Value(FCommandLine::Get(),TEXT("PreviewNode="),Node);FVector P=Objectives[FMath::Clamp(Node,0,6)].Position;P.Z=GroundHeight(P)+130;
  Hero->SetActorLocation(P+FVector(-420,-380,-30));Hero->Camera->SetAbsolute(true,true,false);FVector Offset(-1750,-2150,1950);P.Y+=200;Hero->Camera->SetWorldLocation(P+Offset);Hero->Camera->SetWorldRotation((-Offset).Rotation());Hero->Camera->FieldOfView=56;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("record-combat"))){Hero->Camera->SetRelativeLocation(FVector(-1060,-160,1180));Hero->Camera->SetRelativeRotation((FVector(100,0,0)-Hero->Camera->GetRelativeLocation()).Rotation());Hero->Camera->FieldOfView=58;}
 if(FParse::Param(FCommandLine::Get(),TEXT("capture-bridge"))){
  FVector P(-1850,-3450,0);P.Z=GroundHeight(P)+100;Hero->SetActorLocation(P);
  Hero->Camera->SetRelativeLocation(FVector(-1800,-1800,2000));Hero->Camera->SetWorldRotation((FVector(300,-2800,100)-Hero->Camera->GetComponentLocation()).Rotation());Hero->Camera->FieldOfView=62;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("capture-overview"))){Hero->Camera->SetAbsolute(true,true,false);Hero->Camera->SetWorldLocation(FVector(-7200,-9300,16800));Hero->Camera->SetWorldRotation((FVector(0,0,0)-Hero->Camera->GetComponentLocation()).Rotation());Hero->Camera->FieldOfView=69;}
 if(FParse::Param(FCommandLine::Get(),TEXT("capture-landmark"))){
  const FVector Points[6]={SectorNodes[5]+FVector(750,-750,0),FVector(RiverCenter(1300)-RiverHalfWidth(1300)+150,1300,0),FVector(-1000,-1050,0),SectorNodes[1]+FVector(850,-650,0),SectorNodes[0]+FVector(650,0,0),FVector(-5900,-3200,0)};
  FVector P=Points[Chapter];P.Z=GroundHeight(P)+110;
  Hero->Camera->SetAbsolute(true,true,false);Hero->Camera->SetWorldLocation(P+FVector(-1150,-1550,1050));Hero->Camera->SetWorldRotation((P-Hero->Camera->GetComponentLocation()).Rotation());Hero->Camera->FieldOfView=57;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("wardrobe-preview"))){
  Hero->SetActorLocation(Checkpoint);Hero->Visual->SetRelativeRotation(FRotator::ZeroRotator);Hero->Invincible=100;
  for(auto* A:WorldActors)if(IsValid(A)&&FVector::Dist2D(A->GetActorLocation(),Checkpoint)<2400){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);Canopies.Remove(A);}
  for(auto& E:Enemies)if(IsValid(E.Actor))E.Actor->SetActorHiddenInGame(true);
  Followers.Reset();
  for(int I=0;I<5;I++){
   FVector P=Checkpoint+FVector(0,(I+1)*180,0);P.Z=GroundHeight(P)+100;
   auto* Person=Shape(P,FVector(1),Skin,1,false);Cast<AStaticMeshActor>(Person)->GetStaticMeshComponent()->SetVisibility(false,false);if(I==4)Person->Tags.Add(TEXT("WorkerWardrobe"));MakeSoldier(Person,false,I<3?I:-1);Followers.Add(Person);
  }
  FVector Target=Checkpoint+FVector(0,450,-5);Hero->Camera->SetAbsolute(true,true,false);Hero->Camera->SetWorldLocation(Target+FVector(1800,-260,540));Hero->Camera->SetWorldRotation((Target-Hero->Camera->GetComponentLocation()).Rotation());Hero->Camera->FieldOfView=38;
 }
 BuildFallPreview();Notify(ChapterNames[Chapter]+TEXT("\n")+Briefs[Chapter],7);
 UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_CHAPTER_READY chapter=%d objectives=%d enemies=%d actors=%d"),Chapter,Objectives.Num(),Enemies.Num(),WorldActors.Num());
}
void AExtractionMode::SpawnEnemy(FVector P,int Type){
 P.Z=GroundHeight(P)+100;if(!TerrainWalkable(P)){const FVector Origin=P;for(int R=1;R<18&&!TerrainWalkable(P);R++)for(int K=0;K<16;K++){FVector Candidate=Origin+FVector(FMath::Cos(K*PI/8)*R*180,FMath::Sin(K*PI/8)*R*180,0);if(TerrainWalkable(Candidate)){P=Candidate;break;}}P.Z=GroundHeight(P)+100;}
 if(Type!=3){
  const FVector Wanted=P;FCollisionQueryParams Q;Q.AddIgnoredActor(Hero);bool Clear=false;
  for(int Ring=0;Ring<14&&!Clear;Ring++)for(int K=0;K<(Ring?16:1);K++){
   FVector Candidate=Wanted+FVector(FMath::Cos(K*PI/8),FMath::Sin(K*PI/8),0)*(Ring*110);Candidate.Z=GroundHeight(Candidate)+100;
   if(TerrainWalkable(Candidate)&&!GetWorld()->OverlapBlockingTestByChannel(Candidate,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(44,90),Q)){P=Candidate;Clear=true;break;}
  }
  ensureAlwaysMsgf(Clear,TEXT("Guard spawn requires full-body clearance"));
 }
 auto* A=GetWorld()->SpawnActor<AActor>(P,FRotator::ZeroRotator);auto* C=NewObject<UCapsuleComponent>(A);A->SetRootComponent(C);A->AddInstanceComponent(C);C->InitCapsuleSize(Type==3?165:36,Type==3?165:90);C->SetCollisionProfileName(TEXT("Pawn"));C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->RegisterComponent();A->SetActorLocation(P);
 if(Type==3){
  auto* Armor=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Armor);Armor->SetupAttachment(C);Armor->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Production/Meshes/ArmoredPatrol.ArmoredPatrol")));Armor->SetCollisionEnabled(ECollisionEnabled::NoCollision);Armor->RegisterComponent();Armor->SetRelativeLocation(FVector(0,0,-100));
 }
 else MakeSoldier(A,false,Type);
 FExtractionEnemy E;E.Actor=A;E.Home=P;E.Type=Type;E.Health=Type==3?580:Type==2?150:80;E.Cooldown=Random.FRandRange(.5,2.5);E.Seed=Random.FRandRange(0,100);E.Identity=Enemies.Num();E.Goal=P;E.Think=E.Identity*.023f;Enemies.Add(E);
}
void AExtractionMode::Flash(FVector P,FLinearColor C,float Size,float Life){
 auto* A=Shape(P,FVector(Size),C,1,false,FRotator::ZeroRotator,false);Cast<AStaticMeshActor>(A)->GetStaticMeshComponent()->SetMaterial(0,Material(C*6,true));A->SetLifeSpan(Life);
 auto* L=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(L);L->SetupAttachment(A->GetRootComponent());L->RegisterComponent();L->SetIntensity(Size*1200);L->SetAttenuationRadius(250+Size*100);L->SetLightColor(C);
}
void AExtractionMode::Beam(FVector A,FVector B,FLinearColor C,float Life){
 FVector V=B-A;auto* Actor=Shape((A+B)*.5,FVector(V.Size()/100,.018,.018),C,0,false,V.Rotation(),false);Cast<AStaticMeshActor>(Actor)->GetStaticMeshComponent()->SetMaterial(0,Material(C*4,true));Actor->SetLifeSpan(Life);
}
void AExtractionMode::Sound(int Index,FVector P,float V){
 if(Sounds.IsValidIndex(Index)&&Sounds[Index])UGameplayStatics::PlaySoundAtLocation(this,Sounds[Index],P,V*EffectsVolume);
 RecordEscortSound(Index,P,V);
 if(CombatRecordStart>=0&&FParse::Param(FCommandLine::Get(),TEXT("record-combat"))){FString Event=FString::Printf(TEXT("{\"time\":%.4f,\"sound\":%d,\"volume\":%.3f,\"distance\":%.1f}\n"),LocalTime-CombatRecordStart,Index,V,FVector::Dist(P,Hero->GetActorLocation()));FFileHelper::SaveStringToFile(Event,*(FPaths::ProjectDir()/(FParse::Param(FCommandLine::Get(),TEXT("record-allies"))?TEXT("../../work/world_revision/allies/events.jsonl"):TEXT("../../work/gameplay_capture/events.jsonl"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);}
}
bool AExtractionMode::CanSee(FVector A,FVector B,AActor* Ignore)const{
 FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(Hero);if(Ignore)Q.AddIgnoredActor(Ignore);
 return !GetWorld()->LineTraceSingleByChannel(Hit,A,B,ECC_Visibility,Q);
}
void AExtractionMode::Shoot(){
 if(FireCD>0||Reload>0)return;
 if((Weapon==0&&Ammo<=0)||(Weapon==1&&Arrows<=0)){Notify(Weapon==0?TEXT("R 키로 재장전"):TEXT("화살이 없습니다 · 1 키로 소총 전환"),1);return;}
 if(CombatRecordStart<0&&FParse::Param(FCommandLine::Get(),TEXT("record-combat"))&&(!FParse::Param(FCommandLine::Get(),TEXT("record-allies"))||AllyShots>0))CombatRecordStart=LocalTime;CharacterAction(Hero,TEXT("Fire"));if(Weapon==0)Ammo--;else Arrows--;FireCD=Weapon==0?.13:.65;
 FVector Origin=Hero->GetActorLocation()+FVector(0,0,12);FVector Dir=(Aim-Origin).GetSafeNormal();Origin+=Dir*72;FVector End=Origin+Dir*(Weapon==0?2400:2200);
 FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(Hero);GetWorld()->LineTraceSingleByChannel(Hit,Origin,End,ECC_Visibility,Q);if(Hit.bBlockingHit)End=Hit.ImpactPoint;
 Beam(Origin,End,Weapon==0?Amber:Teal,Weapon==0?.065:.15);Flash(Origin,Amber,Weapon==0?.23:.07,.07);Sound(Weapon==0?0:1,Origin,.45);
 if(Weapon==0){Alarm=FMath::Min(100.f,Alarm+3);HearCombatNoise(Hero->GetActorLocation(),1800);for(auto& E:Enemies)if(IsValid(E.Actor)&&FMath::PointDistToSegment(E.Actor->GetActorLocation()+FVector(0,0,12),Origin,End)<160)E.Suppression=2.8f;}
 for(int i=0;i<Enemies.Num();i++){auto& E=Enemies[i];if(E.Actor==Hit.GetActor()){E.Health-=Weapon==0?34:125;E.Alert=10;E.LastKnown=Hero->GetActorLocation();E.HasContact=true;E.Suppression=3;E.Think=0;Flash(End,Teal,.3,.12);if(E.Health<=0)KillEnemy(i,Dir);else CharacterAction(E.Actor,TEXT("Hit"));break;}}
}
void AExtractionMode::KillEnemy(int I,FVector ImpactDirection){
 auto& E=Enemies[I];if(!IsValid(E.Actor)||E.Health>0)return;
 FVector P=E.Actor->GetActorLocation();bool Tank=E.Type==3;E.Actor->SetActorEnableCollision(false);if(Tank)E.Actor->SetActorRotation(FRotator(0,E.Actor->GetActorRotation().Yaw,85));else StartCharacterFall(E.Actor,ImpactDirection);E.Actor->SetLifeSpan(8);E.Actor=nullptr;Kills++;Reserve=FMath::Min(420,Reserve+8);if(Kills%4==0)Arrows=FMath::Min(40,Arrows+2);if(Kills%12==0)Grenades=FMath::Min(8,Grenades+1);
 if(Tank){CombatAttention=P;CombatAttentionUntil=LocalTime+2.8;BlastVisual(P,1.4);Sound(2,P,.7);for(int j=0;j<Objectives.Num();j++)if(!Objectives[j].Done&&Objectives[j].Kind==4&&FVector::Dist2D(P,Objectives[j].Position)<1000){CompleteObjective(j);break;}}
}
void AExtractionMode::Grenade(){
 if(Grenades<=0){Notify(TEXT("수류탄이 없습니다"),1);return;}Grenades--;FVector P=Aim;P.Z=50;FVector Delta=P-Hero->GetActorLocation();if(Delta.Size2D()>1300)P=Hero->GetActorLocation()+Delta.GetSafeNormal2D()*1300;P.Z=GroundHeight(P)+60;FThrownGrenade T;T.Origin=Hero->GetActorLocation()+FVector(0,0,30);T.Target=P;T.Actor=Shape(T.Origin,FVector(.17),Olive,1,false,FRotator::ZeroRotator,false);Thrown.Add(T);
}
void AExtractionMode::Explode(FVector P){
 HearCombatNoise(P,2500);BlastVisual(P);Flash(P,Amber,.35,.14);Sound(2,P,.8);for(int i=0;i<Enemies.Num();i++){auto& E=Enemies[i];if(IsValid(E.Actor)&&FVector::Dist2D(E.Actor->GetActorLocation(),P)<520){E.Health-=320;E.Alert=12;E.Suppression=4;if(E.Health<=0)KillEnemy(i,(E.Actor->GetActorLocation()-P).GetSafeNormal2D());}}
 Alarm=100;
}
void AExtractionMode::UseMedkit(){if(Medkits>0&&Hero->Health<100){Medkits--;Hero->Health=FMath::Min(100.f,Hero->Health+65);Sound(3,Hero->GetActorLocation());Notify(TEXT("응급 처치 +65"),2);}}
void AExtractionMode::DamageHero(float D,FVector ImpactDirection){if(Hero->Invincible>0||Dead||Menu)return;Hero->Health-=D*(Difficulty==0?.55:Difficulty==2?1.25:1);HitFlash=.25;CharacterAction(Hero,TEXT("Hit"));Sound(4,Hero->GetActorLocation(),.25);if(Hero->Health<=0){Hero->Health=0;Dead=true;StartCharacterFall(Hero,ImpactDirection);Notify(TEXT("작전 실패 · Enter 키로 체크포인트에서 재시도"),10);}}
void AExtractionMode::CompleteObjective(int I){
 if(Objectives.IsValidIndex(I)&&!Objectives[I].Done&&Objectives[I].Kind==0&&EscortRecordStart>=0&&EscortRecordStop<0)EscortRecordStop=LocalTime+5;
 if(!Objectives.IsValidIndex(I)||Objectives[I].Done)return;auto& O=Objectives[I];O.Done=true;Completed++;if(O.Kind==0){FVector SurvivorPosition=IsValid(O.Prop)?O.Prop->GetActorLocation():O.Position+FVector(0,0,100);auto* Survivor=Shape(SurvivorPosition,FVector(1),Skin,1,false);Cast<AStaticMeshActor>(Survivor)->GetStaticMeshComponent()->SetVisibility(false,false);if(IsValid(O.Prop))Survivor->Tags=O.Prop->Tags;MakeSoldier(Survivor,false,-2);Followers.Add(Survivor);AllyRoutes.AddDefaulted();AllyRepaths.Add(0);AllyCooldowns.Add(.5f);RescuedTotal++;if(IsValid(O.Prop)){auto* Before=O.Prop->FindComponentByClass<USkeletalMeshComponent>();auto* After=Survivor->FindComponentByClass<USkeletalMeshComponent>();bool Same=Before&&After&&Before->GetSkeletalMeshAsset()==After->GetSkeletalMeshAsset();UE_LOG(LogTemp,Display,TEXT("ESCORT_WARDROBE chapter=%d objective=%d same=%d mesh=%s"),Chapter,I,Same?1:0,*GetNameSafe(After?After->GetSkeletalMeshAsset():nullptr));}if(IsValid(O.RallyMarker))O.RallyMarker->SetActorHiddenInGame(true);UE_LOG(LogTemp,Display,TEXT("ESCORT_COMPLETE chapter=%d objective=%d rescued=%d allies=%d distance=%.1f"),Chapter,I,RescuedTotal,Followers.Num(),FVector::Dist2D(SurvivorPosition,O.Rally));}
 Checkpoint=O.Position+FVector(-280,-230,0);Checkpoint.Z=GroundHeight(Checkpoint)+100;
 if(O.Kind==5){BridgeOpen=true;UE_LOG(LogTemp,Display,TEXT("RIVER_BRIDGE_OPEN"));}
 Hero->Health=FMath::Min(100.f,Hero->Health+30);Reserve=FMath::Min(420,Reserve+60);Arrows=FMath::Min(40,Arrows+5);Medkits=FMath::Min(5,Medkits+1);if(O.Kind==1){BlastVisual(O.Position+FVector(0,0,70),1.3);Sound(2,O.Position,.6);}if(IsValid(O.Prop)){O.Prop->SetActorHiddenInGame(true);O.Prop->SetActorEnableCollision(false);}if(IsValid(O.Marker))Cast<AStaticMeshActor>(O.Marker)->GetStaticMeshComponent()->SetMaterial(0,Material(Teal,true));
 Interact=Defense=0;Notify(FString::Printf(TEXT("임무 완료  %d / 7  ·  자동 저장됨"),Completed));Sound(3,O.Position,.5);Save();if(O.Kind==0&&Playtest&&FParse::Param(FCommandLine::Get(),TEXT("stop-after-escort-complete")))GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
}
void AExtractionMode::Tick(float Dt){
 Super::Tick(Dt);if(!Hero)return;auto* PC=GetWorld()->GetFirstPlayerController();float D=FMath::Min(Dt,.05f);
 if(Playtest)UpdatePlaytest(D);
 auto Press=[this,PC](FKey K){return Playtest?TestPressed.Contains(K):PC->WasInputKeyJustPressed(K);};auto Down=[this,PC](FKey K){return Playtest?TestHeld.Contains(K):PC->IsInputKeyDown(K);};
 if(Press(EKeys::F5)){MusicVolume=MusicVolume>=.99?0:MusicVolume+.25;Notify(FString::Printf(TEXT("음악 음량 %d%%"),int(MusicVolume*100)),2);}
 if(Press(EKeys::F6)){EffectsVolume=EffectsVolume>=.99?0:EffectsVolume+.25;if(EffectsVolume>1)EffectsVolume=1;Notify(FString::Printf(TEXT("효과음 음량 %d%%"),int(EffectsVolume*100)),2);}
 int MusicState=Menu||Victory||Ending?3:Alarm>35?1:Chapter>=3?2:0;
 for(int i=0;i<MusicLayers.Num();i++)if(MusicLayers[i]){MusicGains[i]=FMath::FInterpTo(MusicGains[i],i==MusicState?1.f:0.f,D,.8f);MusicLayers[i]->SetVolumeMultiplier(FMath::Max(.0001f,MusicGains[i]*MusicVolume*.75f));}
if(AmbiencePlayer)AmbiencePlayer->SetVolumeMultiplier(EffectsVolume*.5);
 if(Ending){UpdateExtraction(D);return;}
 if(!Menu&&!Paused){UpdateWeaponDrops(D);if(UpdateFallPreview(D))return;}
 if(Press(EKeys::Escape)&&!Menu&&!Dead&&!Victory){Paused=!Paused;}
 if(Menu){int VX,VY;PC->GetViewportSize(VX,VY);float MX,MY;PC->GetMousePosition(MX,MY);float UI=FMath::Clamp(VX/1600.f,.65f,1.4f);bool Button=Press(EKeys::LeftMouseButton)&&MX>72*UI&&MX<502*UI&&MY>VY-258*UI&&MY<VY-198*UI;if(Press(EKeys::Escape)){PC->ConsoleCommand(TEXT("quit"));return;}if(Button){Start(false);return;}if(Press(EKeys::One))Difficulty=0;if(Press(EKeys::Two))Difficulty=1;if(Press(EKeys::Three))Difficulty=2;if(Press(EKeys::Enter))Start(false);if(Press(EKeys::C)&&HasSave)Start(true);return;}
 if(Dead){if(Press(EKeys::Enter)){Dead=false;BuildChapter(SavedMask,true);}return;}
 if(Victory){if(Playtest){UE_LOG(LogTemp,Display,TEXT("ENEMY_TACTICS_SUMMARY transitions=%d covers=%d flanks=%d searches=%d"),EnemyTacticTransitions,EnemyCoverSelections,EnemyFlanks,EnemySearches);UE_LOG(LogTemp,Display,TEXT("ROUTE_PLAYTEST_PASS elapsed=%.1f deaths=%d kills=%d"),Elapsed,TestDeaths,Kills);PC->ConsoleCommand(TEXT("quit"));return;}if(Press(EKeys::Enter)){Menu=true;ReadSave();}return;}
 if(Paused){if(Press(EKeys::Q)){Save();Menu=true;Paused=false;}return;}
 if(Press(EKeys::Tab))Map=!Map;
 Elapsed+=D;LocalTime+=D;UpdateSector(D);ToastTime-=D;FireCD-=D;HitFlash-=D;DashCD-=D;Hero->Invincible-=D;Alarm=FMath::Max(0.f,Alarm-D*1.7);
 if(FParse::Param(FCommandLine::Get(),TEXT("wardrobe-preview"))){
  if(LocalTime>3&&LocalTime<5){auto Advance=[&](AActor* Person){FVector P=Person->GetActorLocation()+FVector(145*D,0,0);P.Z=GroundHeight(P)+100;Person->SetActorLocation(P);};Advance(Hero);for(auto* Person:Followers)Advance(Person);}
  int PreviewRole=-1;FParse::Value(FCommandLine::Get(),TEXT("PreviewRole="),PreviewRole);
  FVector Target=Hero->GetActorLocation()+FVector(0,450,-5),Offset(1700,-180,470);float Fov=50;
  if(Followers.IsValidIndex(PreviewRole)){Target=Followers[PreviewRole]->GetActorLocation();Offset=FVector(540,-310,150);Fov=30;}
  Hero->Camera->SetWorldLocation(Target+Offset);Hero->Camera->SetWorldRotation((-Offset).Rotation());Hero->Camera->FieldOfView=Fov;
  if(LocalTime>8&&LocalTime<8+D*1.5f)FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/characters_06/roles-in-engine-%d.png"),PreviewRole),false,false);
  if(LocalTime>10){UE_LOG(LogTemp,Display,TEXT("WARDROBE_PREVIEW_COMPLETE roles=%d"),Followers.Num()+1);PC->ConsoleCommand(TEXT("quit"));}return;
 }
 if(Reload>0){Reload-=D;if(Reload<=0){int N=FMath::Min(30-Ammo,Reserve);Ammo+=N;Reserve-=N;Notify(TEXT("재장전 완료"),1);}}
 if(Press(EKeys::One))Weapon=0;if(Press(EKeys::Two))Weapon=1;for(auto* P:Hero->RifleParts)P->SetVisibility(Weapon==0);for(auto* P:Hero->BowParts)P->SetVisibility(Weapon==1);
 if(Press(EKeys::R)&&Ammo<30&&Reserve>0&&Reload<=0){Reload=1.7;CharacterAction(Hero,TEXT("Reload"));Notify(TEXT("재장전 중…"),1.7);}
 if(Press(EKeys::G))Grenade();if(Press(EKeys::Q))UseMedkit();
 FVector Move=FVector::ZeroVector;if(Down(EKeys::W)||Down(EKeys::Up))Move.X+=1;if(Down(EKeys::S)||Down(EKeys::Down))Move.X-=1;if(Down(EKeys::A)||Down(EKeys::Left))Move.Y-=1;if(Down(EKeys::D)||Down(EKeys::Right))Move.Y+=1;Move=Move.GetSafeNormal();
 if(Press(EKeys::RightMouseButton)){FVector R,V;if(PC->DeprojectMousePositionToWorld(R,V)&&FMath::Abs(V.Z)>.001){MoveTarget=R+V*((Hero->GetActorLocation().Z-R.Z)/V.Z);for(int I=0;I<3;I++)MoveTarget=R+V*((GroundHeight(MoveTarget)+100-R.Z)/V.Z);ClickMoving=FindRoute(MoveTarget);if(ClickMoving&&Route.Num())MoveTarget=Route.Last();}}
 if(!Move.IsNearlyZero())ClickMoving=false;
 if(ClickMoving){if(FVector::Dist2D(Hero->GetActorLocation(),MoveTarget)<65)ClickMoving=false;else Move=FollowRoute();}
 if(Playtest)Move=TestMove;
 bool RigPreview=FParse::Param(FCommandLine::Get(),TEXT("rig-preview"));
 int RigStage=FMath::Max(0,int((LocalTime-2)/2));
 if(RigPreview){
  Hero->Invincible=100;Move=FVector::ZeroVector;
  if(RigStage==1||RigStage==2)Move=FVector(1,0,0);
  if(RigStage==3)Move=FVector(0,1,0);if(RigStage==4)Move=FVector(-1,0,0);if(RigStage==5)Move=FVector(0,-1,0);
  if(RigStage==6&&FMath::Fmod(LocalTime,.18f)<D){CharacterAction(Hero,TEXT("Fire"));if(Enemies.Num())CharacterAction(Enemies.Last().Actor,TEXT("Fire"));}
  if(RigStage!=RigLastStage){RigLastStage=RigStage;const TCHAR* Action=RigStage==7?TEXT("Reload"):RigStage==8?TEXT("Hit"):RigStage==9?TEXT("Dodge"):RigStage==10?TEXT("Death"):TEXT("");if(*Action){CharacterAction(Hero,Action);if(Enemies.Num())CharacterAction(Enemies.Last().Actor,Action);}UE_LOG(LogTemp,Display,TEXT("RIG_PREVIEW_STAGE %d action=%s bones=%d"),RigStage,Action,Hero->CharacterMesh->GetNumBones());}
 }

 bool Sprint=Down(EKeys::LeftShift)&&Hero->Stamina>2&&Move.SizeSquared()>0;float Speed=Sprint?680:430;if(RigPreview)Speed=RigStage==2?355:145;
 Hero->Stamina=FMath::Clamp(Hero->Stamina+D*(Sprint?-20:15),0.f,100.f);
 if(Press(EKeys::SpaceBar)&&DashCD<=0&&Move.SizeSquared()>0&&Hero->Stamina>=25){Hero->Stamina-=25;Hero->Invincible=.42;DashCD=1.5;DashTime=.35;DashDirection=Move;CharacterAction(Hero,TEXT("Dodge"));}
 FVector BeforeMove=Hero->GetActorLocation();
 if(DashTime>0){float Step=FMath::Min(D,DashTime);DashTime-=Step;FVector Dash=DashDirection*(310.f/.35f)*Step;if(TerrainSegmentClear(Hero->GetActorLocation(),Hero->GetActorLocation()+Dash))MoveOnTerrain(Hero,Dash);}
 if(!TerrainSegmentClear(Hero->GetActorLocation(),Hero->GetActorLocation()+Move*Speed*D)){FVector X(Move.X,0,0),Y(0,Move.Y,0);Move=TerrainSegmentClear(Hero->GetActorLocation(),Hero->GetActorLocation()+X*Speed*D)?X:TerrainSegmentClear(Hero->GetActorLocation(),Hero->GetActorLocation()+Y*Speed*D)?Y:FVector::ZeroVector;}
 FHitResult MoveHit;MoveOnTerrain(Hero,Move*Speed*D,&MoveHit);if(Playtest&&MoveHit.bBlockingHit&&FMath::Fmod(LocalTime,10.f)<D){UE_LOG(LogTemp,Warning,TEXT("ROUTE_BLOCKED pos=%s move=%s actor=%s normal=%s next=%s"),*Hero->GetActorLocation().ToCompactString(),*Move.ToCompactString(),*GetNameSafe(MoveHit.GetActor()),*MoveHit.Normal.ToCompactString(),Route.Num()?*Route[0].ToCompactString():TEXT("none"));}if(MoveHit.bBlockingHit){FVector Slide=FVector::VectorPlaneProject(Move*Speed*D*(1-MoveHit.Time),MoveHit.Normal.GetSafeNormal2D());Slide.Z=0;MoveOnTerrain(Hero,Slide);}
 FVector HP=Hero->GetActorLocation();if(!TerrainSegmentClear(BeforeMove,HP))HP=BeforeMove;HP.X=FMath::Clamp(HP.X,-9900.,9900.);HP.Y=FMath::Clamp(HP.Y,-7000.,7000.);HP.Z=GroundHeight(HP)+100;Hero->SetActorLocation(HP);for(auto* Tree:Canopies)if(IsValid(Tree))Tree->SetActorHiddenInGame(FMath::PointDistToSegment(Tree->GetActorLocation(),Hero->Camera->GetComponentLocation(),HP+FVector(0,0,100))<620);Hero->Animate(LocalTime,Move.Size()*Speed);
 UpdateCompanions(D);UpdateEscorts(D);
 FVector Ray,Dir;if(PC->DeprojectMousePositionToWorld(Ray,Dir)&&FMath::Abs(Dir.Z)>.001){float T=(HP.Z+12-Ray.Z)/Dir.Z;Aim=Ray+Dir*T;for(int I=0;I<3;I++)Aim=Ray+Dir*((GroundHeight(Aim)+112-Ray.Z)/Dir.Z);}
 if(Playtest)Aim=TestAim;
 if(FParse::Param(FCommandLine::Get(),TEXT("record-combat"))){
  // Keep both participants in frame; this changes only the recording camera.
  FVector Focus=((LocalTime<CombatAttentionUntil?CombatAttention:Aim)-HP)*.45f;Focus.Z=0;Focus=Focus.GetClampedToMaxSize(330);
  const FVector Offset(-1600,-120,2200);
  Hero->Camera->SetRelativeLocation(FMath::VInterpTo(Hero->Camera->GetRelativeLocation(),Offset+Focus,D,3));
  Hero->Camera->SetRelativeRotation((-Offset).Rotation());Hero->Camera->FieldOfView=62;
 }
 if(RigPreview||FParse::Param(FCommandLine::Get(),TEXT("capture-character")))Aim=HP+FVector(1000,0,12);
 Hero->Visual->SetWorldRotation(FMath::RInterpTo(Hero->Visual->GetComponentRotation(),(Aim-HP).GetSafeNormal2D().Rotation(),D,14));
 if(Down(EKeys::LeftMouseButton))Shoot();
 for(int i=Thrown.Num()-1;i>=0;i--){auto& T=Thrown[i];T.Time+=D;float A=FMath::Clamp(T.Time/.9f,0.f,1.f);if(IsValid(T.Actor))T.Actor->SetActorLocation(FMath::Lerp(T.Origin,T.Target,A)+FVector(0,0,FMath::Sin(A*PI)*340));if(A>=1){Explode(T.Target);if(IsValid(T.Actor))T.Actor->Destroy();Thrown.RemoveAtSwap(i);}}
 for(int i=Blasts.Num()-1;i>=0;i--){auto& B=Blasts[i];B.Age+=D;if(!IsValid(B.Actor)||B.Age>=2.6f){if(IsValid(B.Actor))B.Actor->Destroy();Blasts.RemoveAtSwap(i);continue;}B.Material->SetScalarParameterValue(TEXT("Progress"),B.Age/2.6f);B.Actor->SetActorRotation(FRotationMatrix::MakeFromZX((Hero->Camera->GetComponentLocation()-B.Actor->GetActorLocation()).GetSafeNormal(),Hero->Camera->GetRightVector()).Rotator());}
 // Predictable projectiles make incoming fire readable and dodgeable.
 for(int i=Shots.Num()-1;i>=0;i--){auto& S=Shots[i];FVector Old=S.Position;S.Position+=S.Velocity*D;S.Life-=D;FHitResult H;FCollisionQueryParams Q;Q.AddIgnoredActor(Hero);for(auto& E:Enemies)if(IsValid(E.Actor))Q.AddIgnoredActor(E.Actor);bool Block=GetWorld()->LineTraceSingleByChannel(H,Old,S.Position,ECC_Visibility,Q);
  const FVector NearestHero=FMath::ClosestPointOnSegment(HP+FVector(0,0,10),Old,S.Position);
  const bool ReachesHero=!Block||FVector::DistSquared(Old,NearestHero)<FVector::DistSquared(Old,H.ImpactPoint);
  if(ReachesHero&&FVector::Dist(HP+FVector(0,0,10),NearestHero)<44){DamageHero(S.Damage,S.Velocity);S.Life=0;}
  if(S.Life<=0||Block){if(IsValid(S.Visual))S.Visual->Destroy();Shots.RemoveAtSwap(i);}else if(IsValid(S.Visual))S.Visual->SetActorLocation(S.Position);
 }
 UpdateEnemies(D,Move*Speed,RigPreview);
 Focused=-1;float Nearest=1e9;for(int i=0;i<Objectives.Num();i++){if(Objectives[i].Done)continue;float Dist=FVector::Dist2D(HP,Objectives[i].Position);if(Dist<Nearest){Nearest=Dist;Focused=i;}}const int EscortIndex=ActiveEscort();if(EscortIndex>=0){Focused=EscortIndex;Nearest=FVector::Dist2D(HP,Objectives[Focused].Position);}
 Hint=TEXT("");
 if(Focused>=0){auto& O=Objectives[Focused];if(Nearest<(O.Kind==3?650:350)){Hint=O.Description;bool Nearby=false;for(auto& E:Enemies)if(IsValid(E.Actor)&&E.Health>0&&FVector::Dist2D(E.Actor->GetActorLocation(),O.Position)<680)Nearby=true;
  if(O.Kind==5&&(!Objectives[1].Done||!Objectives[2].Done)){Hint=TEXT("기관사를 구출하고 교량 경비 전력을 차단하십시오");Interact=0;}else if(O.Kind==4){Interact=0;}else if(O.Kind==3){if(Press(EKeys::E)&&Defense<=0){Defense=.01;Notify(TEXT("교신 시작 · 무전기 주변에서 방어하십시오"),4);}
   if(Defense>0&&Nearest<650){float Before=Defense;Defense+=D;if(int(Before/12)!=int(Defense/12)){for(int k=0;k<4;k++){float A=Random.FRandRange(0,2*PI);SpawnEnemy(O.Position+FVector(FMath::Cos(A)*1500,FMath::Sin(A)*1500,95),k%3==0?1:0);Enemies.Last().Alert=20;Enemies.Last().LastKnown=Hero->GetActorLocation();Enemies.Last().HasContact=true;}}if(Defense>=50)CompleteObjective(Focused);}
  }else if(Down(EKeys::E)){if(Nearby){Hint=TEXT("주변 경비병을 먼저 제압하십시오");Interact=0;}else if(O.Kind==0&&O.Stage==1&&!EscortReady(Focused)){Hint=O.EscortUnderFire?TEXT("동료가 적의 사격을 피해 멈췄습니다 · 가까이에서 엄호하십시오"):TEXT("보급 지점까지 동료를 데려오십시오");Interact=0;}else{Interact+=D;if(Interact>=2.2){if(O.Kind==0&&O.Stage==0)BeginEscort(Focused);else CompleteObjective(Focused);}}}else Interact=0;
 }else{Interact=0;if(Defense>0){Hint=TEXT("무전기에서 멀어졌습니다 · 돌아오면 방어가 계속됩니다");}}
 }
 if(EscortIndex>=0&&Hint.IsEmpty()){const auto& E=Objectives[EscortIndex];Hint=E.EscortUnderFire?TEXT("동료가 적의 사격을 피해 멈췄습니다 · 가까이에서 엄호하십시오"):TEXT("초록색 표식의 보급 지점으로 동료를 이끄십시오");}
 if(Completed==7){Hint=TEXT("모든 임무 완료 · 지도에 표시된 헬기장으로 이동해 E 키");if(FVector::Dist2D(HP,Exit)<500&&Press(EKeys::E)){if(Chapter==5){BeginExtraction();}else{Chapter++;BuildChapter();Save();}}}
 UpdateEscortRecording(D);
 if(RigPreview){
  if(LocalTime>=2&&LocalTime<24){FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/rigging/frames/frame_%05d.png"),RigFrame++),false,false);}
  if(LocalTime>=24){UE_LOG(LogTemp,Display,TEXT("RIG_PREVIEW_COMPLETE frames=%d"),RigFrame);PC->ConsoleCommand(TEXT("quit"));}
 }else if(FParse::Param(FCommandLine::Get(),TEXT("site-preview"))){
  if(LocalTime>8&&LocalTime<8+D*1.5){int Node=0;FParse::Value(FCommandLine::Get(),TEXT("PreviewNode="),Node);FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/props_08/site_%d_%d.png"),Chapter,Node),true,false);}
  if(LocalTime>10)PC->ConsoleCommand(TEXT("quit"));
 }else if(FParse::Param(FCommandLine::Get(),TEXT("capture-preview"))){if(LocalTime>8&&LocalTime<8+D*1.5){FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/(FParse::Param(FCommandLine::Get(),TEXT("capture-landmark"))?FString::Printf(TEXT("../../work/world_revision/props_%d.png"),Chapter):FParse::Param(FCommandLine::Get(),TEXT("capture-overview"))?FString::Printf(TEXT("../../work/world_revision/overview_%d.png"),Chapter):FParse::Param(FCommandLine::Get(),TEXT("capture-character"))?TEXT("../../work/revision2/CharacterInEngine.png"):(FParse::Param(FCommandLine::Get(),TEXT("capture-props"))?TEXT("../EquipmentInEngine.png"):(FParse::Param(FCommandLine::Get(),TEXT("capture-bridge"))?TEXT("../../work/campaign_03/bridge-preview.png"):TEXT("../Gameplay.png")))),true,false);}if(LocalTime>10)PC->ConsoleCommand(TEXT("quit"));}
 if(CombatRecordStart>=0&&FParse::Param(FCommandLine::Get(),TEXT("record-combat"))){
  if(LocalTime-CombatRecordStart<22){FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/(FParse::Param(FCommandLine::Get(),TEXT("record-allies"))?TEXT("../../work/world_revision/allies"):TEXT("../../work/gameplay_capture"))/FString::Printf(TEXT("frame_%05d.png"),CombatFrame++),true,false);}
  else{UE_LOG(LogTemp,Display,TEXT("COMBAT_RECORD_COMPLETE frames=%d health=%.1f ammo=%d kills=%d"),CombatFrame,Hero->Health,Ammo,Kills);PC->ConsoleCommand(TEXT("quit"));}
 }
 // Optional deterministic integration test: uses the same combat / objective / save paths.
 if(Autotest&&LocalTime>2){
  if(Chapter==0){
   Hero->SetActorLocation(FVector(-8400,0,100));SpawnEnemy(FVector(-7600,0,100));int TestIndex=Enemies.Num()-1;Weapon=0;Ammo=30;Reload=0;Aim=FVector(-7600,0,112);
   for(int k=0;k<3;k++){FireCD=0;Shoot();}
   check(Enemies[TestIndex].Health<=0);check(Ammo==27);UE_LOG(LogTemp,Display,TEXT("COMBAT_TEST_PASS hitscan=3 enemy_killed=true ammo=27"));
   Hero->Invincible=0;Hero->Health=100;DamageHero(20);check(Hero->Health<100);Medkits=2;UseMedkit();check(Hero->Health==100);check(Medkits==1);UE_LOG(LogTemp,Display,TEXT("HEAL_TEST_PASS damage_and_medkit=true"));
  }
  int Alive=0;for(auto& E:Enemies)if(IsValid(E.Actor))Alive++;
  UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_TEST chapter=%d live=%d"),Chapter,Alive);
  for(int i=0;i<Enemies.Num();i++){Enemies[i].Health=0;KillEnemy(i);}for(int i=0;i<Objectives.Num();i++)CompleteObjective(i);
  check(Completed==7);ReadSave();check(SavedMask==127);check(SavedChapter==Chapter);
  if(Chapter<5){Chapter++;BuildChapter();Save();}else{UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_TEST_PASS six_chapters objectives=42 save_load=PASS"));GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));}
 }
}
void AExtractionHUD::Text(const FString& S,float X,float Y,float Size,FLinearColor C){
 if(!KoreanFont){KoreanFont=NewObject<UFont>(this);KoreanFont->FontCacheType=EFontCacheType::Runtime;KoreanFont->LegacyFontSize=20;KoreanFont->CompositeFont=FCompositeFont(FName(TEXT("Regular")),FPaths::ProjectContentDir()/TEXT("Fonts/NanumGothic.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);}
 FCanvasTextItem Item(FVector2D(X,Y),FText::FromString(S),KoreanFont,C);Item.SlateFontInfo=FSlateFontInfo(KoreanFont,FMath::RoundToInt(Size));Item.EnableShadow(FLinearColor(0,0,0,.7),FVector2D(1,1));Canvas->DrawItem(Item);
}
void AExtractionHUD::Box(float X,float Y,float W,float H,FLinearColor C){FCanvasTileItem T(FVector2D(X,Y),FVector2D(W,H),C);T.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(T);}
void AExtractionHUD::Image(UTexture2D* T,float X,float Y,float W,float H,float U,float V,float UW,float VH){if(!T)return;DrawTexture(T,X,Y,W,H,U,V,UW,VH,FLinearColor::White,BLEND_Translucent);}
void AExtractionHUD::DrawHUD(){
 Super::DrawHUD();if(FParse::Param(FCommandLine::Get(),TEXT("capture-landmark"))||FParse::Param(FCommandLine::Get(),TEXT("wardrobe-preview"))||FParse::Param(FCommandLine::Get(),TEXT("fall-preview")))return;auto* G=GM(GetWorld());if(FParse::Param(FCommandLine::Get(),TEXT("rig-preview"))&&G&&Canvas){const TCHAR* Names[]={TEXT("대기 · 호흡"),TEXT("걷기"),TEXT("달리기"),TEXT("오른쪽 이동"),TEXT("뒷걸음"),TEXT("왼쪽 이동"),TEXT("사격 반동"),TEXT("재장전"),TEXT("피격"),TEXT("회피"),TEXT("쓰러짐")};Box(22,22,410,91,FLinearColor(.005,.02,.015,.8));Text(TEXT("주인공 · 경비병 / 동작 검토"),40,34,20,Teal);Text(Names[FMath::Clamp(G->RigLastStage,0,10)],40,70,25,FLinearColor::White);return;}if(!G||!G->Hero||!Canvas)return;float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Clamp(W/1600.f,.65f,1.4f);FLinearColor White(.89,.92,.85),Muted(.47,.61,.55);
 if(G->Ending){
  Box(0,0,W,H*.10,FLinearColor::Black);Box(0,H*.90,W,H*.10,FLinearColor::Black);
  FString Line=G->EndingTime<7?TEXT("항공팀: 착륙한다. 모두 탑승해!"):G->EndingTime<22?FString::Printf(TEXT("마지막 한 사람까지 · %d / %d명 탑승"),G->Boarded,G->ExtractionParty.Num()):TEXT("전원 탑승 확인. 기지로 귀환한다.");
  Text(Line,W*.25,H*.92,20*S,White);return;
 }
 if(G->Menu){
  Image(G->KeyArt,0,0,W,H);Box(0,0,W*.55,H,FLinearColor(.005,.018,.014,.60));Box(70*S,80*S,42*S,4*S,Teal);
  Text(TEXT("정글 구출 작전"),70*S,108*S,18*S,Teal);Text(TEXT("LAST"),64*S,160*S,78*S,White);Text(TEXT("EXTRACTION"),64*S,245*S,66*S,White);
  Text(TEXT("마지막 한 사람까지, 살아서 돌아온다."),72*S,354*S,20*S,White);
  Text(TEXT("람보 고전 게임에서 영감을 받은 3D 액션"),72*S,397*S,13*S,Muted);
  Box(72*S,H-258*S,430*S,60*S,FLinearColor(.06,.22,.17,.9));Text(TEXT("ENTER     새 작전 시작"),96*S,H-242*S,22*S,White);
  if(G->HasSave)Text(TEXT("C             저장한 작전 계속"),96*S,H-178*S,19*S,Teal);
  Text(TEXT("1  쉬움     2  보통     3  어려움"),72*S,H-118*S,15*S,Muted);
  Text(FString::Printf(TEXT("선택: %s   /   6개 지역 · 42개 임무"),G->Difficulty==0?TEXT("쉬움"):G->Difficulty==1?TEXT("보통"):TEXT("어려움")),72*S,H-84*S,14*S,White);
  Text(TEXT("개발용 플레이 빌드  0.9   ·   Unreal Engine 5.8"),72*S,H-42*S,11*S,Muted);return;
 }
 if(G->HitFlash>0)Box(0,0,W,H,FLinearColor(.65,.04,.02,G->HitFlash*.55));
 Box(24*S,22*S,460*S,100*S,FLinearColor(.005,.025,.02,.83));Box(24*S,22*S,4*S,100*S,Teal);
 Text(G->ChapterNames[G->Chapter],42*S,34*S,20*S,White);Text(FString::Printf(TEXT("임무 %d / 7   ·   경계 %d%%   ·   %02d:%02d"),G->Completed,int(G->Alarm),int(G->Elapsed)/60,int(G->Elapsed)%60),42*S,70*S,13*S,Muted);
 if(G->Followers.Num())Text(FString::Printf(TEXT("지원 동료 %d명 · 자동 교전"),G->Followers.Num()),42*S,126*S,12*S,Teal);
 if(G->Focused>=0){auto& O=G->Objectives[G->Focused];Text(FString::Printf(TEXT("%s  ·  %dm"),*O.Name,int(FVector::Dist2D(G->Hero->GetActorLocation(),O.Position)/100)),42*S,94*S,13*S,Amber);}
 for(auto& E:G->Enemies){if(!IsValid(E.Actor)||E.Alert<=0)continue;FVector2D P;if(GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(E.Actor->GetActorLocation()+FVector(0,0,110),P)&&P.X>0&&P.X<W&&P.Y>0&&P.Y<H){float Max=E.Type==3?580:E.Type==2?150:80;Box(P.X-22*S,P.Y,44*S,4*S,FLinearColor(.06,.06,.04,.9));Box(P.X-22*S,P.Y,44*S*E.Health/Max,4*S,FLinearColor(.9,.21,.08));}}
 for(int I=0;I<G->Followers.Num();I++)if(IsValid(G->Followers[I])){FVector2D P;if(GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(G->Followers[I]->GetActorLocation()+FVector(0,0,105),P)&&P.X>0&&P.X<W&&P.Y>0&&P.Y<H)Text(FString::Printf(TEXT("동료 %d"),I+1),P.X-20*S,P.Y-13*S,11*S,Teal);}
 const int EscortIndex=G->ActiveEscort();if(EscortIndex>=0){auto& O=G->Objectives[EscortIndex];if(IsValid(O.Prop)){FVector2D P;if(GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(O.Prop->GetActorLocation()+FVector(0,0,105),P))Text(O.EscortUnderFire?TEXT("엄호 필요"):TEXT("구출 동료"),P.X-32*S,P.Y-13*S,12*S,Teal);Text(FString::Printf(TEXT("동료와 거리 %dm · 보급 지점까지 엄호"),int(FVector::Dist2D(O.Prop->GetActorLocation(),G->Hero->GetActorLocation())/100)),42*S,(G->Followers.Num()?148:126)*S,12*S,Teal);}}
 // Nearest objective always has a screen-space marker, clamped to the edge.
 FVector Target=G->Focused>=0?G->Objectives[G->Focused].Position:G->Exit;FVector2D Screen;auto* PC=GetWorld()->GetFirstPlayerController();if(PC->ProjectWorldLocationToScreen(Target+FVector(0,0,160),Screen)){
  Screen.X=FMath::Clamp(Screen.X,45.f*S,W-45*S);Screen.Y=FMath::Clamp(Screen.Y,160.f*S,H-160*S);Box(Screen.X-6*S,Screen.Y-6*S,12*S,12*S,EscortIndex>=0?Teal:Amber);Text(FString::Printf(TEXT("%dm"),int(FVector::Dist2D(G->Hero->GetActorLocation(),Target)/100)),Screen.X+14*S,Screen.Y-10*S,13*S,Amber);
 }
 // Health / stamina / ammunition panel.
 Box(24*S,H-124*S,370*S,100*S,FLinearColor(.005,.025,.02,.86));Text(FString::Printf(TEXT("체력  %03d"),int(G->Hero->Health)),42*S,H-113*S,17*S,White);Box(42*S,H-78*S,240*S,7*S,FLinearColor(.15,.18,.16));Box(42*S,H-78*S,240*S*G->Hero->Health/100,7*S,G->Hero->Health>30?Teal:Amber);Box(42*S,H-61*S,240*S*G->Hero->Stamina/100,3*S,Amber);Text(FString::Printf(TEXT("Q  구급 %d     G  수류탄 %d"),G->Medkits,G->Grenades),42*S,H-47*S,12*S,Muted);
 Box(W-350*S,H-124*S,326*S,100*S,FLinearColor(.005,.025,.02,.86));Image(G->Equipment,W-342*S,H-116*S,80*S,80*S,0,G->Weapon==1?.5:0,.5,.5);Text(G->Weapon==0?TEXT("1  소총"):TEXT("2  무소음 활"),W-246*S,H-110*S,17*S,White);Text(G->Reload>0?TEXT("재장전 중"):FString::Printf(TEXT("%02d  /  %03d"),G->Weapon==0?G->Ammo:G->Arrows,G->Weapon==0?G->Reserve:0),W-246*S,H-77*S,25*S,White);
 Text(TEXT("WASD / 우클릭 이동   좌클릭 사격   1/2 무기   R 장전   E 상호작용   Shift 달리기   Space 회피   Tab 지도   Esc 일시정지"),25*S,H-19*S,10*S,Muted);
 if(!G->Hint.IsEmpty()){Box(W*.25,H-188*S,W*.5,42*S,FLinearColor(.005,.02,.016,.88));Text(G->Hint,W*.25+16*S,H-180*S,14*S,Amber);}
 if(G->Interact>0)Box(W*.35,H-139*S,W*.3*(G->Interact/2.2),6*S,Teal);
 if(G->Defense>0){Text(FString::Printf(TEXT("무전기 방어   %02d / 50초"),int(G->Defense)),W*.40,145*S,20*S,Teal);Box(W*.35,181*S,W*.3*(G->Defense/50),5*S,Teal);}
 if(G->ToastTime>0){Box(W*.24,133*S,W*.52,83*S,FLinearColor(.005,.02,.016,.87));Text(G->Toast,W*.24+18*S,147*S,15*S,White);}
 // Compact tactical map. North is to the right in world space; player camera faces north.
 float MW=G->Map?W*.66:210*S,MH=G->Map?H*.63:155*S,MX=G->Map?(W-MW)*.5:W-MW-24*S,MY=G->Map?H*.18:24*S;
 Box(MX,MY,MW,MH,FLinearColor(.015,.06,.048,.94));for(int i=1;i<6;i++){Box(MX+i*MW/6,MY,1,MH,FLinearColor(.08,.17,.12,.6));Box(MX,MY+i*MH/6,MW,1,FLinearColor(.08,.17,.12,.6));}
 auto MapPoint=[&](FVector P){return FVector2D(MX+(P.Y+7500)/15000*MW,MY+(10500-P.X)/21000*MH);};
 auto MapLine=[&](FVector A,FVector B,FLinearColor Color,float Thickness){auto P=MapPoint(A),Q=MapPoint(B);DrawLine(P.X,P.Y,Q.X,Q.Y,Color,Thickness);};
 if(G->Chapter==1){for(int Y=-7000;Y<7000;Y+=250)MapLine(FVector(G->RiverCenter(Y),Y,0),FVector(G->RiverCenter(Y+250),Y+250,0),FLinearColor(.08,.25,.27),MW*.15);MapLine(FVector(-1800,-2800,0),FVector(1800,-2800,0),G->BridgeOpen?Teal:Amber,G->Map?5:2);}
 if(G->Chapter==3){for(int I=0;I<40;I++){float A=I*PI/20,B=(I+1)*PI/20;MapLine(FVector(400+3200*FMath::Cos(A),2700*FMath::Sin(A),0),FVector(400+3200*FMath::Cos(B),2700*FMath::Sin(B),0),FLinearColor(.40,.30,.15),G->Map?4:2);}}
 if(G->Chapter==5)MapLine(FVector(-9000,0,0),FVector(9000,0,0),FLinearColor(.25,.28,.26),MW*.14);
 if(G->Chapter==2){for(float X:{-7600.f,-4000.f,0.f,4000.f,7600.f})MapLine(FVector(X,-5600,0),FVector(X,6000,0),FLinearColor(.28,.28,.16),G->Map?5:2);for(float Y:{-4900.f,-1800.f,1900.f,5200.f})MapLine(FVector(-8000,Y,0),FVector(8000,Y,0),FLinearColor(.28,.28,.16),G->Map?5:2);}
 if(G->Chapter==4){for(float X:{-4800.f,4800.f})MapLine(FVector(X,-5600,0),FVector(X,6300,0),FLinearColor(.40,.40,.32),G->Map?5:2);for(float Y:{-5700.f,-600.f,3000.f,6800.f})MapLine(FVector(-4800,Y,0),FVector(4800,Y,0),FLinearColor(.40,.40,.32),G->Map?4:2);}
 auto Dot=[&](FVector P,FLinearColor C,float R){float X=MX+(P.Y+7500)/15000*MW,Y=MY+(10500-P.X)/21000*MH;Box(X-R,Y-R,R*2,R*2,C);};
 for(auto& O:G->Objectives){Dot(O.Position,O.Done||O.Stage==1?Teal:Amber,G->Map?6:3);if(!O.Done&&O.Stage==1&&IsValid(O.Prop))Dot(O.Prop->GetActorLocation(),White,G->Map?4:2);}Dot(G->Exit,Teal,4);Dot(G->Hero->GetActorLocation(),White,4);
 if(G->Map){Text(TEXT("작전 지도   ·   Tab 닫기"),MX+18,MY+12,20*S,White);for(auto& E:G->Enemies)if(IsValid(E.Actor)&&FVector::Dist2D(E.Actor->GetActorLocation(),G->Hero->GetActorLocation())<2000)Dot(E.Actor->GetActorLocation(),FLinearColor(.9,.2,.1),3);}
 float MouseX,MouseY;if(PC->GetMousePosition(MouseX,MouseY)){DrawLine(MouseX-9,MouseY,MouseX-3,MouseY,Teal,1.4);DrawLine(MouseX+3,MouseY,MouseX+9,MouseY,Teal,1.4);DrawLine(MouseX,MouseY-9,MouseX,MouseY-3,Teal,1.4);DrawLine(MouseX,MouseY+3,MouseX,MouseY+9,Teal,1.4);}
 if(G->Paused||G->Dead||G->Victory){Box(0,0,W,H,FLinearColor(.005,.015,.012,.86));if(G->Paused)Image(G->CharacterArt,W*.68,H*.20,W*.26,H*.62,0,0,.5,1);Text(G->Victory?TEXT("모두 돌아왔다."):G->Dead?TEXT("다시, 정글 속으로."):TEXT("작전 일시정지"),W*.27,H*.32,42*S,White);Text(G->Victory?TEXT("6개 지역의 구출 작전을 완료했습니다."):G->Dead?TEXT("ENTER  마지막 임무 체크포인트에서 재시도"):TEXT("ESC 계속하기   Q 저장하고 시작 화면   F5 음악   F6 효과음"),W*.27,H*.44,20*S,Teal);Text(FString::Printf(TEXT("전투 기록   %d명 제압   ·   %d분 %d초"),G->Kills,int(G->Elapsed)/60,int(G->Elapsed)%60),W*.27,H*.53,17*S,Muted);if(G->Victory)Text(TEXT("ENTER  시작 화면"),W*.27,H*.62,18*S,White);}
}

void AExtractionMode::BlastVisual(FVector P,float Scale){
 auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(P+FVector(0,0,130*Scale),FRotator::ZeroRotator);auto* C=A->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));C->SetCollisionProfileName(TEXT("NoCollision"));C->SetGenerateOverlapEvents(false);A->SetActorEnableCollision(false);C->SetCastShadow(false);A->SetActorScale3D(FVector(6*Scale));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Production/Materials/M_Explosion.M_Explosion"));auto* M=UMaterialInstanceDynamic::Create(Base,this);C->SetMaterial(0,M);FBlastEffect FX;FX.Actor=A;FX.Material=M;Blasts.Add(FX);
}
