// Copyright Lost Cosmonauts

#pragma once

#include "Engine/DataTable.h"

#include "BruceImportData.generated.h"

/**
 * Variable type for extended values.
 */
UENUM(BlueprintType)
enum class EImportType : uint8
{
	Unselected UMETA(Hidden),
	Alembic,
	Groom,
	GroomCache,
	VDB,
	FBX,
	RigLogic,
	SpeedTree,
	USD,
};

/** Structure that defines an import table entry */
USTRUCT(BlueprintType)
struct FBruceImportData : public FTableRowBase
{
	GENERATED_BODY()

	/** The 'Name' column is the same as the display name of the import entry. This is for logging purposes only. */

	/** What type of asset are we importing? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce)
	EImportType AssetImportType = EImportType::Unselected;

	/** The asset import type specific options */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (AllowedClasses = "AbcImportSettings,GroomImportOptions,GroomCacheImportOptions,FbxImportUI,DNAAssetImportUI,SpeedTreeImportData,UsdStageImportOptions"))
	TSubclassOf<UObject> ImportOptions = nullptr;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::Alembic", EditConditionHides, RelativeToGameDir, FilePathFilter = "Alembic Cache files (*.abc)|*.abc"))
	FFilePath AlembicFilename;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::Groom||AssetImportType==EImportType::GroomCache", EditConditionHides, RelativeToGameDir, FilePathFilter = "Alembic Groom files (*.abc)|*.abc"))
	FFilePath GroomFilename;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::VDB", EditConditionHides, RelativeToGameDir, FilePathFilter = "OpenVDB files (*.vdb)|*.vdb"))
	FFilePath VDBFilename;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::FBX", EditConditionHides, RelativeToGameDir, FilePathFilter = "FBX/OBJ files (*.fbx,*.obj)|*.fbx;*.obj"))
	FFilePath FBXFilename;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::RigLogic", EditConditionHides, RelativeToGameDir, FilePathFilter = "MetaHuman Rig Logic DNA files (*.dna)|*.dna"))
	FFilePath RigLogicFilename;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::SpeedTree", EditConditionHides, RelativeToGameDir, FilePathFilter = "SpeedTree files (*.st9,*.st,*.srt)|*.st9;*.st;*.srt"))
	FFilePath SpeedTreeFilename;

	/** File name to import */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (EditCondition = "AssetImportType==EImportType::USD", EditConditionHides, RelativeToGameDir, FilePathFilter = "USD files (*.usd,*.usda,*.usdc)|*.usd;*.usda;*.usdc"))
	FFilePath USDFilename;

	/** Content path in the project's content directory where assets will be imported */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bruce, meta = (RelativeToContentDir, LongPackageName))
	FDirectoryPath DestinationFolder{TEXT("/Game/")};

	FFilePath GetFileName() const
	{
		switch (AssetImportType)
		{
		case EImportType::Alembic:
			return AlembicFilename;
		case EImportType::Groom:
		case EImportType::GroomCache:
			return GroomFilename;
		case EImportType::VDB:
			return VDBFilename;
		case EImportType::FBX:
			return FBXFilename;
		case EImportType::RigLogic:
			return FBXFilename;
		case EImportType::SpeedTree:
			return SpeedTreeFilename;
		case EImportType::USD:
			return USDFilename;
		default:
			ensureMsgf(false, TEXT("Unknown/unselected asset import type!"));
			return FFilePath{};
		}
	}
};
