#include "Extraction.h"
#include "CommandoAnimation.h"
#include "Components/SkeletalMeshComponent.h"

bool AExtractionMode::MoveCompanion(AActor* Person,FVector Destination){
 const FVector Start=Person->GetActorLocation();if(!TerrainSegmentClear(Start,Destination))return false;
 Destination.Z=GroundHeight(Destination)+100;
 FCollisionQueryParams Query;Query.AddIgnoredActor(Person);Query.AddIgnoredActor(Hero);
 for(auto* Ally:Followers)if(IsValid(Ally))Query.AddIgnoredActor(Ally);
 for(const auto& Enemy:Enemies)if(IsValid(Enemy.Actor))Query.AddIgnoredActor(Enemy.Actor);
 FHitResult Hit;
 if(GetWorld()->SweepSingleByChannel(Hit,Start,Destination,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeCapsule(42,90),Query))return false;
 Person->SetActorLocation(Destination);return true;
}

void AExtractionMode::UpdateCompanions(float D){
 for(int I=0;I<Followers.Num();I++){
  AActor* Ally=Followers[I];if(!IsValid(Ally))continue;
  while(AllyRoutes.Num()<=I){AllyRoutes.AddDefaulted();AllyRepaths.Add(0);AllyCooldowns.Add(.5f);}
  FVector P=Ally->GetActorLocation(),HP=Hero->GetActorLocation();int Target=-1;float Best=2450;
  for(int J=0;J<Enemies.Num();J++){auto& E=Enemies[J];if(!IsValid(E.Actor)||E.Health<=0)continue;float Distance=FVector::Dist2D(P,E.Actor->GetActorLocation());
   if(Distance<Best&&CanSee(P+FVector(0,0,12),E.Actor->GetActorLocation()+FVector(0,0,12),E.Actor)){Best=Distance;Target=J;}
  }
  FVector Formation=HP+Hero->Visual->GetComponentRotation().RotateVector(FVector(-240-(I/2)*155,(I%2?1:-1)*(180+I*25),0));Formation.Z=GroundHeight(Formation)+100;
  if(!TerrainWalkable(Formation))Formation=HP;
  AllyRepaths[I]-=D;AllyCooldowns[I]-=D;auto& Waypoints=AllyRoutes[I];
  const float Distance=FVector::Dist2D(P,Formation);
  if(Distance>140){
   if(AllyRepaths[I]<=0){FindRouteFor(Ally,Formation,Waypoints);AllyRepaths[I]=1.1f+I*.12f;}
   while(Waypoints.Num()&&FVector::Dist2D(P,Waypoints[0])<5)Waypoints.RemoveAt(0);
   if(Waypoints.Num()){
    FVector Move=(Waypoints[0]-P).GetSafeNormal2D();float Speed=Distance>850?500:Target>=0?230:350;
    FVector Next=P+Move*FMath::Min(Speed*D,float(FVector::Dist2D(P,Waypoints[0])));
    if(!MoveCompanion(Ally,Next)){Waypoints.Reset();AllyRepaths[I]=0;}
    if(Target<0)Ally->SetActorRotation(FMath::RInterpTo(Ally->GetActorRotation(),Move.Rotation(),D,9));
   }
  }else Waypoints.Reset();
  if(Target>=0){
   auto& Enemy=Enemies[Target];FVector EP=Enemy.Actor->GetActorLocation(),Direction=(EP-P).GetSafeNormal2D();Ally->SetActorRotation(FMath::RInterpTo(Ally->GetActorRotation(),Direction.Rotation(),D,12));
   if(AllyCooldowns[I]<=0){
    AllyCooldowns[I]=.65f+Random.FRandRange(0,.30f);FVector Muzzle=P+Direction*95+FVector(0,0,28);
    Beam(Muzzle,EP+FVector(0,0,15),FLinearColor(.8,.70,.38),.07f);Flash(Muzzle,FLinearColor(1,.58,.16),.12f,.055f);Sound(0,P,.16f);
    if(auto* Mesh=Ally->FindComponentByClass<USkeletalMeshComponent>())if(auto* Anim=Cast<UCommandoAnimation>(Mesh->GetAnimInstance()))Anim->Trigger(TEXT("Fire"));
    Enemy.Health-=Enemy.Type==3?5:14;Enemy.Alert=12;Enemy.LastKnown=P;Enemy.HasContact=true;Enemy.Suppression=1.8f;AllyShots++;if(Enemy.Health<=0)KillEnemy(Target,Direction);
    if(AllyShots==1||AllyShots%40==0)UE_LOG(LogTemp,Display,TEXT("ALLY_SUPPORT_FIRE shots=%d allies=%d target=%d"),AllyShots,Followers.Num(),Target);
   }
  }
 }
}
