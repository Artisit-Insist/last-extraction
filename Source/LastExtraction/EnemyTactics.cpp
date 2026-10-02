#include "Extraction.h"
#include "CommandoAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
const TCHAR* TacticName(EEnemyTactic T){
 switch(T){case EEnemyTactic::Patrol:return TEXT("patrol");case EEnemyTactic::Search:return TEXT("search");case EEnemyTactic::Advance:return TEXT("advance");case EEnemyTactic::Flank:return TEXT("flank");case EEnemyTactic::Hide:return TEXT("cover");case EEnemyTactic::Peek:return TEXT("peek");default:return TEXT("hold");}
}
}

void AExtractionMode::SetEnemyTactic(FExtractionEnemy& E,EEnemyTactic T,float Duration){
 if(E.Tactic!=T){
  E.Tactic=T;E.StateAge=0;E.Waypoints.Reset();E.Repath=0;EnemyTacticTransitions++;
  if(T==EEnemyTactic::Flank)EnemyFlanks++;
  if(T==EEnemyTactic::Search)EnemySearches++;
  if(Playtest)UE_LOG(LogTemp,Display,TEXT("ENEMY_TACTIC chapter=%d id=%d type=%d state=%s pos=%s goal=%s"),Chapter,E.Identity,E.Type,TacticName(T),*E.Actor->GetActorLocation().ToCompactString(),*E.Goal.ToCompactString());
 }
 E.StateTime=Duration;
}

void AExtractionMode::HearCombatNoise(FVector P,float Radius){
 for(auto& E:Enemies){
  if(!IsValid(E.Actor)||E.Health<=0||FVector::DistSquared2D(E.Actor->GetActorLocation(),P)>Radius*Radius)continue;
  // Sound gives a last known position. It never grants line of sight through walls.
  if(E.LostSight>.2f)E.LastKnown=P;
  E.HasContact=true;E.Alert=FMath::Max(E.Alert,7.f);E.Think=FMath::Min(E.Think,.15f);
 }
}

bool AExtractionMode::EnemySegmentClear(AActor* Person,FVector A,FVector B) const{
 if(!TerrainSegmentClear(A,B))return false;
 FCollisionQueryParams Q;Q.AddIgnoredActor(Hero);Q.AddIgnoredActor(Person);
 for(const auto& E:Enemies)if(IsValid(E.Actor))Q.AddIgnoredActor(E.Actor);
 for(auto* F:Followers)if(IsValid(F))Q.AddIgnoredActor(F);
 const FVector Start=A;int Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(A,B)/140));
 for(int I=1;I<=Steps;I++){
  FVector Next=FMath::Lerp(Start,B,float(I)/Steps);Next.Z=GroundHeight(Next)+100;
  FHitResult Hit;if(GetWorld()->SweepSingleByChannel(Hit,A,Next,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(38,90),Q))return false;A=Next;
 }
 return true;
}

void AExtractionMode::BuildEnemyCover(){
 EnemyCoverBounds.Reset();NextEnemyPathTime=0;EnemyPathCursor=0;
 for(auto* A:WorldActors){
  if(!IsValid(A)||!A->GetActorEnableCollision())continue;
  FVector C,B;A->GetActorBounds(true,C,B);const float Wide=FMath::Max(B.X,B.Y);
  if(B.Z<65||Wide<40||Wide>1400)continue;
  EnemyCoverBounds.Add(FBox(C-B,C+B));
 }
 UE_LOG(LogTemp,Display,TEXT("TACTICAL_COVER_BUILT chapter=%d solid_objects=%d"),Chapter,EnemyCoverBounds.Num());
 // Landmarks and vegetation are built after sentries. Resolve new overlaps once,
 // before play begins; agents never teleport as a runtime navigation shortcut.
 for(auto& E:Enemies){
  if(!IsValid(E.Actor)||E.Type==3)continue;
  FCollisionQueryParams Q;Q.AddIgnoredActor(E.Actor);Q.AddIgnoredActor(Hero);
  const FVector From=E.Actor->GetActorLocation();bool Clear=false;
  for(int Ring=0;Ring<14&&!Clear;Ring++)for(int K=0;K<(Ring?16:1);K++){
   FVector P=From+FVector(FMath::Cos(K*PI/8),FMath::Sin(K*PI/8),0)*(Ring*110);P.Z=GroundHeight(P)+100;
   if(!TerrainWalkable(P)||GetWorld()->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(44,90),Q))continue;
   E.Actor->SetActorLocation(P);E.Home=E.Goal=P;Clear=true;break;
  }
  ensureAlwaysMsgf(Clear,TEXT("A guard must start outside scenery"));
 }
}

bool AExtractionMode::SelectEnemyCover(FExtractionEnemy& E,FVector Threat){
 const FVector From=E.Actor->GetActorLocation();float Best=1300;FVector Hidden,Exposed;bool Found=false;
 for(const FBox& B:EnemyCoverBounds){
  FVector C=B.GetCenter(),Size=B.GetExtent();if(FVector::DistSquared2D(From,C)>1400*1400)continue;
  FVector Away=(C-Threat).GetSafeNormal2D(),Side(-Away.Y,Away.X,0);
  float Radius=FMath::Abs(Away.X)*Size.X+FMath::Abs(Away.Y)*Size.Y;
  FVector Hide=C+Away*(Radius+80);Hide.Z=GroundHeight(Hide)+100;
  bool Occupied=false;for(const auto& Other:Enemies)if(&Other!=&E&&IsValid(Other.Actor)&&(Other.Tactic==EEnemyTactic::Hide||Other.Tactic==EEnemyTactic::Peek)&&FVector::DistSquared2D(Other.Cover,Hide)<180*180){Occupied=true;break;}
  if(Occupied)continue;
  if(FVector::Dist2D(Hide,Threat)<420||!TerrainWalkable(Hide)||CanSee(Hide+FVector(0,0,12),Threat+FVector(0,0,12),E.Actor))continue;
  float Score=FVector::Dist2D(From,Hide);if(Score>=Best)continue;
  for(float Offset:{180.f,320.f,500.f,760.f}){
   for(float Sign:{-1.f,1.f}){
    FVector Peek=Hide+Side*Offset*Sign;Peek.Z=GroundHeight(Peek)+100;
    if(!TerrainWalkable(Peek)||!EnemySegmentClear(E.Actor,Hide,Peek)||!CanSee(Peek+FVector(0,0,12),Threat+FVector(0,0,12),E.Actor))continue;
    if(!EnemySegmentClear(E.Actor,From,Hide)&&!EnemySegmentClear(E.Actor,From,Peek))continue;
    Hidden=Hide;Exposed=Peek;Best=Score;Found=true;break;
   }
   if(Found&&Best==Score)break;
  }
 }
 if(!Found)return false;
 E.Cover=Hidden;E.Peek=Exposed;E.Goal=Hidden;E.CoverCycles=0;
 EnemyCoverSelections++;SetEnemyTactic(E,EEnemyTactic::Hide,1.0f+(E.Identity%3)*.25f);
 if(Playtest)UE_LOG(LogTemp,Display,TEXT("ENEMY_COVER_SELECTED chapter=%d id=%d hide=%s peek=%s"),Chapter,E.Identity,*Hidden.ToCompactString(),*Exposed.ToCompactString());
 return true;
}

void AExtractionMode::SpawnSiteGuards(FVector P,int Node,int Kind){
 const int Count=4+Chapter+(Difficulty==2?2:0);
 const FVector Approach=(P-(Node?SectorNodes[Node-1]:SectorStart)).GetSafeNormal2D(),Side(-Approach.Y,Approach.X,0);
 // A sentry pair, a rear support post, then staggered patrols: no circular guard ring.
 const FVector2D Patterns[6][9]={
  {{-650,-390},{-180,770},{680,-530},{970,610},{-900,460},{470,940},{-550,-950},{1120,-50},{210,-850}},
  {{-880,-280},{-580,440},{350,-680},{890,500},{-1150,190},{660,840},{150,-930},{1130,-350},{-370,990}},
  {{-760,-480},{-380,820},{770,-190},{510,950},{-1060,140},{920,-820},{-180,-910},{1090,480},{120,540}},
  {{-880,-610},{-500,680},{340,-890},{950,470},{-1100,250},{710,890},{-290,-1130},{1160,-430},{70,790}},
  {{-800,-600},{-800,620},{320,-920},{320,900},{970,-420},{1020,450},{-1170,50},{820,1090},{870,-1090}},
  {{-920,-500},{-650,730},{460,-810},{820,690},{-1180,190},{1040,-250},{-180,1070},{280,-1130},{1140,1090}}
 };
 for(int I=0;I<Count;I++){
  FVector2D Q=Patterns[Chapter][I%9];if(Node%2)Q.Y=-Q.Y;
  const int Before=Enemies.Num();SpawnEnemy(P+Approach*Q.X+Side*Q.Y,(I==2&&Chapter>1)?2:I%4==0?1:0);
  if(Enemies.Num()>Before)Enemies.Last().Actor->SetActorRotation((-Approach).Rotation());
 }
 if(Kind==4)SpawnEnemy(P+FVector(0,-180,100),3);
}

void AExtractionMode::UpdateEnemies(float D,FVector HeroVelocity,bool RigPreview){
 const FVector HP=Hero->GetActorLocation();
 int PathCandidate=INDEX_NONE;
 if(LocalTime>=NextEnemyPathTime&&!RigPreview){
  // Round-robin ownership prevents the first few guards from consuming every
  // route slot when a large group is active at the same time.
  for(int Offset=0;Offset<Enemies.Num();Offset++){
   const int I=(EnemyPathCursor+Offset)%Enemies.Num();const auto& E=Enemies[I];
   if(!IsValid(E.Actor)||E.Health<=0||E.Type==3||E.Repath>D||FVector::DistSquared2D(E.Actor->GetActorLocation(),HP)>3300*3300||FVector::DistSquared2D(E.Actor->GetActorLocation(),E.Goal)<=45*45)continue;
   PathCandidate=I;EnemyPathCursor=(I+1)%Enemies.Num();break;
  }
 }
 for(auto& E:Enemies){
  if(!IsValid(E.Actor)||E.Health<=0)continue;
  if(RigPreview){bool Review=E.Actor==Enemies.Last().Actor;E.Actor->SetActorHiddenInGame(!Review);if(Review){E.Actor->SetActorLocation(HP+FVector(0,-170,0));E.Actor->SetActorRotation(FRotator::ZeroRotator);}continue;}
  FVector P=E.Actor->GetActorLocation();float Dist=FVector::Dist2D(P,HP);if(Dist>3300)continue;
  E.Think-=D;E.Repath-=D;E.Cooldown-=D;E.StateAge+=D;E.Suppression=FMath::Max(0.f,E.Suppression-D);E.LostSight+=D;
  const bool Sight=Dist<2050&&CanSee(P+FVector(0,0,12),HP+FVector(0,0,12),E.Actor);
  const bool Facing=FVector::DotProduct(E.Actor->GetActorForwardVector(),(HP-P).GetSafeNormal2D())>.25;
  if(Sight&&((Dist<1500&&(Facing||E.Alert>0))||Dist<380)){
   E.LastKnown=HP;E.HasContact=true;E.Alert=8;E.LostSight=0;
  }else E.Alert=FMath::Max(0.f,E.Alert-D);
  if(E.Alert<=0){
   E.HasContact=false;E.Goal=E.Home+FVector(FMath::Cos(LocalTime*.13+E.Seed)*260,FMath::Sin(LocalTime*.13+E.Seed)*260,0);SetEnemyTactic(E,EEnemyTactic::Patrol);
  }else if(E.Think<=0){
   E.Think=.4f+(E.Identity%4)*.035f;
   const bool InCoverCycle=E.Tactic==EEnemyTactic::Hide||E.Tactic==EEnemyTactic::Peek;
   if(E.LostSight>1.3f&&!InCoverCycle&&E.Suppression<=0){E.Goal=E.LastKnown;SetEnemyTactic(E,EEnemyTactic::Search);}
   else if(!InCoverCycle&&E.Type!=3){
    if((E.Suppression>0||(E.Type==0&&E.Identity%2==0))&&Dist<1700&&E.StateTime<=0&&SelectEnemyCover(E,E.LastKnown)){}
    else if(E.Type==1){
     if(E.Tactic!=EEnemyTactic::Flank&&E.Tactic!=EEnemyTactic::Hold){
      FVector Offset=(P-E.LastKnown).GetSafeNormal2D().RotateAngleAxis((E.Identity%2?1:-1)*62,FVector::UpVector);
      FVector Flank=E.LastKnown+Offset*1000;Flank.Z=GroundHeight(Flank)+100;
      if(TerrainWalkable(Flank)){E.Goal=Flank;SetEnemyTactic(E,EEnemyTactic::Flank,5.0f);}
      else{E.Goal=E.LastKnown;SetEnemyTactic(E,EEnemyTactic::Advance);}
     }
    }else{
     FVector Away=(P-E.LastKnown).GetSafeNormal2D();E.Goal=E.LastKnown+Away*(E.Type==2?1000:780);
     if(E.Tactic!=EEnemyTactic::Advance)SetEnemyTactic(E,EEnemyTactic::Advance);
    }
   }
  }
  FVector Goal=E.Goal;
  if((E.Tactic==EEnemyTactic::Hide||E.Tactic==EEnemyTactic::Peek)&&E.StateAge>6.0f){
   E.Goal=E.LastKnown;Goal=E.Goal;SetEnemyTactic(E,EEnemyTactic::Advance,4.f);
  }
  if(E.Tactic==EEnemyTactic::Hide||E.Tactic==EEnemyTactic::Peek){
   Goal=E.Tactic==EEnemyTactic::Hide?E.Cover:E.Peek;
   if(FVector::Dist2D(P,Goal)<65)E.StateTime-=D;
   if(E.StateTime<=0){
    if(E.Tactic==EEnemyTactic::Hide){E.Goal=E.Peek;SetEnemyTactic(E,EEnemyTactic::Peek,1.45f);}
    else if(++E.CoverCycles>=2||E.LostSight>6){E.Goal=E.LastKnown;SetEnemyTactic(E,EEnemyTactic::Advance,3.5f);E.Think=.6f;}
    else{E.Goal=E.Cover;SetEnemyTactic(E,EEnemyTactic::Hide,1.1f);}
    Goal=E.Goal;
   }
  }else if(E.Tactic==EEnemyTactic::Flank){
   E.StateTime-=D;if(FVector::Dist2D(P,Goal)<100||E.StateTime<=0){E.Goal=P;Goal=P;SetEnemyTactic(E,EEnemyTactic::Hold,2.3f);}
  }else if(E.Tactic==EEnemyTactic::Hold){E.StateTime-=D;if(E.StateTime<=0){E.Goal=E.LastKnown;SetEnemyTactic(E,EEnemyTactic::Advance);}}
  else E.StateTime=FMath::Max(0.f,E.StateTime-D);

  FVector Move=FVector::ZeroVector;const float Distance=FVector::Dist2D(P,Goal);
  if(E.Type!=3&&Distance>45){
   if(E.Repath<=0&&E.Identity==PathCandidate){
    NextEnemyPathTime=LocalTime+.10f;E.Repath=1.5f+(E.Identity%5)*.13f;
    FindRouteFor(E.Actor,Goal,E.Waypoints);
   }
   while(E.Waypoints.Num()&&FVector::Dist2D(P,E.Waypoints[0])<12)E.Waypoints.RemoveAt(0);
   if(E.Waypoints.Num())Move=(E.Waypoints[0]-P).GetSafeNormal2D();
   const float Speed=E.Tactic==EEnemyTactic::Patrol?85:E.Type==2?145:E.Tactic==EEnemyTactic::Flank?270:210;
   if(!Move.IsNearlyZero()){
    float Step=FMath::Min(Speed*D,float(FVector::Dist2D(P,E.Waypoints[0])));FHitResult Hit;MoveOnTerrain(E.Actor,Move*Step,&Hit);
    if(Hit.bBlockingHit){E.Waypoints.Reset();E.Repath=FMath::Min(E.Repath,.25f);}
   }
  }
  FVector Look=E.Alert>0?E.LastKnown-P:Move;
  if(!Look.IsNearlyZero())E.Actor->SetActorRotation(FMath::RInterpTo(E.Actor->GetActorRotation(),Look.GetSafeNormal2D().Rotation(),D,8));
  if(E.Type!=3)AnimateCharacter(E.Actor->FindComponentByClass<USkeletalMeshComponent>(),FVector::Dist2D(P,E.Actor->GetActorLocation())/FMath::Max(D,.001f));
  const bool Aimed=FVector::DotProduct(E.Actor->GetActorForwardVector(),(HP-E.Actor->GetActorLocation()).GetSafeNormal2D())>.85f;
  const bool MayFire=Aimed&&E.HasContact&&E.LostSight<.2f&&E.Tactic!=EEnemyTactic::Hide&&E.Tactic!=EEnemyTactic::Flank;
  if(MayFire&&E.Cooldown<=0&&Dist<1850&&Sight){
   P=E.Actor->GetActorLocation();const FVector Target=HP+HeroVelocity*.15+FVector(Random.FRandRange(-110,110),Random.FRandRange(-110,110),12);
   const FVector Muzzle=P+(Target-P).GetSafeNormal2D()*72+FVector(0,0,12);
   if(!CanSee(P+FVector(0,0,12),Muzzle,E.Actor)||!CanSee(Muzzle,HP+FVector(0,0,12),E.Actor)){E.Cooldown=.15f;continue;}
   if(E.Type==2&&E.Burst<2){E.Burst++;E.Cooldown=.24f;}else{E.Burst=0;E.Cooldown=(E.Type==1?2.1f:E.Type==2?2.6f:E.Type==3?1.8f:1.35f)+Random.FRandRange(0,.5f);}
   if(Playtest&&E.Type==2)UE_LOG(LogTemp,Display,TEXT("ENEMY_HEAVY_SHOT chapter=%d id=%d burst_step=%d time=%.3f"),Chapter,E.Identity,E.Burst,LocalTime);
   FExtractionShot S;S.Position=Muzzle;S.Velocity=(Target-Muzzle).GetSafeNormal()*(E.Type==1?1400:1000);S.Damage=E.Type==3?24:E.Type==1?17:E.Type==2?8:10;
   const FLinearColor Amber(1,.5,.12);S.Visual=Shape(S.Position,FVector(.38,.014,.014),Amber,0,false,S.Velocity.Rotation(),false);
   Cast<AStaticMeshActor>(S.Visual)->GetStaticMeshComponent()->SetMaterial(0,Material(Amber*4,true));Shots.Add(S);
   if(auto* Mesh=E.Actor->FindComponentByClass<USkeletalMeshComponent>())if(auto* Anim=Cast<UCommandoAnimation>(Mesh->GetAnimInstance()))Anim->Trigger(TEXT("Fire"));
   Flash(Muzzle,Amber,.18,.09);
   Sound(0,Muzzle,FMath::Clamp(.15f*(1-Dist/2200.f),.02f,.15f));
  }
 }
}
