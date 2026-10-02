#include "Extraction.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include <queue>
#include <vector>

bool AExtractionMode::FindRoute(FVector Destination){return FindRouteFor(Hero,Destination,Route);}
bool AExtractionMode::FindRouteFor(AActor* Mover,FVector Destination,TArray<FVector>& Result){
 Result.Empty();const int NX=125,NY=89;const float Step=160;const FVector Origin(-9920,-7040,100);
 auto Index=[&](FVector P){int X=FMath::Clamp(FMath::RoundToInt((P.X-Origin.X)/Step),0,NX-1),Y=FMath::Clamp(FMath::RoundToInt((P.Y-Origin.Y)/Step),0,NY-1);return Y*NX+X;};
 auto Position=[&](int I){FVector P=Origin+FVector((I%NX)*Step,(I/NX)*Step,0);P.Z=GroundHeight(P)+100;return P;};
 FCollisionQueryParams Q;Q.AddIgnoredActor(Hero);Q.AddIgnoredActor(Mover);for(auto& E:Enemies)if(IsValid(E.Actor))Q.AddIgnoredActor(E.Actor);for(auto* F:Followers)if(IsValid(F))Q.AddIgnoredActor(F);
 auto Clear=[&](FVector A,FVector B,float Radius=42.f){
  if(!TerrainSegmentClear(A,B))return false;
  const int Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(A,B)/120));FVector From=A;
  for(int I=1;I<=Steps;I++){FVector To=FMath::Lerp(A,B,float(I)/Steps);To.Z=GroundHeight(To)+100;FHitResult H;if(GetWorld()->SweepSingleByChannel(H,From,To,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(Radius,90),Q))return false;From=To;}return true;
 };
 FVector Start=Mover->GetActorLocation();Destination.Z=GroundHeight(Destination)+100;
 if(Clear(Start,Destination)){Result.Add(Destination);return true;}
 TArray<int8> Walk;Walk.Init(-1,NX*NY);
 auto CanWalk=[&](int I){if(Walk[I]<0)Walk[I]=TerrainWalkable(Position(I))&&!GetWorld()->OverlapBlockingTestByChannel(Position(I),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(44,90),Q);return Walk[I]>0;};
 int S=-1,G=Index(Destination),Center=Index(Start);float StartBest=1e12;
 // Rounding a player beside a rock can put its grid cell inside the rock.
 // Attach the actual position to a nearby free node with a clear connecting segment.
 for(int y=-3;y<=3;y++)for(int x=-3;x<=3;x++){int X=Center%NX+x,Y=Center/NX+y;if(X<0||X>=NX||Y<0||Y>=NY)continue;int I=Y*NX+X;float Dist=FVector::DistSquared2D(Start,Position(I));if(Dist<StartBest&&CanWalk(I)&&Clear(Start,Position(I),35.f)){S=I;StartBest=Dist;}}
 if(S<0){
  TArray<FOverlapResult> Overlaps;GetWorld()->OverlapMultiByChannel(Overlaps,Start,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(35,90),Q);
  for(auto& Hit:Overlaps){auto* Mesh=Cast<UStaticMeshComponent>(Hit.GetComponent());UE_LOG(LogTemp,Warning,TEXT("ROUTE_START_OVERLAP actor=%s mesh=%s bounds=%s"),*GetNameSafe(Hit.GetActor()),Mesh?*GetNameSafe(Mesh->GetStaticMesh()):TEXT("none"),Hit.GetComponent()?*Hit.GetComponent()->Bounds.BoxExtent.ToCompactString():TEXT("none"));}
  UE_LOG(LogTemp,Warning,TEXT("ROUTE_START_NOT_CONNECTED %s"),*Start.ToCompactString());return false;
 }
 // A free rounded grid cell does not guarantee the requested formation point is free.
 if(!CanWalk(G)||!Clear(Position(G),Destination)){float Best=1e9;int Near=-1;for(int y=-3;y<=3;y++)for(int x=-3;x<=3;x++){int X=G%NX+x,Y=G/NX+y;if(X<0||X>=NX||Y<0||Y>=NY)continue;int I=Y*NX+X;if(CanWalk(I)){float D=FVector::DistSquared2D(Position(I),Destination);if(D<Best){Best=D;Near=I;}}}if(Near<0)return false;G=Near;Destination=Position(G);}
 using Entry=std::pair<float,int>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> Open;
 TArray<float> Cost;Cost.Init(1e12,NX*NY);TArray<int> Parent;Parent.Init(-1,NX*NY);TArray<bool> Closed;Closed.Init(false,NX*NY);Cost[S]=0;Open.push({0,S});bool Found=false;
 while(!Open.empty()){int I=Open.top().second;Open.pop();if(Closed[I])continue;Closed[I]=true;if(I==G){Found=true;break;}
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){if(!dx&&!dy)continue;int X=I%NX+dx,Y=I/NX+dy;if(X<0||X>=NX||Y<0||Y>=NY)continue;int N=Y*NX+X;if(Closed[N]||!CanWalk(N)||!Clear(Position(I),Position(N)))continue;
   if(dx&&dy&&(!CanWalk((I/NX)*NX+X)||!CanWalk(Y*NX+I%NX)))continue;
   float New=Cost[I]+(dx&&dy?226.275f:Step);if(New<Cost[N]){Cost[N]=New;Parent[N]=I;Open.push({New+FVector::Dist2D(Position(N),Position(G)),N});}
  }
 }
 if(!Found){UE_LOG(LogTemp,Warning,TEXT("ROUTE_NOT_FOUND from=%s to=%s"),*Start.ToCompactString(),*Destination.ToCompactString());return false;}
 TArray<FVector> Reverse;for(int I=G;I!=S&&I>=0;I=Parent[I])Reverse.Add(Position(I));
 TArray<FVector> Raw;Raw.Add(Position(S));for(int i=Reverse.Num()-1;i>=0;i--)Raw.Add(Reverse[i]);Raw.Add(Destination);
 FVector From=Start;int At=0;while(At<Raw.Num()){int To=Raw.Num()-1;while(To>At&&!Clear(From,Raw[To]))To--;Result.Add(Raw[To]);From=Raw[To];At=To+1;}
 return Result.Num()>0;
}
FVector AExtractionMode::FollowRoute(){
 while(Route.Num()&&FVector::Dist2D(Hero->GetActorLocation(),Route[0])<22)Route.RemoveAt(0);
 return Route.Num()?(Route[0]-Hero->GetActorLocation()).GetSafeNormal2D():FVector::ZeroVector;
}
void AExtractionMode::UpdatePlaytest(float D){
 TestPressed.Empty();TestHeld.Empty();TestMove=FVector::ZeroVector;TestElapsed+=D;TestRepath-=D;
 if(Dead){TestDeaths++;if(TestDeaths>8){UE_LOG(LogTemp,Error,TEXT("ROUTE_PLAYTEST_FAILED too_many_deaths chapter=%d completed=%d"),Chapter,Completed);GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));return;}TestPressed.Add(EKeys::Enter);return;}
 if(Menu||Paused||Victory||Ending)return;
 if(Completed!=TestLastCompleted){TestLastCompleted=Completed;TestProgress=TestElapsed;TestRepath=0;UE_LOG(LogTemp,Display,TEXT("ROUTE_PLAYTEST_PROGRESS chapter=%d complete=%d elapsed=%.1f health=%.1f"),Chapter,Completed,Elapsed,Hero->Health);
  if(FParse::Param(FCommandLine::Get(),TEXT("capture-route")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/FString::Printf(TEXT("../../work/campaign_03/playtest/chapter_%d_objective_%d.png"),Chapter,Completed),true,false);
 }
 int StageKey=0;for(int I=0;I<Objectives.Num();I++)StageKey|=(Objectives[I].Stage&1)<<I;if(StageKey!=TestLastStage){TestLastStage=StageKey;TestProgress=TestElapsed;TestRepath=0;}
 if(TestElapsed-TestProgress>240){UE_LOG(LogTemp,Error,TEXT("ROUTE_PLAYTEST_FAILED stalled chapter=%d objective=%d position=%s ammo=%d reserve=%d enemies=%d health=%.1f"),Chapter,Focused,*Hero->GetActorLocation().ToCompactString(),Ammo,Reserve,Enemies.Num(),Hero->Health);GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));return;}
 FVector HP=Hero->GetActorLocation();int Goal=-1;for(int i=0;i<Objectives.Num();i++)if(!Objectives[i].Done){Goal=i;break;}
 if(ActiveEscort()>=0)Goal=ActiveEscort();
 FVector Target=Goal>=0?Objectives[Goal].Position+FVector(-180,-140,100):Exit+FVector(0,0,100);
 if(Goal>=0&&Objectives[Goal].Kind==0&&Objectives[Goal].Stage==1&&IsValid(Objectives[Goal].Prop)){auto& O=Objectives[Goal];FVector P=O.Prop->GetActorLocation();if(FVector::Dist2D(P,HP)>(O.EscortUnderFire?600:1100)){Target=P+(O.Rally-P).GetSafeNormal2D()*220;TestRepath=FMath::Min(TestRepath,.5f);}}
 int Visible=-1;float Nearest=1e9;for(int i=0;i<Enemies.Num();i++){auto& E=Enemies[i];if(!IsValid(E.Actor)||E.Health<=0)continue;FVector P=E.Actor->GetActorLocation();float Dist=FVector::Dist2D(HP,P);if(Dist<2100&&Dist<Nearest&&CanSee(HP+FVector(0,0,12),P+FVector(0,0,12),E.Actor)){Nearest=Dist;Visible=i;}}
 if(Hero->Health<52&&Medkits>0)TestPressed.Add(EKeys::Q);
 if(Ammo<4&&Reserve>0&&Reload<=0)TestPressed.Add(EKeys::R);
 if(Visible>=0){TestAim=Enemies[Visible].Actor->GetActorLocation()+FVector(0,0,12);TestHeld.Add(EKeys::LeftMouseButton);
  FVector Direction=(TestAim-HP).GetSafeNormal2D();TestMove=FVector(-Direction.Y,Direction.X,0)*FMath::Sin(TestElapsed*.9f);
  if(Nearest<380)TestMove=-Direction;if(Enemies[Visible].Type==3&&Grenades>0&&FMath::Fmod(TestElapsed,3)<D)TestPressed.Add(EKeys::G);
  if(FParse::Param(FCommandLine::Get(),TEXT("record-combat"))&&Nearest<1100&&Grenades>0&&FMath::Fmod(TestElapsed,5)<D)TestPressed.Add(EKeys::G);
 }else{
  TestAim=Target;
  if(FVector::Dist2D(HP,Target)>55){if(TestRepath<=0){FindRoute(Target);TestRepath=3;}TestMove=FollowRoute();}
 }
 if(FMath::Fmod(TestElapsed,20.f)<D){UE_LOG(LogTemp,Display,TEXT("ROUTE_DIAGNOSTIC pos=%s target=%s move=%s visible=%d nearest=%.1f route=%d next=%s"),*HP.ToCompactString(),*Target.ToCompactString(),*TestMove.ToCompactString(),Visible,Nearest,Route.Num(),Route.Num()?*Route[0].ToCompactString():TEXT("none"));}
 if(Goal>=0&&FVector::Dist2D(HP,Objectives[Goal].Position)<345){TestHeld.Add(EKeys::E);if(Objectives[Goal].Kind==3&&Defense<=0)TestPressed.Add(EKeys::E);}
 if(Goal<0&&FVector::Dist2D(HP,Exit)<480)TestPressed.Add(EKeys::E);
}
