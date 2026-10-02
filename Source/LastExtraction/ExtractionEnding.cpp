#include "Extraction.h"
#include "CommandoAnimation.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

void AExtractionMode::BeginExtraction(){
 if(Ending||Victory)return;
 Ending=true;EndingTime=0;EndingFrame=0;Boarded=0;Map=false;ToastTime=0;Hero->Invincible=100;Hero->Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 LandingPosition=Exit;LandingPosition.Z=GroundHeight(Exit)+12;
 for(auto& S:Shots)if(IsValid(S.Visual))S.Visual->Destroy();Shots.Reset();
 for(auto& E:Enemies)if(IsValid(E.Actor))E.Actor->SetActorHiddenInGame(true);
 for(auto* F:Followers)if(IsValid(F))F->SetActorHiddenInGame(true);
 Helicopter=Asset(TEXT("RescueTransport"),LandingPosition+FVector(-1800,2200,2200));
 auto Attach=[&](const TCHAR* Name,FVector Offset){AActor* A=Asset(Name,FVector::ZeroVector);if(A&&Helicopter){A->AttachToActor(Helicopter,FAttachmentTransformRules::KeepRelativeTransform);A->SetActorRelativeLocation(Offset);}return A;};
 MainRotor=Attach(TEXT("TransportRotor"),FVector(-10,0,399));TailRotor=Attach(TEXT("TransportTailRotor"),FVector(-850,-34,330));for(auto* Rotor:{MainRotor,TailRotor})if(Rotor)Cast<AStaticMeshActor>(Rotor)->GetStaticMeshComponent()->SetCastShadow(false);
 HelicopterDoors.Reset();for(int Side:{-1,1})for(int Panel:{-1,1})HelicopterDoors.Add(Attach(TEXT("TransportDoor"),FVector(Panel*275,Side*138,76)));
 ExtractionParty.Reset();
 const int Survivors=FMath::Clamp(RescuedTotal,Followers.Num(),12);
 for(int I=0;I<Survivors;I++){
  FVector P=LandingPosition+FVector(-180-(I%4)*110,-770-(I/4)*175,100);P.Z=GroundHeight(P)+100;
  AActor* Person=Shape(P,FVector(1),FLinearColor::White,1,false);Cast<AStaticMeshActor>(Person)->GetStaticMeshComponent()->SetVisibility(false,false);if(I%3==0)Person->Tags.Add(TEXT("WorkerWardrobe"));MakeSoldier(Person,false,-1);
  Person->SetActorRotation(FRotator(0,90,0));ExtractionParty.Add(Person);
 }
 Hero->Tags.Remove(TEXT("OnBoard"));Hero->Tags.Remove(TEXT("Seated"));ExtractionParty.Add(Hero);Hero->SetActorLocation(LandingPosition+FVector(-780,-1150,100));Hero->Visual->SetRelativeRotation(FRotator::ZeroRotator);
 for(auto* Part:Hero->RifleParts)Part->SetVisibility(false);for(auto* Part:Hero->BowParts)Part->SetVisibility(false);
 if(Hero->CharacterMesh)if(auto* Anim=Cast<UCommandoAnimation>(Hero->CharacterMesh->GetAnimInstance()))Anim->SetCivilian(true);
 Hero->Camera->SetAbsolute(true,true,false);Hero->Camera->FieldOfView=57;
 GetWorld()->GetFirstPlayerController()->bShowMouseCursor=false;
 if(auto* Rotor=LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/RescueRotor.RescueRotor")))RotorAudio=UGameplayStatics::SpawnSound2D(this,Rotor,.0001f);
 UE_LOG(LogTemp,Display,TEXT("EXTRACTION_ENDING_STARTED rescued=%d party=%d"),Survivors,ExtractionParty.Num());
}

void AExtractionMode::UpdateExtraction(float D){
 EndingTime+=D;float T=EndingTime;
 auto Ease=[](float V){V=FMath::Clamp(V,0.f,1.f);return V*V*(3-2*V);};
 const float LastArrival=10.6f+(ExtractionParty.Num()-1)*.85f;
 const float DoorStart=LastArrival+1.0f,LiftStart=DoorStart+3.2f,Departure=LiftStart+12,Finish=Departure+8;
 FVector HelicopterP=LandingPosition;
 if(T<7)HelicopterP+=FVector(-1800,2200,2200)*(1-Ease(T/7));
 else if(T>LiftStart){float F=Ease((T-LiftStart)/12);HelicopterP+=FVector(550*F,200*F,1350*F);if(T>Departure){float Out=Ease((T-Departure)/8);HelicopterP+=FVector(6300,3000,2700)*Out;}}
 if(Helicopter){Helicopter->SetActorLocation(HelicopterP);Helicopter->SetActorRotation(FRotator(T>LiftStart?-5*Ease((T-LiftStart)/6):0,T>Departure?20*Ease((T-Departure)/5):0,0));}
 if(MainRotor)MainRotor->SetActorRelativeRotation(FRotator(0,FMath::Fmod(T*1560,360.f),0));
 if(TailRotor)TailRotor->SetActorRelativeRotation(FRotator(FMath::Fmod(T*2700,360.f),0,0));
 for(int I=0;I<HelicopterDoors.Num();I++)if(HelicopterDoors[I]){int Panel=I%2?1:-1,Side=I<2?-1:1;HelicopterDoors[I]->SetActorRelativeLocation(FVector(Panel*FMath::Lerp(275.f,103.f,Ease((T-DoorStart)/1.8f)),Side*138,76));}
 for(int I=0;I<ExtractionParty.Num();I++){
  AActor* Person=ExtractionParty[I];if(!IsValid(Person))continue;float Travel=(T-(7+I*.85f))/3.6f;
  if(Travel<=0)continue;
  if(Travel>=1){
   if(!Person->Tags.Contains(TEXT("OnBoard"))){Person->Tags.Add(TEXT("OnBoard"));Person->AttachToActor(Helicopter,FAttachmentTransformRules::KeepWorldTransform);Boarded++;UE_LOG(LogTemp,Display,TEXT("EXTRACTION_BOARDED person=%d boarded=%d total=%d"),I,Boarded,ExtractionParty.Num());}
   const float SeatX[3]={-165,-95,115};FVector Cabin=I<6?FVector(SeatX[I/2],I%2?80:-80,169):FVector(-165+(I-6)*55,0,169);
   float IntoCabin=Ease((Travel-1)*3.6f/.8f);Person->SetActorRelativeLocation(FMath::Lerp(FVector(0,-30,169),Cabin,IntoCabin));Person->SetActorRelativeRotation(FRotator(0,I<6?(I%2?-90:90):90,0));
   if(I<6&&IntoCabin>=1&&!Person->Tags.Contains(TEXT("Seated"))){Person->Tags.Add(TEXT("Seated"));if(auto* M=Person->FindComponentByClass<USkeletalMeshComponent>())if(auto* A=Cast<UCommandoAnimation>(M->GetAnimInstance()))A->Trigger(TEXT("Sit"));}
   continue;
  }
  FVector Start=LandingPosition+FVector(-180-(I%4)*110,-770-(I/4)*175,100);if(Person==Hero)Start=LandingPosition+FVector(-780,-1150,100);Start.Z=GroundHeight(Start)+100;
  FVector Step=LandingPosition+FVector(0,-235,100),Inside=LandingPosition+FVector(0,-30,169),P;
  if(Travel<.72f){P=FMath::Lerp(Start,Step,Travel/.72f);P.Z=GroundHeight(P)+100;}
  else P=FMath::Lerp(Step,Inside,(Travel-.72f)/.28f);
  FVector Direction=P-Person->GetActorLocation();Person->SetActorLocation(P);
  if(Direction.Size2D()>1)Person->SetActorRotation(FMath::RInterpTo(Person->GetActorRotation(),Direction.GetSafeNormal2D().Rotation(),D,9));
 }
 FVector Camera,Look;float Fov=57;
 if(T<7){Camera=LandingPosition+FVector(-2100,-2400,1250);Look=FMath::Lerp(LandingPosition+FVector(0,0,170),HelicopterP+FVector(0,0,150),.7f);Fov=63;}
 else if(T<DoorStart){float A=Ease((T-7)/(DoorStart-7));Camera=LandingPosition+FMath::Lerp(FVector(580,-1230,370),FVector(-570,-1140,330),A);Look=LandingPosition+FVector(-80,-210,145);Fov=58;}
 else if(T<LiftStart){Camera=LandingPosition+FVector(600,-1150,330);Look=LandingPosition+FVector(-30,0,170);Fov=58;}
 else if(T<Departure){float A=Ease((T-LiftStart)/12);Camera=LandingPosition+FMath::Lerp(FVector(-1700,-2200,1050),FVector(-2400,-3000,1900),A);Look=HelicopterP+FVector(-60,0,150);Fov=62;}
 else{Camera=LandingPosition+FVector(-3200,-3900,1800);Look=FMath::Lerp(HelicopterP,LandingPosition+FVector(3500,1400,1500),.25f);Fov=64;}
 Hero->Camera->SetWorldLocation(Camera);Hero->Camera->SetWorldRotation((Look-Camera).Rotation());Hero->Camera->FieldOfView=Fov;
 if(RotorAudio){float Gain=T<7?FMath::Lerp(.13f,.70f,Ease(T/7)):T<Departure?.7f:FMath::Lerp(.7f,.06f,Ease((T-Departure)/8));RotorAudio->SetVolumeMultiplier(Gain*EffectsVolume);}
 if(FParse::Param(FCommandLine::Get(),TEXT("record-ending")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/world_revision/frames/frame_%05d.png"),EndingFrame++),true,false);
 if(FParse::Param(FCommandLine::Get(),TEXT("ending-preview"))&&!FParse::Param(FCommandLine::Get(),TEXT("record-ending"))&&FMath::FloorToInt(T)!=FMath::FloorToInt(T-D)&&int(T)%5==0)FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/world_revision/ending_%02d.png"),int(T)),true,false);
 if(T>=Finish){
  Ending=false;Victory=true;Save();if(RotorAudio)RotorAudio->FadeOut(1,0);
  int Occupants=0;for(auto* P:ExtractionParty)if(IsValid(P)&&!P->IsHidden()&&P->GetAttachParentActor()==Helicopter)Occupants++;
  UE_LOG(LogTemp,Display,TEXT("EXTRACTION_ENDING_COMPLETE boarded=%d total=%d aboard_actors=%d duration=%.2f"),Boarded,ExtractionParty.Num(),Occupants,T);ensureAlways(Occupants==ExtractionParty.Num());
  ensureAlwaysMsgf(Boarded==ExtractionParty.Num(),TEXT("Every rescued person and the hero must board before victory."));
  if(FParse::Param(FCommandLine::Get(),TEXT("ending-preview")))GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
 }
}
