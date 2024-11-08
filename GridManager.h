// Copyright Rekt Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GridManager.generated.h"

class UProceduralMeshComponent;

USTRUCT(BlueprintType)
struct FTileMod
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0))
	int Row;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0))
	int Column;

	FTileMod(): Row(-1), Column(-1)
	{
	}

	explicit FTileMod(int const NewRow, int const NewColumn): Row(NewRow), Column(NewColumn)
	{
	}
};

USTRUCT(BlueprintType)
struct FTileInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FIntPoint Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanSpawnOn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanWalkOn;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> ActorOnTile;

	FTileInfo(): Position(FIntPoint(-1,-1)), bCanSpawnOn(true), bCanWalkOn(true)
	{
	}

	FTileInfo(int const Row, int const Column): bCanSpawnOn(true), bCanWalkOn(true)
	{
		Position.X = Row;
		Position.Y = Column;
	}

	FTileInfo(const bool bNewCanSpawn, const bool bNewCanWalk): Position(FIntPoint(-1,-1))
	{
		bCanWalkOn = bNewCanWalk;
		bCanSpawnOn = bNewCanSpawn;
	}

	FTileInfo(const bool bNewCanSpawn, const bool bNewCanWalk, AActor* NewActorOnTile)
	{
		bCanWalkOn = bNewCanWalk;
		bCanSpawnOn = bNewCanSpawn;
		ActorOnTile = NewActorOnTile;
	}
};

UCLASS(meta=(PrioritizeCategories = "GridManager"))
class AGridManager : public AActor
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess))
	TObjectPtr<UProceduralMeshComponent> LineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess))
	TObjectPtr<UProceduralMeshComponent> SelectionMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess))
	TObjectPtr<UProceduralMeshComponent> NoWalkMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess))
	TObjectPtr<UProceduralMeshComponent> NoSpawnMesh;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	int NumRows;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	int NumColumns;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	float TileSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	float LineThickness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	float LineOpacity;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	float SelectionOpacity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	FLinearColor LineColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	FLinearColor SelectionColor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GridManager|Generation", meta=(AllowPrivateAccess))
	TObjectPtr<UMaterialInterface> MaterialInterface;
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Modifiers", meta=(AllowPrivateAccess))
	float NoWalkOpacity;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Modifiers", meta=(AllowPrivateAccess))
	float NoSpawnOpacity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Modifiers", meta=(AllowPrivateAccess))
	FLinearColor NoWalkColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GridManager|Modifiers", meta=(AllowPrivateAccess))
	FLinearColor NoSpawnColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GridManager|Modifiers", meta=(AllowPrivateAccess))
	TArray<FTileMod> NoSpawningStartingTiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GridManager|Modifiers", meta=(AllowPrivateAccess))
	TArray<FTileMod> NoWalkingStartingTiles;

	TArray<FTileInfo> TilesInfo;

	bool bStartingModifiersInitialized;
	
public:	
	AGridManager();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void GenerateTileInfo();
	void InitializeStartingTilesModifiers();
	TObjectPtr<UMaterialInstanceDynamic> CreateMaterialInstance(FLinearColor Color, float Opacity);

	static void CreateMeshSection(UProceduralMeshComponent* ProceduralMesh, const TArray<FVector>& Vertices, const TArray<int>& Triangles);
	static void CreateLine(const FVector& Start, const FVector& End, float Thickness, TArray<FVector>& Vertices, TArray<int>& Triangles);
	
public:
	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool GetTileInReferenceToTile(int InRow, int InColumn, int RowOffset, int ColOffset, FVector& OutLocation, FTileInfo& TileInfo, bool bConsiderRotation = false, FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool GetTileInReferenceToLocation(FVector Location, int RowOffset, int ColOffset, FVector& OutLocation, FTileInfo& TileInfo, bool bConsiderRotation = false, FRotator Rotation = FRotator::ZeroRotator);
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool IsValidTile(int Row, int Column) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool IsValidWalkTile(int Row, int Column) const;
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool IsValidSpawnTile(int Row, int Column) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	void LocationToTile(FVector Location, int& RowOut, int& ColumnOut, bool& bValid) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool TakeTileSpace(AActor* Actor, bool bAffectWalkable = true);

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FVector LocationToGridLocation(FVector Location, bool& bValid);
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	AActor* SpawnActorOnGrid(TSubclassOf<AActor> ActorClass, int Row, int Column, bool& bSpawned, FTransform SpawnTransform, bool bCenter = true, bool bAffectWalkable = false);

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FVector TileToGridLocation(int Row, int Column, bool& bValid, bool bCenter = true, FVector Offset = FVector(0.0f, 0.0f, 0.0f)) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FVector TileToWalkGridLocation(int Row, int Column, bool& bValid, bool bCenter = true, FVector Offset = FVector(0.0f, 0.0f, 0.0f)) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FVector TileToSpawnGridLocation(int Row, int Column, bool& bValid, bool bCenter = true, FVector Offset = FVector(0.0f, 0.0f, 0.0f)) const;
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	void SetSelectedTile(int Row, int Column) const;
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool IsGridInfoInitialized() const;
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	FTileInfo GetTileInfoAtIndexCopy(const int Index) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool SetTileInfoAtIndex(const int Index, const FTileInfo TileInfoIn);

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FTileInfo GetTileInfoAtPositionCopy(const int Row, const int Column, bool& bValid) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FTileInfo GetTileInfoAtPositionWithLocation(const int Row, const int Column, FVector& TileLocation, bool& bValid) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	bool SetTileInfoAtPosition(const int Row, const int Column, const FTileInfo TileInfoIn);

	FORCEINLINE void GetGridRowsAndColumns(int& Row, int& Column) const;

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FORCEINLINE float GetTileSize() const { return TileSize; } 

	UFUNCTION(BlueprintCallable, Category="GridManager")
	FORCEINLINE float GetGridWidth() const { return NumColumns * TileSize; }
	
	UFUNCTION(BlueprintCallable, Category="GridManager")
	FORCEINLINE float GetGridHeight() const { return NumRows * TileSize; }

	UFUNCTION(BlueprintCallable, Category = "GridManager")
	FORCEINLINE UProceduralMeshComponent* GetLineMeshComponent() const { return LineMesh; }

	UFUNCTION(BlueprintCallable, Category = "GridManager")
	TArray<FTileInfo> GetNeighboringTiles(const int Row, const int Column, const int NeighboringRows, const int NeighboringColumns);

	void DisplayDebugInfoOnTile(const FVector& Location) const;
};