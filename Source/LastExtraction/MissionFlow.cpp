#include "Extraction.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Camera/CameraComponent.h"

FVector AExtractionMode::RescueRallyPoint(int Index)const{
 const FVector Points[6][7]={
  {{0,0,0},{-8200,1400,0},{0,0,0},{-1900,6200,0},{0,0,0},{0,0,0},{0,0,0}},
  {{0,0,0},{-8200,1900,0},{0,0,0},{0,0,0},{0,0,0},{8200,4800,0},{0,0,0}},
  {{0,0,0},{-8200,2200,0},{-4100,5900,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
  {{0,0,0},{-7100,3800,0},{0,0,0},{0,0,0},{8100,3200,0},{0,0,0},{0,0,0}},
  {{0,0,0},{0,0,0},{0,0,0},{-3500,4300,0},{0,0,0},{0,0,0},{0,0,0}},
  {{-8100,-1700,0},{0,0,0},{4700,-6000,0},{0,0,0},{0,0,0},{-4800,5900,0},{0,0,0}}
 };
 return Points[FMath::Clamp(Chapter,0,5)][FMath::Clamp(Index,0,6)];
}

void AExtractionMode::BuildEscortSites(){
 if(Playtest){
  uint32 Hash=0;int Count=0;
  for(auto* Actor:WorldActors)if(IsValid(Actor))if(auto* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>()){
   const FString Entry=GetNameSafe(Mesh->GetStaticMesh())+Actor->GetActorTransform().ToHumanReadableString();Hash=FCrc::StrCrc32(*Entry,Hash);Count++;
  }
  UE_LOG(LogTemp,Display,TEXT("SCENERY_LAYOUT chapter=%d meshes=%d hash=%08x"),Chapter,Count,Hash);
 }
 FCollisionQueryParams Query;Query.AddIgnoredActor(Hero);for(const auto& Enemy:Enemies)if(IsValid(Enemy.Actor))Query.AddIgnoredActor(Enemy.Actor);
 const FLinearColor Green(.035,.40,.31);
 for(int I=0;I<Objectives.Num();I++){
  auto& O=Objectives[I];if(O.Kind!=0)continue;
  const FVector Desired=O.Rally;bool Clear=false;
  for(int Ring=0;Ring<10&&!Clear;Ring++)for(int Step=0;Step<(Ring?16:1);Step++){
   FVector P=Desired+FVector(FMath::Cos(Step*PI/8),FMath::Sin(Step*PI/8),0)*(Ring*130);P.Z=GroundHeight(P);
   if(TerrainWalkable(P)&&!GetWorld()->OverlapBlockingTestByChannel(P+FVector(0,0,100),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(60,90),Query)){O.Rally=P;Clear=true;break;}
  }
  ensureAlwaysMsgf(Clear,TEXT("An escort rally point must have a clear arrival area."));
  FVector Crate=O.Rally+FVector(330,190,0);Crate.Z=GroundHeight(Crate);Asset(TEXT("wooden_crate_02"),Crate,FVector(1.2),FRotator(0,-20,0),true);
  Asset(TEXT("FieldRifle"),Crate+FVector(0,0,63),FVector(1),FRotator(0,25,90));
  const TCHAR* ShelterName=Chapter==0?(I==1?TEXT("JungleBivouac08"):TEXT("FieldClinic08")):Chapter==1?(I==1?TEXT("BoatRepair08"):TEXT("FieldClinic08")):Chapter==2?(I==1?TEXT("VillageKitchen08"):TEXT("MarketStall")):Chapter==3?(I==1?TEXT("QuarryWorkbench08"):TEXT("JungleBivouac08")):Chapter==4?TEXT("FieldClinic08"):I==0?TEXT("AirCargoTrolley08"):I==2?TEXT("CommandTable08"):TEXT("FieldClinic08");
  FVector Shelter=O.Rally+FVector(690,670,0);Shelter.Z=GroundHeight(Shelter);Asset(ShelterName,Shelter,FVector(.9),FRotator(0,I%2?25:155,0),true);
  UE_LOG(LogTemp,Display,TEXT("ESCORT_SITE_THEME chapter=%d objective=%d shelter=%s"),Chapter,I,ShelterName);
  O.RallyMarker=Shape(O.Rally+FVector(0,0,8),FVector(1.4,1.4,.035),Green,2,false);
  Cast<AStaticMeshActor>(O.RallyMarker)->GetStaticMeshComponent()->SetMaterial(0,Material(Green,true));O.RallyMarker->SetActorHiddenInGame(true);
  UE_LOG(LogTemp,Display,TEXT("ESCORT_SITE chapter=%d objective=%d origin=%s rally=%s"),Chapter,I,*O.Origin.ToCompactString(),*O.Rally.ToCompactString());
 }
}

int AExtractionMode::ActiveEscort()const{
 for(int I=0;I<Objectives.Num();I++)if(!Objectives[I].Done&&Objectives[I].Kind==0&&Objectives[I].Stage==1)return I;
 return -1;
}

bool AExtractionMode::EscortReady(int I)const{
 if(!Objectives.IsValidIndex(I))return false;const auto& O=Objectives[I];
 return O.Stage==1&&IsValid(O.Prop)&&FVector::Dist2D(O.Prop->GetActorLocation(),O.Rally)<520;
}

void AExtractionMode::BeginEscort(int I,bool Restoring){
 if(!Objectives.IsValidIndex(I))return;auto& O=Objectives[I];if(O.Done||O.Kind!=0||!IsValid(O.Prop))return;
 O.Stage=1;O.Position=O.Rally;O.Description=TEXT("동료와 함께 도착 후 E · 무장시키고 지원 전투 합류");O.EscortRepath=0;O.EscortRoute.Reset();Interact=0;
 if(EscortRecordStart<0&&FParse::Param(FCommandLine::Get(),TEXT("record-escort")))EscortRecordStart=LocalTime;
 if(Restoring&&SavedEscortPositions.IsValidIndex(I)&&!SavedEscortPositions[I].IsNearlyZero()){
  FVector P=SavedEscortPositions[I];if(TerrainWalkable(P)){P.Z=GroundHeight(P)+100;O.Prop->SetActorLocation(P);}
 }
 if(IsValid(O.Marker))O.Marker->SetActorHiddenInGame(true);if(IsValid(O.RallyMarker))O.RallyMarker->SetActorHiddenInGame(false);
 FVector Forward=(O.Origin-O.Rally).GetSafeNormal2D(),Side(-Forward.Y,Forward.X,0);
 const int Count=3+Chapter/2+(Difficulty==2?1:0);
 for(int K=0;K<Count;K++){
  FVector P=O.Rally+Forward*(900+(K%2)*250)+Side*((K-(Count-1)*.5f)*350);SpawnEnemy(P,K==0&&Chapter>=3?2:K%3==0?1:0);Enemies.Last().Alert=12;Enemies.Last().LastKnown=Hero->GetActorLocation();Enemies.Last().HasContact=true;
 }
 Checkpoint=Hero->GetActorLocation();Notify(TEXT("포로 해방 · 추격대를 막으며 초록 표식의 보급 지점까지 엄호하십시오"),6);
 UE_LOG(LogTemp,Display,TEXT("ESCORT_%s chapter=%d objective=%d actor=%s target=%s ambushers=%d rescued=%d"),Restoring?TEXT("RESTORED"):TEXT("RELEASED"),Chapter,I,*O.Prop->GetActorLocation().ToCompactString(),*O.Rally.ToCompactString(),Count,RescuedTotal);
 if(!Restoring){Save();if(Playtest&&FParse::Param(FCommandLine::Get(),TEXT("stop-after-escort-release")))GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));}
}

void AExtractionMode::RecordEscortSound(int Index,FVector Position,float Volume){
 if(EscortRecordStart<0||!FParse::Param(FCommandLine::Get(),TEXT("record-escort")))return;
 FString Event=FString::Printf(TEXT("{\"time\":%.4f,\"sound\":%d,\"volume\":%.3f,\"distance\":%.1f}\n"),LocalTime-EscortRecordStart,Index,Volume,FVector::Dist(Position,Hero->GetActorLocation()));
 FFileHelper::SaveStringToFile(Event,*(FPaths::ProjectDir()/TEXT("../../work/mission_05/record/events.jsonl")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
}

void AExtractionMode::UpdateEscortRecording(float D){
 if(EscortRecordStart<0||!FParse::Param(FCommandLine::Get(),TEXT("record-escort")))return;
 const int I=ActiveEscort();FVector Focus=FVector::ZeroVector;
 if(I>=0&&IsValid(Objectives[I].Prop))Focus=(Objectives[I].Prop->GetActorLocation()-Hero->GetActorLocation())*.45f;
 Focus.Z=0;Focus=Focus.GetClampedToMaxSize(350);const FVector Offset(-1600,-120,2000);
 Hero->Camera->SetRelativeLocation(FMath::VInterpTo(Hero->Camera->GetRelativeLocation(),Offset+Focus,D,3));Hero->Camera->SetRelativeRotation((-Offset).Rotation());Hero->Camera->FieldOfView=62;
 if(LocalTime-EscortRecordStart<38&&(EscortRecordStop<0||LocalTime<EscortRecordStop)){
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/mission_05/record/frame_%05d.png"),EscortFrame++),true,false);
 }else{
  UE_LOG(LogTemp,Display,TEXT("ESCORT_RECORD_COMPLETE frames=%d completed=%d rescued=%d ally_shots=%d"),EscortFrame,EscortRecordStop>=0,RescuedTotal,AllyShots);GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
 }
}

void AExtractionMode::UpdateEscorts(float D){
 const int I=ActiveEscort();if(I<0)return;auto& O=Objectives[I];if(!IsValid(O.Prop))return;
 AActor* Person=O.Prop;FVector P=Person->GetActorLocation(),HP=Hero->GetActorLocation();const float Distance=FVector::Dist2D(P,HP);
 O.EscortUnderFire=false;
 for(const auto& E:Enemies)if(IsValid(E.Actor)&&E.Health>0&&E.Alert>0&&FVector::Dist2D(P,E.Actor->GetActorLocation())<1050&&CanSee(P+FVector(0,0,12),E.Actor->GetActorLocation()+FVector(0,0,12),E.Actor)){O.EscortUnderFire=true;break;}
 if(O.EscortUnderFire&&Distance>650){O.EscortRoute.Reset();O.EscortRepath=0;return;}
 if(Distance<180){O.EscortRoute.Reset();return;}
 const FVector Direction=(HP-P).GetSafeNormal2D();FVector Target=HP-Direction*135;Target.Z=GroundHeight(Target)+100;
 O.EscortRepath-=D;if(O.EscortRepath<=0){FindRouteFor(Person,Target,O.EscortRoute);O.EscortRepath=.75f;}
 while(O.EscortRoute.Num()&&FVector::Dist2D(P,O.EscortRoute[0])<5)O.EscortRoute.RemoveAt(0);
 if(O.EscortRoute.Num()){
  FVector Move=(O.EscortRoute[0]-P).GetSafeNormal2D();const float Speed=Distance>750?490:O.EscortUnderFire?330:380;
  FVector Next=P+Move*FMath::Min(Speed*D,float(FVector::Dist2D(P,O.EscortRoute[0])));if(!TerrainSegmentClear(P,Next))return;
  if(!MoveCompanion(Person,Next)){O.EscortRoute.Reset();O.EscortRepath=0;}
  Person->SetActorRotation(FMath::RInterpTo(Person->GetActorRotation(),Move.Rotation(),D,9));
 }
 if(Playtest&&FMath::Fmod(LocalTime,20.f)<D)UE_LOG(LogTemp,Display,TEXT("ESCORT_PROGRESS chapter=%d objective=%d hero_distance=%.1f rally_distance=%.1f threatened=%d route=%d"),Chapter,I,Distance,FVector::Dist2D(Person->GetActorLocation(),O.Rally),O.EscortUnderFire,O.EscortRoute.Num());
 if(Playtest&&FParse::Param(FCommandLine::Get(),TEXT("stop-mid-escort"))&&FVector::Dist2D(Person->GetActorLocation(),O.Origin)>900){
  Save();UE_LOG(LogTemp,Display,TEXT("ESCORT_MIDWAY_SAVED chapter=%d objective=%d position=%s distance_from_origin=%.1f"),Chapter,I,*Person->GetActorLocation().ToCompactString(),FVector::Dist2D(Person->GetActorLocation(),O.Origin));GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
 }
}
