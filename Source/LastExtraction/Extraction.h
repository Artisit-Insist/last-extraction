#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "Extraction.generated.h"
class USkeletalMeshComponent; class UAnimationAsset; class UStaticMeshComponent; class USphereComponent; class UCapsuleComponent; class UMaterialInstanceDynamic; class UCameraComponent; class USoundWave; class UAudioComponent;
class ADirectionalLight; class ASkyLight; class AExponentialHeightFog;
UCLASS()
class AExtractionPawn : public APawn {
 GENERATED_BODY()
public:
 AExtractionPawn();
 UPROPERTY() UCapsuleComponent* Body;
 UPROPERTY() UCameraComponent* Camera;
 UPROPERTY() USceneComponent* Visual;
 UPROPERTY() USkeletalMeshComponent* CharacterMesh;
 UPROPERTY() TArray<UStaticMeshComponent*> Parts;
 UPROPERTY() TArray<UStaticMeshComponent*> RifleParts;
 UPROPERTY() TArray<UStaticMeshComponent*> BowParts;
 float Health=100, Stamina=100, Invincible=0;
 virtual void BeginPlay() override;
 void Animate(float Time,float Speed);
};
enum class EEnemyTactic : uint8 { Patrol, Search, Advance, Flank, Hide, Peek, Hold };
USTRUCT()
struct FExtractionEnemy {
 GENERATED_BODY()
 UPROPERTY() AActor* Actor=nullptr;
 FVector Home=FVector::ZeroVector;
 float Health=80,Cooldown=1,Alert=0,Seed=0;
 int Type=0;
 EEnemyTactic Tactic=EEnemyTactic::Patrol;
 FVector LastKnown=FVector::ZeroVector,Goal=FVector::ZeroVector,Cover=FVector::ZeroVector,Peek=FVector::ZeroVector;
 TArray<FVector> Waypoints;
 float Think=0,StateTime=0,StateAge=0,Repath=0,Suppression=0,LostSight=30;
 int Identity=0,Burst=0,CoverCycles=0;
 bool HasContact=false;
};
USTRUCT()
struct FExtractionObjective {
 GENERATED_BODY()
 FVector Position=FVector::ZeroVector,Origin=FVector::ZeroVector,Rally=FVector::ZeroVector;
 FString Name,Description;
 int Stage=0;
 float EscortRepath=0;
 bool EscortUnderFire=false;
 TArray<FVector> EscortRoute;
 UPROPERTY() AActor* RallyMarker=nullptr;
 int Kind=0;
 bool Done=false;
 UPROPERTY() AActor* Marker=nullptr;
 UPROPERTY() AActor* Prop=nullptr;
};
USTRUCT()
struct FExtractionShot {
 GENERATED_BODY()
 FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector;
 float Life=4,Damage=8;
 UPROPERTY() AActor* Visual=nullptr;
};
USTRUCT()
struct FThrownGrenade {
 GENERATED_BODY()
 FVector Origin=FVector::ZeroVector,Target=FVector::ZeroVector;
 float Time=0;
 UPROPERTY() AActor* Actor=nullptr;
};
USTRUCT()
struct FBlastEffect {
 GENERATED_BODY()
 UPROPERTY() AActor* Actor=nullptr;
 UPROPERTY() UMaterialInstanceDynamic* Material=nullptr;
 float Age=0;
};
USTRUCT()
struct FPendingWeaponDrop {
 GENERATED_BODY()
 UPROPERTY() UStaticMeshComponent* Weapon=nullptr;
 FVector Direction=FVector::ZeroVector;
 float Delay=.22f;
};
UCLASS()
class AExtractionMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AExtractionMode();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 UPROPERTY() AExtractionPawn* Hero;
 UPROPERTY() TArray<FExtractionEnemy> Enemies;
 UPROPERTY() TArray<FExtractionObjective> Objectives;
 UPROPERTY() TArray<FExtractionShot> Shots;
 UPROPERTY() TArray<FThrownGrenade> Thrown;
 UPROPERTY() TArray<AActor*> WorldActors;
 UPROPERTY() TArray<AActor*> Effects;
 UPROPERTY() TArray<UMaterialInstanceDynamic*> Materials;
 UPROPERTY() UStaticMesh* Cube;
 UPROPERTY() UStaticMesh* Sphere;
 UPROPERTY() UStaticMesh* Cylinder;
 UPROPERTY() UStaticMesh* Cone;
 UPROPERTY() TArray<UStaticMesh*> Detailed;
 UPROPERTY() TMap<FString,UStaticMesh*> ProductionMeshes;
 UPROPERTY() TArray<UAnimationAsset*> CharacterAnimations;
 UPROPERTY() UAudioComponent* MusicPlayer;
 UPROPERTY() TArray<UAudioComponent*> MusicLayers;
 UPROPERTY() TArray<FBlastEffect> Blasts;
 UPROPERTY() TArray<FPendingWeaponDrop> PendingWeaponDrops;
 UPROPERTY() TArray<AActor*> DroppedWeapons;
 UPROPERTY() TMap<AActor*,FVector> WeaponDropKicks;
 int FallFrame=0,FallStage=-1,WeaponDrops=0;
 float FallPreviewTime=0;
 void StartCharacterFall(AActor* Actor,FVector ImpactDirection);
 void UpdateWeaponDrops(float Delta);
 void BuildFallPreview();
 bool UpdateFallPreview(float Delta);
 float MusicGains[4]={0,0,0,0};
 void BlastVisual(FVector Position,float Scale=1);
 UPROPERTY() UAudioComponent* AmbiencePlayer;
 float MusicVolume=.5f,EffectsVolume=.7f;
 UPROPERTY() UTexture2D* KeyArt;
 UPROPERTY() UTexture2D* Equipment;
 UPROPERTY() TArray<USoundWave*> Sounds;
 FVector Aim=FVector(1000,0,90),Exit=FVector(8500,0,0),Checkpoint=FVector(-8400,0,100);
 int Chapter=0,Weapon=0,Ammo=30,Reserve=210,Arrows=24,Grenades=5,Medkits=3,Kills=0,Difficulty=1,SavedChapter=0,SavedMask=0;
 int Completed=0,Focused=-1;
 TArray<int> SavedStages;
 TArray<FVector> SavedEscortPositions;
 FVector SavedHeroPosition=FVector::ZeroVector;
 bool SavedHeroPositionValid=false;
 bool Menu=true,Paused=false,Dead=false,Victory=false,Map=false,HasSave=false,Autotest=false;
 float Elapsed=0,LocalTime=0,FireCD=0,Reload=0,Interact=0,Defense=0,ToastTime=0,Alarm=0,HitFlash=0,DashCD=0,DashTime=0;
 int RigFrame=0,RigLastStage=-1,CombatFrame=0;
 float CombatRecordStart=-1;
 float EscortRecordStart=-1,EscortRecordStop=-1;
 int EscortFrame=0;
 float CombatAttentionUntil=-1;
 FVector CombatAttention=FVector::ZeroVector;
 FVector DashDirection=FVector::ZeroVector;
 FString Toast,Hint,Status;
 TArray<FString> ChapterNames,Briefs;
 FRandomStream Random;
 FVector PrevHero,MoveTarget;
 bool ClickMoving=false;
 TArray<FVector> Route;
 bool Playtest=false;
 TSet<FKey> TestPressed,TestHeld;
 FVector TestAim,TestMove;
 float TestRepath=0,TestElapsed=0,TestProgress=0;
 int TestDeaths=0,TestLastCompleted=-1,TestLastStage=-1;
 bool FindRoute(FVector Destination);
 bool FindRouteFor(AActor* Mover,FVector Destination,TArray<FVector>& Result);
 FVector FollowRoute();
 void UpdatePlaytest(float Delta);
 UPROPERTY() UTexture2D* CharacterArt;
 UPROPERTY() TArray<AActor*> Followers;
 TArray<TArray<FVector>> AllyRoutes;
 TArray<float> AllyRepaths,AllyCooldowns;
 int AllyShots=0,RescuedTotal=0;
 void UpdateCompanions(float Delta);
 bool MoveCompanion(AActor* Person,FVector Destination);
 UPROPERTY() ADirectionalLight* SectorSun=nullptr;
 UPROPERTY() ASkyLight* SectorSky=nullptr;
 UPROPERTY() AExponentialHeightFog* SectorFog=nullptr;
 UPROPERTY() TArray<AActor*> Canopies;
 AActor* Asset(const FString& Name,FVector Position,FVector Scale=FVector(1),FRotator Rotation=FRotator::ZeroRotator,bool Collision=false);
 void AnimateCharacter(USkeletalMeshComponent* Mesh,float Speed);
 TArray<FVector> SectorNodes;
 FVector SectorStart=FVector(-9200,0,0);
 bool BuildingSector=false,BridgeOpen=true;
 UPROPERTY() AActor* BridgeGate=nullptr;
 void ConfigureSector();void BuildTerrain();void BuildSectorLandmarks();void UpdateSector(float Delta);
 void BuildObjectiveSite(FVector Position,int Node,int Kind);
 void BuildRouteSurface();void ConfigureAtmosphere();
 float TerrainPathWeight(FVector P)const;float RiverCenter(float Y)const;float RiverHalfWidth(float Y)const;float RouteDistance(FVector P)const;
 float GroundHeight(FVector Position)const;
 float RawGroundHeight(FVector Position)const;
 float TerrainSurfaceHeight(FVector Position)const;
 void MoveOnTerrain(AActor* Actor,FVector Delta,FHitResult* Hit=nullptr);
 bool TerrainWalkable(FVector Position)const;
 bool TerrainSegmentClear(FVector Start,FVector End)const;
 FVector RescueRallyPoint(int Index) const;
 void BuildEscortSites();void BeginEscort(int Index,bool Restoring=false);void UpdateEscorts(float Delta);
 int ActiveEscort() const;bool EscortReady(int Index) const;
 void UpdateEscortRecording(float Delta);void RecordEscortSound(int Index,FVector Position,float Volume);
 void Start(bool Continue); void BuildChapter(int Mask=0,bool RestoreStages=false); void Save(); void ReadSave();
 void Shoot(); void Grenade(); void Explode(FVector P); void UseMedkit(); void DamageHero(float Damage,FVector ImpactDirection=FVector::ZeroVector); void KillEnemy(int Index,FVector ImpactDirection=FVector::ZeroVector); void CompleteObjective(int Index);
 void SpawnEnemy(FVector P,int Type=0); void MakeSoldier(AActor* Actor,bool Player=false,int Type=0);
 void SpawnSiteGuards(FVector Position,int Node,int Kind);
 void BuildEnemyCover();void UpdateEnemies(float Delta,FVector HeroVelocity,bool RigPreview);
 bool EnemySegmentClear(AActor* Person,FVector From,FVector To) const;
 bool SelectEnemyCover(FExtractionEnemy& Enemy,FVector Threat);
 void HearCombatNoise(FVector Position,float Radius);
 void SetEnemyTactic(FExtractionEnemy& Enemy,EEnemyTactic Tactic,float Duration=0);
 TArray<FBox> EnemyCoverBounds;
 float NextEnemyPathTime=0;
 int EnemyTacticTransitions=0,EnemyCoverSelections=0,EnemyFlanks=0,EnemySearches=0,EnemyPathCursor=0;
 void Flash(FVector P,FLinearColor Color,float Size=.35,float Life=.12);
 void Beam(FVector A,FVector B,FLinearColor Color,float Life=.09);
 void Sound(int Index,FVector P,float Volume=1);
 void Notify(const FString& Message,float Time=4);
 AActor* Shape(FVector P,FVector Scale,FLinearColor Color,int Mesh=0,bool Collision=true,FRotator Rotation=FRotator::ZeroRotator,bool Track=true);
 UStaticMeshComponent* Part(AActor* Owner,USceneComponent* Parent,FVector P,FVector Scale,FLinearColor Color,int Mesh=0);
 UMaterialInstanceDynamic* Material(FLinearColor Color,bool Glow=false,float Rough=.7,float Metal=0);
 bool CanSee(FVector A,FVector B,AActor* Ignore=nullptr) const;
 bool Ending=false;
 float EndingTime=0;
 int EndingFrame=0,Boarded=0;
 UPROPERTY() AActor* Helicopter=nullptr;
 UPROPERTY() AActor* MainRotor=nullptr;
 UPROPERTY() AActor* TailRotor=nullptr;
 UPROPERTY() TArray<AActor*> HelicopterDoors;
 UPROPERTY() TArray<AActor*> ExtractionParty;
 UPROPERTY() UAudioComponent* RotorAudio=nullptr;
 FVector LandingPosition=FVector::ZeroVector;
 void BeginExtraction();void UpdateExtraction(float Delta);
};
UCLASS()
class AExtractionHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 UPROPERTY() UFont* KoreanFont;
 void Text(const FString& S,float X,float Y,float Size,FLinearColor Color=FLinearColor::White);
 void Box(float X,float Y,float W,float H,FLinearColor C);
 void Image(UTexture2D* Texture,float X,float Y,float W,float H,float U=0,float V=0,float UW=1,float VH=1);
};
