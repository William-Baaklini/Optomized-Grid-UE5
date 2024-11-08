// Copyright Rekt Studios. All Rights Reserved.

#include "GridManager.h"

#include "Kismet/KismetMathLibrary.h"
#include "ProceduralMeshComponent.h"
// #include "CryptoArena/CryptoArenaGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "OptimizedGrid/OptimizedGridGameMode.h"

AGridManager::AGridManager()
{
	// Actor never participate in Tick
	PrimaryActorTick.bCanEverTick = false;

	// Creating Components
	const TObjectPtr<USceneComponent> DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	RootComponent = DefaultSceneComponent;

	LineMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Lines Mesh"));
	LineMesh->SetupAttachment(RootComponent);
	LineMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.4f));
	
	SelectionMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Selection Mesh"));
	SelectionMesh->SetupAttachment(RootComponent);
	SelectionMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.3f));

	NoWalkMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("No Walk Mesh"));
	NoWalkMesh->SetupAttachment(RootComponent);
	NoWalkMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.2f));

	NoSpawnMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("No Spawn Mesh"));
	NoSpawnMesh->SetupAttachment(RootComponent);
	NoSpawnMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.1f));
	
	// Setting up Defaults
	NumRows = 10;
	NumColumns = 10;
	TileSize = 100.0f;
	LineThickness = 10.0f;
	LineOpacity = 1.0f;
	SelectionOpacity = 0.35f;
	LineColor = FLinearColor::Green;
	SelectionColor = FLinearColor::White;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Script/Engine.Material'/Game/Art/Materials/M_Grid.M_Grid'"));
	
	if(Material.Object != nullptr)
	{
		MaterialInterface = Cast<UMaterialInterface>(Material.Object);
	}

	bStartingModifiersInitialized = false;
}

void AGridManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Create material instances for the meshes
	const TObjectPtr<UMaterialInstanceDynamic> LinesMaterial = CreateMaterialInstance(LineColor, LineOpacity);
	const TObjectPtr<UMaterialInstanceDynamic> SelectionMaterial = CreateMaterialInstance(SelectionColor, SelectionOpacity);
	const TObjectPtr<UMaterialInstanceDynamic> NoWalkMaterial = CreateMaterialInstance(NoWalkColor, NoWalkOpacity);
	const TObjectPtr<UMaterialInstanceDynamic> NoSpawnMaterial = CreateMaterialInstance(NoSpawnColor, NoSpawnOpacity);

	TArray<FVector> LinesVertices;
	TArray<int> LinesTriangles;

	// Create vertices and triangles for horizontal lines
	for (int i = 0; i < NumRows + 1; ++i)
	{
		const float LineStart = TileSize * i;
		const float LineEnd = GetGridWidth();
		CreateLine(FVector(LineStart, 0.0f, 0.0f), FVector(LineStart, LineEnd, 0.0f), LineThickness, LinesVertices, LinesTriangles);
	}

	// Create vertices and triangles for vertical lines
	for (int i = 0; i < NumColumns + 1; ++i)
	{
		const float LineStart = TileSize * i;
		const float LineEnd = GetGridHeight();
		CreateLine(FVector(0.0f, LineStart, 0.0f), FVector(LineEnd, LineStart, 0.0f), LineThickness, LinesVertices, LinesTriangles);
	}

	// Create the lines mesh from vertices and triangles and set material
	CreateMeshSection(LineMesh, LinesVertices, LinesTriangles);
	LineMesh->SetMaterial(0, LinesMaterial);

	TArray<FVector> SelectionVertices;
	TArray<int> SelectionTriangles;

	// Create a selection tile mesh and set material
	CreateLine(FVector(0.0f, TileSize/2, 0.0f), FVector(TileSize, TileSize/2, 0.0f), TileSize, SelectionVertices, SelectionTriangles);
	CreateMeshSection(SelectionMesh, SelectionVertices, SelectionTriangles);
	SelectionMesh->SetMaterial(0, SelectionMaterial);
	SelectionMesh->SetVisibility(false);
	
	// Create modifiers mesh and set material
	TArray<FVector> NoWalkVertices;
	TArray<int> NoWalkTriangles;
	for (const auto TileMod : NoWalkingStartingTiles)
	{
		bool bValid;
		
		FVector StartingLocation = TileToGridLocation(TileMod.Row, TileMod.Column, bValid, false) - GetActorLocation();
		if(!bValid) continue;
		
		CreateLine(FVector(StartingLocation.X, StartingLocation.Y + TileSize/2.0f, 0.0f), FVector(StartingLocation.X + TileSize, StartingLocation.Y + TileSize/2.0f, 0.0f), TileSize, NoWalkVertices, NoWalkTriangles);
	}
	
	CreateMeshSection(NoWalkMesh, NoWalkVertices, NoWalkTriangles);
	NoWalkMesh->SetMaterial(0, NoWalkMaterial);
	
	TArray<FVector> NoSpawnVertices;
	TArray<int> NoSpawnTriangles;
	for (const auto TileMod : NoSpawningStartingTiles)
	{
		bool bValid;
		
		FVector StartingLocation = TileToGridLocation(TileMod.Row, TileMod.Column, bValid, false) - GetActorLocation();
		if(!bValid) continue;
		
		CreateLine(FVector(StartingLocation.X, StartingLocation.Y + TileSize/2, 0.0f), FVector(StartingLocation.X + TileSize, StartingLocation.Y + TileSize/2, 0.0f), TileSize, NoSpawnVertices, NoSpawnTriangles);
	}
	
	CreateMeshSection(NoSpawnMesh, NoSpawnVertices, NoSpawnTriangles);
	NoSpawnMesh->SetMaterial(0, NoSpawnMaterial);
}

void AGridManager::BeginPlay()
{
	Super::BeginPlay();

	// TODO Comment This if you're moving to new project
	// Singleton Reference to the GameMode
	if(AOptimizedGridGameMode* ArenaGameMode = Cast<AOptimizedGridGameMode>(GetWorld()->GetAuthGameMode()); ArenaGameMode != nullptr)
	{
		if(ArenaGameMode->GetGridManager() == nullptr)
		{
			ArenaGameMode->SetGridManager(this);
		}
		else
		{
			Destroy();
		}
	}

	// Generate Tile Info Array if Tile Number Changes
	GenerateTileInfo();
	
	// Set Modifiers States to Tiles Info if not initialized or tile number changes
	InitializeStartingTilesModifiers();
}

/**
 * @brief Sets The Starting tiles info from the Modifiers arrays 
 */
void AGridManager::InitializeStartingTilesModifiers()
{
	if(bStartingModifiersInitialized) return;
	
	for (const auto TileMod : NoSpawningStartingTiles)
	{
		FTileInfo TileInfo;
		TileInfo.bCanSpawnOn = false;
		SetTileInfoAtPosition(TileMod.Row, TileMod.Column, TileInfo);
	}

	for (const auto TileMod: NoWalkingStartingTiles)
	{
		FTileInfo TileInfo;
		TileInfo.bCanWalkOn = false;
		SetTileInfoAtPosition(TileMod.Row, TileMod.Column, TileInfo);
	}

	bStartingModifiersInitialized = true;
}

/**
 * @brief Creates a Material Instance From Color and Opacity
 * @return Material Instance Dynamic
 */
TObjectPtr<UMaterialInstanceDynamic> AGridManager::CreateMaterialInstance(const FLinearColor Color, const float Opacity)
{
	const TObjectPtr<UMaterialInstanceDynamic> Material = UMaterialInstanceDynamic::Create(MaterialInterface, this);
	Material->SetVectorParameterValue(FName("Color"), Color);
	Material->SetScalarParameterValue(FName("Opacity"), Opacity);
	return Material;
}

/**
 * @brief Function To Overload Create Mesh Section parameters to only vertices and triangles. Target: procedural mesh component
 */
void AGridManager::CreateMeshSection(UProceduralMeshComponent* ProceduralMesh, const TArray<FVector>& Vertices, const TArray<int>& Triangles)
{
	const TArray<FVector> Normals;
	const TArray<FVector2D> UVs;
	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

/**
 * @brief Create a line mesh vertices and triangles
 * @param Start Start location of the line
 * @param End End location of the line
 * @param Thickness Line thickness
 * @param Vertices Array Reference to append to
 * @param Triangles Array Reference to append to
 */
void AGridManager::CreateLine(const FVector& Start, const FVector& End, const float Thickness, TArray<FVector>& Vertices, TArray<int>& Triangles)
{
	FVector Direction = End - Start;
	Direction.Normalize();
	const FVector ThicknessDirection = UKismetMathLibrary::Cross_VectorVector(Direction, FVector::UpVector);
	const float HalfThickness = Thickness / 2;

	const int VerticesCount = Vertices.Num();
	TArray<int> NewTriangles;
	NewTriangles.Add(VerticesCount+2);
	NewTriangles.Add(VerticesCount+1);
	NewTriangles.Add(VerticesCount+0);
	NewTriangles.Add(VerticesCount+2);
	NewTriangles.Add(VerticesCount+3);
	NewTriangles.Add(VerticesCount+1);
	Triangles.Append(NewTriangles);

	TArray<FVector> NewVertices;
	NewVertices.Add(Start + ThicknessDirection * HalfThickness);
	NewVertices.Add(End + ThicknessDirection * HalfThickness);
	NewVertices.Add(Start - ThicknessDirection * HalfThickness);
	NewVertices.Add(End -	 ThicknessDirection * HalfThickness);
	Vertices.Append(NewVertices);
}

bool AGridManager::GetTileInReferenceToTile(const int InRow, const int InColumn, const int RowOffset, const int ColOffset, FVector& OutLocation, FTileInfo& TileInfo, bool bConsiderRotation, FRotator Rotation)
{
	bool bValidTile;
	const FVector TileLocation = TileToGridLocation(InRow, InColumn, bValidTile);
	if(!bValidTile) return false;
	
	FVector Offset = FVector(TileSize * RowOffset, TileSize * ColOffset, 0.0f);
	if(bConsiderRotation) Offset = UKismetMathLibrary::Quat_RotateVector(Rotation.Quaternion(), Offset);

	int OutRow, OutColumn;
	LocationToTile(Offset+TileLocation, OutRow, OutColumn, bValidTile);
	if(!bValidTile) return false;

	TileInfo = GetTileInfoAtPositionWithLocation(OutRow, OutColumn, OutLocation, bValidTile);
	
	return bValidTile;
}

bool AGridManager::GetTileInReferenceToLocation(const FVector Location, const int RowOffset, const int ColOffset, FVector& OutLocation, FTileInfo& TileInfo, bool bConsiderRotation, FRotator Rotation)
{
	int Row, Column;
	bool bValid; 
	LocationToTile(Location, Row, Column, bValid);
	if(!bValid) return false;
	
	return GetTileInReferenceToTile(Row, Column, RowOffset, ColOffset, OutLocation, TileInfo, bConsiderRotation, Rotation);
}

/**
 * @brief Function for checking if Row and Column are in the Grid Range
 * @return Boolean
 */
bool AGridManager::IsValidTile(const int Row, const int Column) const
{
	return Row >= 0 && Row < NumRows && Column >= 0 && Column < NumColumns;
}

/**
 * @brief Function for checking if Row and Column are in the Grid Range and tile is walkable
 * @return Boolean
 */
bool AGridManager::IsValidWalkTile(const int Row, const int Column) const
{
	bool bValid;
	return IsValidTile(Row, Column) && GetTileInfoAtPositionCopy(Row, Column, bValid).bCanWalkOn;
}

/**
 * @brief Function for checking if Row and Column are in the Grid Range and tile is spawn free
 * @return Boolean
 */
bool AGridManager::IsValidSpawnTile(int Row, int Column) const
{
	bool bValid;
	return IsValidTile(Row, Column) && GetTileInfoAtPositionCopy(Row, Column, bValid).bCanSpawnOn;
}

/**
 * @brief Mapping from world location to tile row and column
 * @param Location World Location
 * @param RowOut Tile row Mapped
 * @param ColumnOut Tile column Mapped
 * @param bValid Location is in Grid Range
 */
void AGridManager::LocationToTile(const FVector Location, int& RowOut, int& ColumnOut, bool& bValid) const
{
	const FVector ActorLocation = GetActorLocation();
	RowOut = UKismetMathLibrary::FFloor((Location.X-ActorLocation.X)/GetGridWidth()*NumColumns);
	ColumnOut = UKismetMathLibrary::FFloor((Location.Y-ActorLocation.Y)/GetGridHeight()*NumRows);
	bValid = IsValidTile(RowOut, ColumnOut);
}

bool AGridManager::TakeTileSpace(AActor* Actor, bool bAffectWalkable)
{
	int Row, Column;
	bool bValid;
	LocationToTile(Actor->GetActorLocation(), Row, Column, bValid);

	if(!bValid)
	{
		return false;
	}

	FTileInfo TileInfo = GetTileInfoAtPositionCopy(Row, Column, bValid);

	if(TileInfo.Position.X < 0 || TileInfo.Position.Y < 0)
	{
		return false;
	}

	TileInfo.bCanSpawnOn = false;
	TileInfo.bCanWalkOn = bAffectWalkable? false : TileInfo.bCanWalkOn;
	TileInfo.ActorOnTile = Actor;
	return SetTileInfoAtPosition(Row, Column, TileInfo);
}

FVector AGridManager::LocationToGridLocation(FVector Location, bool& bValid)
{
	int Row, Column;
	LocationToTile(Location, Row, Column, bValid);
	return TileToGridLocation(Row, Column, bValid);
}

AActor* AGridManager::SpawnActorOnGrid(const TSubclassOf<AActor> ActorClass, const int Row, const int Column, bool& bSpawned, const FTransform SpawnTransform, const bool bCenter, const bool bAffectWalkable)
{
	if (!IsValid(ActorClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s() Invalid Actor Spawn Class"), *FString(__FUNCTION__))
		return nullptr;
	}
	
	if (const UWorld* Level = GetWorld(); IsValid(Level))
	{
		bool bTileValid;
		const FVector SpawnLocation = TileToSpawnGridLocation(Row, Column, bTileValid, bCenter, SpawnTransform.GetLocation());

		if(!bTileValid)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s() Tile (%d, %d) is Invalid"), *FString(__FUNCTION__), Row, Column);
			bSpawned = false;
			return nullptr;
		}

		FTransform ActorSpawnTransform;
		ActorSpawnTransform.SetLocation(SpawnLocation);
		ActorSpawnTransform.SetRotation(SpawnTransform.GetRotation());
		ActorSpawnTransform.SetScale3D(SpawnTransform.GetScale3D());

		AActor* SpawnedActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ActorClass, SpawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, nullptr, ESpawnActorScaleMethod::OverrideRootScale);
		if(SpawnedActor != nullptr)
		{
			SpawnedActor->FinishSpawning(ActorSpawnTransform);
			bSpawned = true;
			bool bValid;
			FTileInfo TileInfo = GetTileInfoAtPositionCopy(Row, Column, bValid);
			TileInfo.bCanSpawnOn = false;
			TileInfo.bCanWalkOn = bAffectWalkable? false : TileInfo.bCanWalkOn;
			TileInfo.ActorOnTile = SpawnedActor;
			SetTileInfoAtPosition(Row, Column, TileInfo);
		}
		return SpawnedActor;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s() No Level To Spawn"), *FString(__FUNCTION__));
	return nullptr;
}

/**
 * @brief Mapping from grid row and column to world location
 * @param Row Tile row position
 * @param Column Tile column position
 * @param bValid Row and column are in grid range
 * @param bCenter Get the center location of the tile or the bottom left corner if false 
 * @param Offset Offset vector to Add to the end location
 * @return Tile Location (center/corner) + offset
 */
FVector AGridManager::TileToGridLocation(const int Row, const int Column, bool& bValid, const bool bCenter, const FVector Offset) const
{
	const FVector ActorLocation = GetActorLocation();
	
	bValid = IsValidTile(Row, Column);
	FVector GridLocation;
	GridLocation.X = bCenter? Row * TileSize + ActorLocation.X + (TileSize / 2) : Row * TileSize + ActorLocation.X;
	GridLocation.Y = bCenter? Column * TileSize + ActorLocation.Y +(TileSize / 2) : Column * TileSize + ActorLocation.Y;
	GridLocation = GridLocation + Offset;
	return GridLocation;
}

/**
 * @brief Mapping from grid row and column to world location
 * @param Row Tile row position
 * @param Column Tile column position
 * @param bValid Row and column are in grid range and tile is walkable
 * @param bCenter Get the center location of the tile or the bottom left corner if false 
 * @param Offset Offset vector to Add to the end location
 * @return Tile Location (center/corner) + offset
 */
FVector AGridManager::TileToWalkGridLocation(const int Row, const int Column, bool& bValid, const bool bCenter, const FVector Offset) const
{
	const FVector ActorLocation = GetActorLocation();
	
	bValid = IsValidWalkTile(Row, Column);
	FVector GridLocation;
	GridLocation.X = bCenter? Row * TileSize + ActorLocation.X + (TileSize / 2) : Row * TileSize + ActorLocation.X;
	GridLocation.Y = bCenter? Column * TileSize + ActorLocation.Y +(TileSize / 2) : Column * TileSize + ActorLocation.Y;
	GridLocation = GridLocation + Offset;
	return GridLocation;
}

/**
 * @brief Mapping from grid row and column to world location
 * @param Row Tile row position
 * @param Column Tile column position
 * @param bValid Row and column are in grid range and tile is spawn free
 * @param bCenter Get the center location of the tile or the bottom left corner if false 
 * @param Offset Offset vector to Add to the end location
 * @return Tile Location (center/corner) + offset
 */
FVector AGridManager::TileToSpawnGridLocation(const int Row, const int Column, bool& bValid, const bool bCenter, const FVector Offset) const
{
	const FVector ActorLocation = GetActorLocation();
	
	bValid = IsValidSpawnTile(Row, Column);
	FVector GridLocation;
	GridLocation.X = bCenter? Row * TileSize + ActorLocation.X + (TileSize / 2) : Row * TileSize + ActorLocation.X;
	GridLocation.Y = bCenter? Column * TileSize + ActorLocation.Y +(TileSize / 2) : Column * TileSize + ActorLocation.Y;
	GridLocation = GridLocation + Offset;
	return GridLocation;
}

/**
 * @brief Set selected tile mesh to location of the tile
 * @param Row Selected tile row
 * @param Column Selected tile column
 */
void AGridManager::SetSelectedTile(const int Row, const int Column) const
{
	bool bValid;
	const FVector Location = TileToGridLocation(Row, Column, bValid, false);
	
	if(bValid)
	{
		SelectionMesh->SetWorldLocation(FVector(Location.X, Location.Y, SelectionMesh->GetComponentLocation().Z));
	}

	SelectionMesh->SetVisibility(bValid);
}

/**
 * @return true if tiles info array is equal to the number of tiles
 */
bool AGridManager::IsGridInfoInitialized() const
{
	return TilesInfo.Num() == NumColumns * NumRows;
}

/**
 * @brief Clear then Populate Tiles Info Array if grid info array is not the right size (already initialized)
 */
void AGridManager::GenerateTileInfo()
{
	if(IsGridInfoInitialized()) return;
	
	// Fill tiles info array
	bStartingModifiersInitialized = false;
	TilesInfo.Empty();
	for (int i = 0; i < NumRows; ++i)
	{
		for (int j = 0; j < NumColumns; ++j)
		{
			TilesInfo.Add(FTileInfo(i, j));
		}
	}
}

/**
 * @brief Get Tile info at Index
 * @param Index index in array
 * @return FTileInfo(-1, -1) if index out of range or array not initialized
 */
FTileInfo AGridManager::GetTileInfoAtIndexCopy(const int Index) const
{
	if(!IsGridInfoInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Tiles info array not initialized"), *FString(__FUNCTION__));
		return FTileInfo(-1, -1);
	}
	
	if(Index < 0 || Index > TilesInfo.Num()-1)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Index out of bound"), *FString(__FUNCTION__));
		return FTileInfo(-1, -1);
	}

	return TilesInfo[Index];
}

/**
 * @brief Set Tile Info at Index
 * @param Index index in array
 * @param TileInfoIn Tile Info to Set
 * @return false if index out of range or array not initialized
 */
bool AGridManager::SetTileInfoAtIndex(const int Index, const FTileInfo TileInfoIn)
{
	if(!IsGridInfoInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Tiles info array not initialized"), *FString(__FUNCTION__));
		return false;
	}
	
	if(Index < 0 || Index > TilesInfo.Num()-1)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Index out of bound"), *FString(__FUNCTION__));
		return false;
	}

	TilesInfo[Index].bCanWalkOn = TileInfoIn.bCanWalkOn;
	TilesInfo[Index].bCanSpawnOn = TileInfoIn.bCanSpawnOn;
	TilesInfo[Index].ActorOnTile = TileInfoIn.ActorOnTile;
	return true;
}

/**
 * @brief Get Tile Info of position Row, Column Complexity O(1)
 * @param Row starting 0
 * @param Column starting 0
 * @return FTileInfo(-1, -1) if position out of range or array not initialized
 */
FTileInfo AGridManager::GetTileInfoAtPositionCopy(const int Row, const int Column, bool& bValid) const
{
	if(!IsGridInfoInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Tiles info array not initialized in"), *FString(__FUNCTION__));
		bValid = false;
		return FTileInfo(-1, -1);
	}
	
	if(!IsValidTile(Row, Column))
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Position out of grid range"), *FString(__FUNCTION__));
		bValid = false;
		return FTileInfo(-1, -1);
	}
	
	const int Index = Row * NumColumns + Column;
	bValid = true;
	return TilesInfo[Index];
}

FTileInfo AGridManager::GetTileInfoAtPositionWithLocation(const int Row, const int Column, FVector& TileLocation,  bool& bValid) const
{
	TileLocation = TileToGridLocation(Row, Column, bValid);
	return GetTileInfoAtPositionCopy(Row, Column, bValid);
}

/**
 * @brief Set the Tile Info of position Row, Column Complexity O(1)
 * @param Row starting 0
 * @param Column starting 0
 * @param TileInfoIn Tile Info to Set 
 * @return false if position out of range or array not initialized
 */
bool AGridManager::SetTileInfoAtPosition(const int Row, const int Column, const FTileInfo TileInfoIn)
{
	if(!IsGridInfoInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Tiles info array not initialized"), *FString(__FUNCTION__));
		return false;
	}
	
	if(!IsValidTile(Row, Column))
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Position out of grid range"), *FString(__FUNCTION__));
		return false;
	}
	
	const int Index = Row * NumRows + Column;
	TilesInfo[Index].bCanWalkOn = TileInfoIn.bCanWalkOn;
	TilesInfo[Index].bCanSpawnOn = TileInfoIn.bCanSpawnOn;
	TilesInfo[Index].ActorOnTile = TileInfoIn.ActorOnTile;
	return true;
}

void AGridManager::GetGridRowsAndColumns(int& Row, int& Column) const
{
	Row = NumRows;
	Column = NumColumns;
}

TArray<FTileInfo> AGridManager::GetNeighboringTiles(const int Row, const int Column, const int NeighboringRows, const int NeighboringColumns)
{
	TArray<FTileInfo> Neighbors;
	for (int i = Row - NeighboringRows; i <= Row + NeighboringRows; i++)
	{
		for (int j = Column - NeighboringColumns; j <= Column + NeighboringColumns; j++)
		{
			bool bValid;
			const FTileInfo WalkableTile = GetTileInfoAtPositionCopy(i, j, bValid);

			if (WalkableTile.bCanWalkOn)
			{
				FTileInfo TileInfo = FTileInfo(i, j);
				Neighbors.Add(TileInfo);
			}
			
		}
	}
	return Neighbors;
}

void AGridManager::DisplayDebugInfoOnTile(const FVector& Location) const
{
	static const auto CVarGrid = IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugGrid"));

	if(CVarGrid->GetInt() != 0)
	{
		int Row, Column;
		bool bValid;
		LocationToTile(Location, Row, Column, bValid);
		if(bValid)
		{
			const FVector TileLocation = TileToGridLocation(Row, Column, bValid, true, FVector(0, 0, 10.0f));
			const FTileInfo TileInfo = GetTileInfoAtPositionCopy(Row, Column,bValid);
			DrawDebugString(GetWorld(), TileLocation, FString::Printf(TEXT("X: %d \nY: %d \nWalkable %s \nSpawnable %s \n%s"), TileInfo.Position.X, TileInfo.Position.Y, TileInfo.bCanWalkOn ? *FString("True") : *FString("False"), TileInfo.bCanSpawnOn ? *FString("True") : *FString("False"), *GetNameSafe(TileInfo.ActorOnTile)), nullptr, FColor::White, 4.f);
		}
	}
}
