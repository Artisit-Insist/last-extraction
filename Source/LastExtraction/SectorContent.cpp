#include "Extraction.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

void AExtractionMode::ConfigureSector(){
 const FVector Points[6][7]={
  {{-7300,-3900,0},{-6300,-600,0},{-3900,3100,0},{-400,4300,0},{2200,1600,0},{4700,-1600,0},{7400,1700,0}},
  {{-7600,-3900,0},{-5800,-200,0},{-3400,3600,0},{-2250,-2800,0},{2800,-2500,0},{4700,3100,0},{7700,900,0}},
  {{-5900,-3500,0},{-5900,500,0},{-2600,3500,0},{1300,700,0},{1500,-3500,0},{5600,-600,0},{5800,3700,0}},
  {{-6400,-2200,0},{-4300,2500,0},{-1400,4300,0},{2800,4000,0},{5500,1200,0},{3400,-3300,0},{6500,-4500,0}},
  {{-3000,-4200,0},{3200,-3500,0},{0,-1600,0},{-3100,800,0},{3000,1600,0},{0,3800,0},{0,5500,0}},
  {{-6000,-3800,0},{-2200,-4300,0},{2200,-4100,0},{5400,-1700,0},{3400,2300,0},{-2300,3800,0},{4400,4300,0}}
 };
 const FVector Starts[6]={{-9200,-5200,0},{-9200,-4200,0},{-8200,-5800,0},{-8500,-5000,0},{0,-6500,0},{-8500,-5200,0}};
 const FVector Exits[6]={{9400,3600,0},{9400,900,0},{8200,4700,0},{8900,-4800,0},{0,6650,0},{7800,3600,0}};
 SectorNodes.Reset();for(const FVector& P:Points[Chapter])SectorNodes.Add(P);
 SectorStart=Starts[Chapter];Exit=Exits[Chapter];BridgeOpen=Chapter!=1;BridgeGate=nullptr;
}
float AExtractionMode::RiverCenter(float Y)const{
 float Bend=FMath::SmoothStep(650.f,2100.f,FMath::Abs(Y+2800));
 return Bend*(540*FMath::Sin((Y+2800)*.00054f)+230*FMath::Sin((Y+2800)*.00112f));
}
float AExtractionMode::RiverHalfWidth(float Y)const{
 return 1550+FMath::SmoothStep(650.f,2000.f,FMath::Abs(Y+2800))*(150*FMath::Sin(Y*.0007f)+100*FMath::Cos(Y*.0013f));
}
float AExtractionMode::RouteDistance(FVector P)const{
 P.Z=0;float Dist=1e9;FVector A=SectorStart;
 for(const FVector& B:SectorNodes){Dist=FMath::Min(Dist,float(FMath::PointDistToSegment(P,A,B)));A=B;}
 return FMath::Min(Dist,float(FMath::PointDistToSegment(P,A,Exit)));
}

float AExtractionMode::TerrainPathWeight(FVector P)const{
 float Distance=1e9;
 if(Chapter==1)return 0;
 if(Chapter==0||Chapter==3)Distance=RouteDistance(P);
 if(Chapter==2){for(float X:{-7600.f,-4000.f,0.f,4000.f,7600.f})Distance=FMath::Min(Distance,float(FMath::Abs(P.X-X)));for(float Y:{-4900.f,-1800.f,1900.f,5200.f})Distance=FMath::Min(Distance,float(FMath::Abs(P.Y-Y)));}
 if(Chapter==4){for(float X:{-2400.f,0.f,2400.f})Distance=FMath::Min(Distance,float(FMath::Abs(P.X-X)));for(float Y:{-5200.f,-800.f,3000.f,5800.f})Distance=FMath::Min(Distance,float(FMath::Abs(P.Y-Y)));}
 if(Chapter==5){if(FMath::Abs(P.X)<9600&&FMath::Abs(P.Y)<1140)return 1;for(float Y:{-2600.f,2600.f})Distance=FMath::Min(Distance,float(FMath::Abs(P.Y-Y)));for(float X:{-7000.f,0.f,6500.f})if(FMath::Abs(P.Y)<3100)Distance=FMath::Min(Distance,float(FMath::Abs(P.X-X)));}
 if(FMath::Abs(P.X)>10000||FMath::Abs(P.Y)>6900)return 0;
 return 1-FMath::SmoothStep(Chapter==0?100.f:Chapter==3?230.f:180.f,Chapter==0?300.f:Chapter==3?520.f:410.f,Distance);
}
float AExtractionMode::GroundHeight(FVector P)const{
 const double X=FMath::Abs(P.X);
 if(Chapter==1&&FMath::Abs(P.Y+2800)<230&&X<1620){
  if(X<=1300)return 85.5-3.5*FMath::Sin(PI*(P.X+1300)/2600);
  return 44.75-.23945*(X-1450);
 }
 return TerrainSurfaceHeight(P);
}
float AExtractionMode::TerrainSurfaceHeight(FVector P)const{
 // Use the same triangles as the rendered/collidable 100 cm terrain grid.
 const double X=FMath::FloorToDouble(P.X/100)*100,Y=FMath::FloorToDouble(P.Y/100)*100;
 const double FX=(P.X-X)/100,FY=(P.Y-Y)/100;
 const float H00=RawGroundHeight(FVector(X,Y,0)),H10=RawGroundHeight(FVector(X+100,Y,0)),H01=RawGroundHeight(FVector(X,Y+100,0));
 if(FX+FY<=1)return H00+(H10-H00)*FX+(H01-H00)*FY;
 const float H11=RawGroundHeight(FVector(X+100,Y+100,0));
 return H11+(H01-H11)*(1-FX)+(H10-H11)*(1-FY);
}
void AExtractionMode::MoveOnTerrain(AActor* Actor,FVector Delta,FHitResult* Hit){
 if(!Actor||Delta.IsNearlyZero())return;
 const FVector Start=Actor->GetActorLocation(),End=Start+Delta;
 if(!TerrainSegmentClear(Start,End))return;
 Delta.Z=GroundHeight(End)+100-Start.Z;
 Actor->AddActorWorldOffset(Delta,true,Hit);
}
float AExtractionMode::RawGroundHeight(FVector P)const{
 auto Base=[this](FVector V)->float{
  float Noise=FMath::PerlinNoise2D(FVector2D(V.X,V.Y)*.0004f);
  if(Chapter==2)return 28*Noise+20*FMath::Sin(V.X*.0003f);
  if(Chapter==5)return 9*Noise;
  if(Chapter==4)return (V.Y+6500)*.075f+35*Noise;
  if(Chapter==3){float R=FVector2D((V.X-400)/3000,V.Y/2500).Size();float H=-1250+FMath::SmoothStep(.30f,.58f,R)*330+FMath::SmoothStep(.65f,.88f,R)*390+FMath::SmoothStep(.92f,1.13f,R)*580;return H+60*Noise;}
  return 95*FMath::Sin(V.X*.00048f+Chapter)+90*FMath::Cos(V.Y*.00063f+Chapter*.6f)+55*Noise;
 };
 float H=Base(P),Weight=0,Levels=0;
 auto BlendSite=[&](FVector Site){float W=1-FMath::SmoothStep(700.f,1900.f,float(FVector::Dist2D(P,Site)));Weight+=W;Levels+=Base(Site)*W;};
 for(const FVector& N:SectorNodes)BlendSite(N);BlendSite(SectorStart);BlendSite(Exit);
 if(Weight>0)H=FMath::Lerp(H,Levels/Weight,FMath::Min(1.f,Weight));
 if(Chapter==1){
  // Banks descend to water; only the bridge and its bank ramps are walkable crossings.
  float X=FMath::Abs(P.X-RiverCenter(P.Y));float Width=RiverHalfWidth(P.Y);float Bank=FMath::SmoothStep(Width-650.f,Width+280.f,X);H=FMath::Lerp(-235.f,H,Bank);
  // The riverbed stays below water. Only the two landing approaches are graded.
  float Landing=FMath::SmoothStep(1200.f,1500.f,X)*(1-FMath::SmoothStep(1650.f,2150.f,X))*(1-FMath::SmoothStep(400.f,800.f,float(FMath::Abs(P.Y+2800))));
  H=FMath::Lerp(H,0.f,Landing);
 }
 return H;
}
bool AExtractionMode::TerrainWalkable(FVector P)const{
 if(Chapter==3&&FVector2D((P.X-400)/3200,P.Y/2700).Size()<1)return false;
 if(Chapter!=1||FMath::Abs(P.X-RiverCenter(P.Y))>RiverHalfWidth(P.Y))return true;
 return BridgeOpen&&FMath::Abs(P.Y+2800)<225;
}
bool AExtractionMode::TerrainSegmentClear(FVector A,FVector B)const{
 if(Chapter!=1&&Chapter!=3)return true;
 const int Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(A,B)/32));
 for(int I=0;I<=Steps;I++)if(!TerrainWalkable(FMath::Lerp(A,B,float(I)/Steps)))return false;
 return true;
}
void AExtractionMode::BuildTerrain(){
 auto* A=GetWorld()->SpawnActor<AActor>();auto* Mesh=NewObject<UProceduralMeshComponent>(A);A->SetRootComponent(Mesh);A->AddInstanceComponent(Mesh);Mesh->SetMobility(EComponentMobility::Static);Mesh->RegisterComponent();Mesh->bUseComplexAsSimpleCollision=true;Mesh->SetCollisionProfileName(TEXT("BlockAll"));WorldActors.Add(A);
 TArray<FVector> V,N;TArray<FVector2D> UV;TArray<int32> Indices;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
 const int NX=211,NY=151;for(int Y=0;Y<NY;Y++)for(int X=0;X<NX;X++){
  FVector P(-10500+X*100,-7500+Y*100,0);P.Z=TerrainSurfaceHeight(P);V.Add(P);UV.Add(FVector2D(P.X/500,P.Y/500));
  N.Add(FVector(TerrainSurfaceHeight(P-FVector(10,0,0))-TerrainSurfaceHeight(P+FVector(10,0,0)),TerrainSurfaceHeight(P-FVector(0,10,0))-TerrainSurfaceHeight(P+FVector(0,10,0)),20).GetSafeNormal());
  const FLinearColor Tints[6]={FLinearColor(.62,.76,.51),FLinearColor(.66,.72,.58),FLinearColor(.94,.80,.58),FLinearColor(.67,.64,.60),FLinearColor(.68,.68,.64),FLinearColor(.70,.67,.56)};FLinearColor Color=Tints[Chapter];Color.A=TerrainPathWeight(P);Colors.Add(Color);Tangents.Add(FProcMeshTangent(1,0,0));
  if(X<NX-1&&Y<NY-1){int I=Y*NX+X;Indices.Append({I,I+NX,I+1,I+1,I+NX,I+NX+1});}
 }
 auto* Base=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Production/Materials04/M04_Ground_%d.M04_Ground_%d"),Chapter,Chapter));if(!Base)Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Production/Materials/M_GroundRegional.M_GroundRegional"));auto* GroundMat=UMaterialInstanceDynamic::Create(Base,this);GroundMat->SetScalarParameterValue(TEXT("TerrainType"),Chapter);Materials.Add(GroundMat);
 Mesh->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,Tangents,true);Mesh->SetMaterial(0,GroundMat);
 // Continue the landscape beneath the distant rocks, beyond the playable boundary.
 V.Reset();N.Reset();UV.Reset();Indices.Reset();Colors.Reset();Tangents.Reset();
 const int Outer=129;
 for(int Y=0;Y<Outer;Y++)for(int X=0;X<Outer;X++){
  FVector P(-32000+X*500,-32000+Y*500,0);P.Z=TerrainSurfaceHeight(P);V.Add(P);N.Add(FVector::UpVector);UV.Add(FVector2D(P.X/500,P.Y/500));Colors.Add(FLinearColor(.65,.70,.57,0));Tangents.Add(FProcMeshTangent(1,0,0));
  if(X<Outer-1&&Y<Outer-1){bool Inside=P.X>=-10500&&P.X+500<=10500&&P.Y>=-7500&&P.Y+500<=7500;if(!Inside){int I=Y*Outer+X;Indices.Append({I,I+Outer,I+1,I+1,I+Outer,I+Outer+1});}}
 }
 auto* Horizon=NewObject<UProceduralMeshComponent>(A);A->AddInstanceComponent(Horizon);Horizon->SetupAttachment(Mesh);Horizon->SetMobility(EComponentMobility::Static);Horizon->SetCastShadow(false);Horizon->SetCollisionEnabled(ECollisionEnabled::NoCollision);Horizon->RegisterComponent();Horizon->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,Tangents,false);Horizon->SetMaterial(0,GroundMat);

 if(Chapter==1){
  auto* W=GetWorld()->SpawnActor<AActor>();auto* Water=NewObject<UProceduralMeshComponent>(W);W->SetRootComponent(Water);W->AddInstanceComponent(Water);Water->RegisterComponent();Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);Water->SetCastShadow(false);WorldActors.Add(W);
  V.Reset();N.Reset();UV.Reset();Indices.Reset();Colors.Reset();Tangents.Reset();
  const int Across=17,Along=641;
  for(int Y=0;Y<Along;Y++)for(int X=0;X<Across;X++){
   float WY=-32000+Y*100,L=float(X)/(Across-1)*2-1,Width=RiverHalfWidth(WY)+350;
   V.Add(FVector(RiverCenter(WY)+L*Width,WY,-130));N.Add(FVector::UpVector);UV.Add(FVector2D(L,WY/400));
   Colors.Add(FLinearColor(FMath::Clamp((FMath::Abs(L)-.4f)/.6f,0.f,1.f),0,0,1));Tangents.Add(FProcMeshTangent(1,0,0));
   if(X<Across-1&&Y<Along-1){int I=Y*Across+X;Indices.Append({I,I+1,I+Across,I+1,I+Across+1,I+Across});}
  }
  // Water is viewed from above; winding points toward +Z.
  for(int I=0;I<Indices.Num();I+=3)Swap(Indices[I+1],Indices[I+2]);
  Water->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,Tangents,false);Water->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Production/Materials04/M04_River.M04_River")));
 }
}

void AExtractionMode::BuildSectorLandmarks(){
 if(Chapter==1){
  BuildingSector=false;Asset(TEXT("RiverBridge"),FVector(0,-2800,0),FVector(1),FRotator::ZeroRotator,true);BuildingSector=true;
  FVector Hinge(-1660,-2565,0);
  Asset(TEXT("BridgeGateFrame"),Hinge,FVector(1),FRotator::ZeroRotator,true);
  BridgeGate=Asset(TEXT("BridgeGateLeaf"),Hinge,FVector(1),FRotator::ZeroRotator,true);
  if(BridgeOpen&&BridgeGate)BridgeGate->SetActorRotation(FRotator(0,-105,0));
  for(float X:{-1800.f,1800.f})for(int I=0;I<7;I++){float Y=-3800+I*250;if(FMath::Abs(Y+2800)<400)continue;Asset(TEXT("wooden_crate_02"),FVector(X,Y,0),FVector(.9),FRotator(0,I*23,0),true);}
 }
 if(Chapter==0){
  for(int I:{0,2,4,6}){FVector P=SectorNodes[I]+FVector(750,-750,0);Asset(I%4==0?TEXT("CableReel"):TEXT("QuarryCompressor"),P,FVector(.8),FRotator(0,I*32,0),true);}
 }
 if(Chapter==1){
  for(float Y:{-5700.f,1300.f,5000.f}){float X=RiverCenter(Y)-RiverHalfWidth(Y)+100;FVector P(X,Y,-75);BuildingSector=false;Asset(TEXT("RiverDock"),P,FVector(1),FRotator(0,0,0),false);Asset(TEXT("FishingCanoe"),P+FVector(310,190,-42),FVector(1),FRotator(0,80,0),false);BuildingSector=true;Asset(TEXT("FishingNetRack"),FVector(X-250,Y+400,0),FVector(1),FRotator(0,30,0),true);}
  for(int I=0;I<65;I++){float Y=Random.FRandRange(-6900,6900);if(FMath::Abs(Y+2800)<850)continue;float X=RiverCenter(Y)+(I%2?1:-1)*(RiverHalfWidth(Y)+Random.FRandRange(30,160));Asset(FString::Printf(TEXT("rock_moss_set_01_%d"),I%6),FVector(X,Y,-35),FVector(.32,.38,.22),FRotator(0,I*29,0),false);for(int J=0;J<3;J++)Asset(FString::Printf(TEXT("fern_02_%d"),J%4),FVector(X+J*50,Y+90,0),FVector(.8),FRotator(0,J*71,0));}
 }
 if(Chapter==2){
  const FVector Houses[]={{-5700,-700,0},{-5600,-2100,0},{-2600,-3600,0},{-1900,-2600,0},{-2100,600,0},{-1700,1700,0},{2100,3200,0},{3100,4000,0},{6100,800,0},{6400,2100,0}};
  for(int I=0;I<10;I++){FVector P=Houses[I];FRotator R(0,I%3*90,0);Asset(I%3==0?TEXT("VillageStore"):I%3==1?TEXT("StiltHouse"):TEXT("SupplyShed"),P,FVector(I%3==0?1:.9),R,true);Asset(I%3==0?TEXT("Clothesline"):I%3==1?TEXT("MarketStall"):TEXT("FishingNetRack"),P+FVector(100,-460,0),FVector(1),R,true);}
  Asset(TEXT("VillageWell"),FVector(500,2800,0),FVector(1.2),FRotator::ZeroRotator,true);
  for(int I=0;I<4;I++)Asset(TEXT("MarketStall"),FVector(-2300+I*950,-1050+(I%2?130:-80),0),FVector(.90+I*.035,1,1),FRotator(0,(I%2?180:0)+(I-1)*7,0),true);
 }
 if(Chapter==3){
  for(int I=0;I<10;I++){float A=I*2*PI/10;FVector P(400+FMath::Cos(A)*2400,FMath::Sin(A)*2000,0);Asset(FString::Printf(TEXT("rock_moss_set_01_%d"),I%6),P,FVector(3.3,2.2,1.6),FRotator(0,I*53,0));}
  for(int I=0;I<7;I++){FVector P=SectorNodes[I]+FVector(850,-650,0);Asset(I%3==0?TEXT("MineCart"):I%3==1?TEXT("QuarryCompressor"):TEXT("CableReel"),P,FVector(1),FRotator(0,I*47,0),true);}
 }
 if(Chapter==4){
  for(float X:{-4800.f,4800.f})for(int I=0;I<14;I++)Asset(TEXT("FortressWall"),FVector(X,-5200+I*800,0),FVector(1),FRotator(0,90,0),true);
  for(float Y:{-5700.f,-600.f,3000.f,6800.f})for(int I=-5;I<=5;I++){
   float X=I*800;if(FMath::Abs(X)<700||FMath::Abs(X-2400)<700||FMath::Abs(X+2400)<700)continue;
   Asset(TEXT("FortressWall"),FVector(X,Y,0),FVector(1),FRotator::ZeroRotator,true);
  }
  Asset(TEXT("RadarArray"),SectorNodes[0]+FVector(650,0,0),FVector(1.3),FRotator(0,20,0),true);
  for(int I:{1,4})Asset(TEXT("AirDefenseLauncher"),SectorNodes[I]+FVector(-780,0,0),FVector(1.15),FRotator(0,I*30,0),true);
 }
 if(Chapter==5){
  for(int I=0;I<3;I++){Asset(TEXT("FieldHangar"),FVector(-6200+I*4700,-6100,0),FVector(1),FRotator(0,90,0),true);Asset(TEXT("FuelStation08"),FVector(-5900+I*4700,-3200,0),FVector(1),FRotator(0,90,0),true);}
  for(int I=0;I<20;I++){float X=-8800+I*920;Shape(FVector(X,0,4),FVector(3,.22,.025),FLinearColor(.61,.59,.47),0,false);for(float Y:{-1140.f,1140.f})Shape(FVector(X,Y,30),FVector(.13,.13,.28),FLinearColor(1,.55,.12),2,false);}
  for(int I=0;I<8;I++){float A=I*PI/4;FVector P=Exit+FVector(FMath::Cos(A)*1100,FMath::Sin(A)*1100,0);Shape(P+FVector(0,0,25),FVector(.22,.22,.4),FLinearColor(.73,.45,.12),2,false);}
 }

}
void AExtractionMode::UpdateSector(float Delta){
 if(IsValid(BridgeGate)){
  float Yaw=FMath::FInterpConstantTo(BridgeGate->GetActorRotation().Yaw,BridgeOpen?-105.f:0.f,Delta,75.f);
  BridgeGate->SetActorRotation(FRotator(0,Yaw,0));
 }
}
