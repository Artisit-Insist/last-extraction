#include "Extraction.h"
#include "ProceduralMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

void AExtractionMode::ConfigureAtmosphere(){
 const FRotator Angles[6]={{-32,-40,0},{-53,25,0},{-24,120,0},{-48,-95,0},{-16,35,0},{-11,-80,0}};
 const FLinearColor SunColors[6]={{.8,.92,.8},{.91,.95,1},{1,.69,.40},{.83,.87,.94},{1,.58,.32},{1,.57,.33}};
 const FLinearColor FogColors[6]={{.12,.21,.15},{.20,.30,.30},{.27,.23,.17},{.25,.25,.24},{.16,.20,.23},{.28,.20,.17}};
 const float SunStrength[6]={3.4,4.5,4.1,4.8,3.6,3.8},FogDensity[6]={.025,.012,.009,.006,.017,.008};
 if(SectorSun){SectorSun->SetActorRotation(Angles[Chapter]);SectorSun->GetLightComponent()->SetLightColor(SunColors[Chapter]);SectorSun->GetLightComponent()->SetIntensity(SunStrength[Chapter]);}
 if(SectorSky)SectorSky->GetLightComponent()->SetIntensity(Chapter==0?.75:Chapter==4?.82:1.0);
 if(SectorFog){SectorFog->GetComponent()->SetFogDensity(FogDensity[Chapter]);SectorFog->GetComponent()->SetFogInscatteringColor(FogColors[Chapter]);}
}

void AExtractionMode::BuildObjectiveSite(FVector P,int Node,int Kind){
 TArray<FString> Placed;
 auto Place=[&](const TCHAR* Name,float X,float Y,float Yaw=0,float Scale=1){if(Asset(Name,P+FVector(X,Y,0),FVector(Scale),FRotator(0,Yaw,0),true))Placed.Add(Name);};
 // Preserve the central interaction area. Each site has a purpose and an open approach.
 if(Chapter==0){
  switch(Node){
   case 0:Place(TEXT("CommandTable08"),-520,480,25);Place(TEXT("Watchtower"),800,620,25);break;
   case 1:Place(TEXT("JungleBivouac08"),720,660,-20);Place(TEXT("FieldGenerator"),-580,510,70,.8);break;
   case 2:Place(TEXT("CableReel"),-520,460,90);Place(TEXT("CommandTable08"),690,520,180);break;
   case 3:Place(TEXT("BurntShelter08"),-720,580,15);Place(TEXT("FieldClinic08"),750,650,-35);break;
   case 4:Place(TEXT("BoatRepair08"),640,560,70);Place(TEXT("FishingNetRack"),-580,510,-30);break;
   case 5:Place(TEXT("QuarryCompressor"),-560,530,15);Place(TEXT("CableReel"),600,-460,90);Place(TEXT("Watchtower"),880,670,90);break;
   case 6:Place(TEXT("JungleBivouac08"),-790,610,20);Place(TEXT("Sandbags"),640,430,-25);Place(TEXT("CommandTable08"),550,-520,90);break;
  }
 }else if(Chapter==1){
  switch(Node){
   case 0:Place(TEXT("CommandTable08"),-550,520,0);Place(TEXT("FishingNetRack"),580,490,50);break;
   case 1:Place(TEXT("BoatRepair08"),-610,560,-20);Place(TEXT("StiltHouse"),800,780,90);break;
   case 2:Place(TEXT("CableReel"),-540,500,25);Place(TEXT("QuarryWorkbench08"),680,570,110,.85);break;
   case 3:Place(TEXT("BridgeWinch"),-680,580,90);Place(TEXT("Sandbags"),-520,-500,90);break;
   case 4:Place(TEXT("BurntShelter08"),780,640,-30);Place(TEXT("barrel_03"),-580,460,0,1.2);break;
   case 5:Place(TEXT("FishingNetRack"),-590,480,60);Place(TEXT("BoatRepair08"),670,620,0);Place(TEXT("VillageKitchen08"),-690,-570,20);break;
   case 6:Place(TEXT("CommandTable08"),-540,490,0);Place(TEXT("Watchtower"),750,650,-20);Place(TEXT("CableReel"),560,-470,30);break;
  }
 }else if(Chapter==2){
  switch(Node){
   case 0:Place(TEXT("VillageStore"),-830,700,90);Place(TEXT("MarketStall"),710,550,165);break;
   case 1:Place(TEXT("VillageKitchen08"),-600,540,0);Place(TEXT("Clothesline"),780,600,100);Place(TEXT("StiltHouse"),-850,-790,0,.85);break;
   case 2:Place(TEXT("MarketStall"),-710,510,15);Place(TEXT("MarketStall"),730,580,170);Place(TEXT("VillageWell"),660,-600,0,.85);break;
   case 3:Place(TEXT("BurntShelter08"),740,630,20);Place(TEXT("Sandbags"),-610,440,25);break;
   case 4:Place(TEXT("FieldClinic08"),-790,660,0);Place(TEXT("VillageKitchen08"),640,610,-90);break;
   case 5:Place(TEXT("BurntShelter08"),-850,650,80);Place(TEXT("MarketStall"),730,-660,-20);break;
   case 6:Place(TEXT("VillageStore"),820,740,180);Place(TEXT("CommandTable08"),-570,500,15);break;
  }
 }else if(Chapter==3){
  switch(Node){
   case 0:Place(TEXT("QuarryWorkbench08"),-650,550,0);Place(TEXT("CableReel"),600,470,60);break;
   case 1:Place(TEXT("FieldClinic08"),-770,680,20);Place(TEXT("MineCart"),780,560,90);break;
   case 2:Place(TEXT("QuarryCrane"),-980,760,45,.75);Place(TEXT("QuarryWorkbench08"),740,610,-40);break;
   case 3:Place(TEXT("CommandTable08"),-580,540,-15);Place(TEXT("QuarryCompressor"),680,460,80);break;
   case 4:Place(TEXT("JungleBivouac08"),810,690,15);Place(TEXT("MineCart"),-770,610,110);break;
   case 5:Place(TEXT("QuarryCrane"),-980,760,45,.75);Place(TEXT("BurntShelter08"),830,650,-20);break;
   case 6:Place(TEXT("QuarryWorkbench08"),-710,600,0);Place(TEXT("CableReel"),560,470,90);Place(TEXT("Sandbags"),-560,-510,90);break;
  }
 }else if(Chapter==4){
  switch(Node){
   case 0:Place(TEXT("CommandTable08"),-560,500,10);Place(TEXT("ConcreteBunker"),850,730,180);break;
   case 1:Place(TEXT("AirCargoTrolley08"),700,550,90);Place(TEXT("Sandbags"),-600,430,10);break;
   case 2:Place(TEXT("FieldGenerator"),-620,550,90);Place(TEXT("CableReel"),560,470,0);break;
   case 3:Place(TEXT("FieldClinic08"),-770,720,0);Place(TEXT("ConcreteBunker"),850,750,180);break;
   case 4:Place(TEXT("AirCargoTrolley08"),660,630,30);Place(TEXT("CommandTable08"),-580,500,0);break;
   case 5:Place(TEXT("BurntShelter08"),-760,680,-20);Place(TEXT("Sandbags"),660,490,90);break;
   case 6:Place(TEXT("ConcreteBunker"),850,720,180);Place(TEXT("CommandTable08"),-630,530,0);Place(TEXT("Sandbags"),-560,-530,90);break;
  }
 }else{
  switch(Node){
   case 0:Place(TEXT("FieldClinic08"),-810,720,0);Place(TEXT("AirCargoTrolley08"),780,570,90);break;
   case 1:Place(TEXT("FuelStation08"),-840,680,0);Place(TEXT("QuarryWorkbench08"),750,620,20);break;
   case 2:Place(TEXT("AirCargoTrolley08"),-780,600,90);Place(TEXT("JungleBivouac08"),830,710,10);break;
   case 3:Place(TEXT("AirCargoTrolley08"),760,650,10);Place(TEXT("BurntShelter08"),-810,720,30);break;
   case 4:Place(TEXT("FuelStation08"),-810,700,0);Place(TEXT("CableReel"),550,-490,90);break;
   case 5:Place(TEXT("FieldClinic08"),-820,700,0);Place(TEXT("CommandTable08"),690,510,180);break;
   case 6:Place(TEXT("CommandTable08"),-640,540,0);Place(TEXT("AirCargoTrolley08"),780,660,90);Place(TEXT("Sandbags"),-640,-570,90);break;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("SITE_LAYOUT08 chapter=%d node=%d kind=%d props=%s"),Chapter,Node,Kind,*FString::Join(Placed,TEXT(",")));
}

void AExtractionMode::BuildRouteSurface(){
 auto* Actor=GetWorld()->SpawnActor<AActor>();auto* Mesh=NewObject<UProceduralMeshComponent>(Actor);Actor->SetRootComponent(Mesh);Actor->AddInstanceComponent(Mesh);Mesh->RegisterComponent();Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);WorldActors.Add(Actor);
 TArray<FVector> V,N;TArray<FVector2D> UV;TArray<int32> Indices;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
 auto Road=[&](FVector A,FVector B,float Width,FLinearColor Tint){
  const FVector Side=FVector::CrossProduct((B-A).GetSafeNormal2D(),FVector::UpVector);int Steps=FMath::CeilToInt(FVector::Dist2D(A,B)/100),Base=V.Num();
  for(int I=0;I<=Steps;I++){FVector C=FMath::Lerp(A,B,float(I)/Steps);for(int J=0;J<2;J++){FVector P=C+Side*((J?1:-1)*Width*.5f);P.Z=TerrainSurfaceHeight(P)+2;V.Add(P);N.Add(FVector::UpVector);UV.Add(FVector2D(P.X/350,P.Y/350));Colors.Add(Tint);Tangents.Add(FProcMeshTangent(1,0,0));}
   if(I<Steps){int K=Base+I*2;Indices.Append({K,K+2,K+1,K+1,K+2,K+3});}
  }
 };
 FLinearColor Dirt(.24,.18,.105),Gravel(.22,.21,.18),Concrete(.18,.19,.18);
 if(Chapter==2){for(float Y:{-4900.f,-1800.f,1900.f,5200.f})Road(FVector(-8000,Y,0),FVector(8000,Y,0),550,Dirt);for(float X:{-7600.f,-4000.f,0.f,4000.f,7600.f})Road(FVector(X,-5600,0),FVector(X,6000,0),520,Dirt);}
 else if(Chapter==5){Road(FVector(-9400,0,0),FVector(9500,0,0),2300,Concrete);Road(FVector(-7600,2600,0),FVector(8900,2600,0),650,Gravel);Road(FVector(-7600,-2600,0),FVector(8200,-2600,0),650,Gravel);for(float X:{-7000.f,0.f,6500.f})Road(FVector(X,-2800,0),FVector(X,3000,0),800,Concrete);}
 else if(Chapter==4){for(float X:{-2000.f,2200.f})Road(FVector(X,-6200,0),FVector(X,6200,0),600,Gravel);for(float Y:{-5200.f,-800.f,3000.f,5800.f})Road(FVector(-4000,Y,0),FVector(4000,Y,0),600,Gravel);}
 else if(Chapter!=1){FVector A=SectorStart;for(FVector B:SectorNodes){Road(A,B,Chapter==3?700:310,Chapter==3?Gravel:Dirt);A=B;}Road(A,Exit,Chapter==3?700:310,Chapter==3?Gravel:Dirt);}
 Mesh->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,Tangents,false);Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Production/Materials/M_Road.M_Road")));
}
