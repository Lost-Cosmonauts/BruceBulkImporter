// Copyright Lost Cosmonauts

#include "BruceEditorSubsystem.h"

#include "AbcImportSettings.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "GroomImportOptions.h"
#include "GroomCacheImportOptions.h"
#include "DNAAssetImportUI.h"
#include "OpenVDBImportOptions.h"
#include "SpeedTreeImportData.h"
#include "USDStageImportOptions.h"
#include "Factories/FbxImportUI.h"

#include "BruceImportData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BruceEditorSubsystem)


UBruceEditorSubsystem::UBruceEditorSubsystem()
	: ImportToOptionsClass{
		{EImportType::Alembic, UAbcImportSettings::StaticClass()},
		{EImportType::Groom, UGroomImportOptions::StaticClass()},
		{EImportType::GroomCache, UGroomCacheImportOptions::StaticClass()},
		{EImportType::VDB, UOpenVDBImportOptionsObject::StaticClass()},
		{EImportType::FBX, UFbxImportUI::StaticClass()},
		{EImportType::RigLogic, UDNAAssetImportUI::StaticClass()},
		{EImportType::SpeedTree, USpeedTreeImportData::StaticClass()},
		{EImportType::USD, UUsdStageImportOptions::StaticClass()}
	}
{
}

void UBruceEditorSubsystem::ImportAssets(const TArray<FBruceImportData>& ImportList)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<UAssetImportTask*> ImportTasks;
	ImportTasks.Reserve(ImportList.Num());
	for (const auto& ImportData : ImportList)
	{
		const FFilePath& File = ImportData.GetFileName();
		FString FileName = File.FilePath;
		if (FileName.IsEmpty())
		{
			continue;
		}
		FileName = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FileName);

		TStrongObjectPtr Task{ NewObject<UAssetImportTask>() };
		Task->AddToRoot();

		Task->Filename = FileName;
		Task->bAutomated = true;
		Task->bReplaceExisting = true;
		Task->bSave = true;
		Task->bAsync = true;

		if (ImportToOptionsClass.Find(ImportData.AssetImportType))
		{
			const UClass* Class = ImportToOptionsClass[ImportData.AssetImportType];
			if (ImportData.ImportOptions && ImportData.ImportOptions->IsChildOf(Class))
			{
				Task->Options = ImportData.ImportOptions->GetDefaultObject();
			}
			else
			{
				Task->RemoveFromRoot();
				continue;
			}
		}
		else
		{
			Task->RemoveFromRoot();
			continue;
		}

		FString DestinationPath = ImportData.DestinationFolder.Path;
		if (!DestinationPath.IsEmpty() && FPackageName::GetPackageMountPoint(DestinationPath) == NAME_None)
		{
			// Path doesn't have a valid mount point.  assume it is in /Game
			DestinationPath = TEXT("/Game") / DestinationPath;
		}
		FString PackagePath;
		FString FailureReason;
		if (!FPackageName::TryConvertFilenameToLongPackageName(DestinationPath, PackagePath, &FailureReason))
		{
			DestinationPath = TEXT("/Game");
		}
		else
		{
			// Package path is what we will use for importing.  So use that as the dest path now that it has been created
			DestinationPath = PackagePath;
		}
		Task->DestinationPath = DestinationPath;

		ImportTasks.Add(Task.Get());
	}

	AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
}

void UBruceEditorSubsystem::ImportDefaultDataTable()
{
	if (!IsValid(ImportDataTable))
	{
		return;
	}
	static const FString ContextString{ TEXT("UBruceEditorSubsystem::ImportDefaultDataTable") };
	TArray<FBruceImportData*> ImportTableRowPtrs;
	ImportDataTable->GetAllRows<FBruceImportData>(ContextString, ImportTableRowPtrs);
	TArray<FBruceImportData> ImportTableRows;
	ImportTableRows.Reserve(ImportTableRowPtrs.Num());
	for (const auto ImportDataPtr : ImportTableRowPtrs)
	{
		ImportTableRows.Emplace(*ImportDataPtr);
	}
	ImportAssets(ImportTableRows);
}

void UBruceEditorSubsystem::EnableFbxContentType(UFbxImportUI* FbxImportUI, bool bEnable)
{
	if (!FbxImportUI)
	{
		return;
	}

	FbxImportUI->bAllowContentTypeImport = bEnable;
}
