// Copyright Lost Cosmonauts

#include "BruceEditorSubsystem.h"

#include "AbcImportSettings.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Animation/AnimSequence.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "MovieSceneTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Sections/MovieSceneSkeletalAnimationSection.h"
#include "Tracks/MovieSceneSubTrack.h"
#include "Sections/MovieSceneSubSection.h"
#include "Engine/Blueprint.h"
#include "BruceBulkImporterSettings.h"
#include "GroomImportOptions.h"
#include "GroomCacheImportOptions.h"
#if UE_VERSION_OLDER_THAN(5,8,0)
#include "DNAAssetImportUI.h"
#endif
#include "OpenVDBImportOptions.h"
#include "SpeedTreeImportData.h"
#include "USDStageImportOptions.h"
#include "Factories/FbxImportUI.h"
#include "Factories/DataTableFactory.h"
#include "EditorUtilityLibrary.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "BruceImportData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BruceEditorSubsystem)


UBruceEditorSubsystem::UBruceEditorSubsystem()
	: ImportToOptionsClass{
		{EImportType::Alembic, UAbcImportSettings::StaticClass()},
		{EImportType::Groom, UGroomImportOptions::StaticClass()},
		{EImportType::GroomCache, UGroomCacheImportOptions::StaticClass()},
		{EImportType::VDB, UOpenVDBImportOptionsObject::StaticClass()},
		{EImportType::FBX, UFbxImportUI::StaticClass()},
#if UE_VERSION_OLDER_THAN(5,8,0)
		{EImportType::RigLogic, UDNAAssetImportUI::StaticClass()},
#endif
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

void UBruceEditorSubsystem::BuildImportListFromSelection(const FString& ImportDir, TSubclassOf<UObject> DefaultImportOptions, const TMap<FString, TSubclassOf<UObject>>& ImportOptionMap, bool bClearList)
{
	const UBruceBulkImporterSettings* Settings = GetDefault<UBruceBulkImporterSettings>();
	FString SanitizedImportDir = ImportDir;

	if (SanitizedImportDir.IsEmpty())
	{
		SanitizedImportDir = Settings->DefaultImportDir;
	}

	if (!SanitizedImportDir.EndsWith(TEXT("/")))
	{
		SanitizedImportDir += TEXT("/");
	}

	TArray<UObject*> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
	for (UObject* Asset : SelectedAssets)
	{
		UDataTable* DataTable = Cast<UDataTable>(Asset);
		if (!DataTable)
		{
			continue;
		}

		if (DataTable->GetRowStruct() != FBruceImportData::StaticStruct())
		{
			UE_LOG(LogTemp, Warning, TEXT("Selected DataTable %s does not use FBruceImportData struct, skipping."), *DataTable->GetName());
			continue;
		}

		FString CurrentShot = DataTable->GetName();
		if (!CurrentShot.StartsWith(TEXT("DT_Bruce_")))
		{
			UE_LOG(LogTemp, Error, TEXT("DataTable %s name does not start with DT_Bruce_, skipping."), *CurrentShot);
			continue;
		}

		CurrentShot.ReplaceInline(TEXT("DT_Bruce_"), TEXT(""));

		FString SeqNum;
		FString ShotNum;
		if (!CurrentShot.Split(TEXT("_"), &SeqNum, &ShotNum))
		{
			UE_LOG(LogTemp, Error, TEXT("DataTable %s suffix '%s' does not match the SeqNum_ShotNum format, skipping."), *DataTable->GetName(), *CurrentShot);
			continue;
		}

		TSet<FName> ExistingItems;
		if (bClearList)
		{
			DataTable->EmptyTable();
		}
		else
		{
			for (auto It : DataTable->GetRowMap())
			{
				ExistingItems.Add(It.Key);
			}
		}

		FString ImportDirFull = FPaths::Combine(FPaths::ProjectDir(), SanitizedImportDir, CurrentShot);

		TArray<FString> FoundFiles;
		IFileManager::Get().FindFiles(FoundFiles, *(ImportDirFull / TEXT("*.fbx")), true, false);

		bool bAddedAny = false;
		for (const FString& FoundFile : FoundFiles)
		{
			FString CurrentShotPrefix = CurrentShot + TEXT("_");
			FString CurrentShotDotPrefix = CurrentShot + TEXT(".");
			if (!FoundFile.StartsWith(CurrentShotPrefix, ESearchCase::IgnoreCase) && !FoundFile.StartsWith(CurrentShotDotPrefix, ESearchCase::IgnoreCase))
			{
				continue;
			}

			FString BaseFilename = FPaths::GetBaseFilename(FoundFile);
			FName RowName = FName(*BaseFilename);

			if (!bClearList && ExistingItems.Contains(RowName))
			{
				continue;
			}

			TSubclassOf<UObject> CurrentImportOptions = DefaultImportOptions;
			for (const auto& KVP : ImportOptionMap)
			{
				if (BaseFilename.Contains(KVP.Key, ESearchCase::IgnoreCase))
				{
					CurrentImportOptions = KVP.Value;
					break;
				}
			}

			FString DestFolderStr = FString::Format(*Settings->DestFolderFormatStr, { SeqNum, ShotNum });
			FDirectoryPath DestFolder;
			DestFolder.Path = DestFolderStr;

			FFilePath FBXFilePath;
			FBXFilePath.FilePath = FPaths::Combine(ImportDirFull, FoundFile);

			FBruceImportData NewRow;
			NewRow.AssetImportType = EImportType::FBX;
			NewRow.ImportOptions = CurrentImportOptions;
			NewRow.FBXFilename = FBXFilePath;
			NewRow.DestinationFolder = DestFolder;

			DataTable->AddRow(RowName, NewRow);
			bAddedAny = true;
		}

		if (bAddedAny || bClearList)
		{
			DataTable->MarkPackageDirty();
		}
	}
}

void UBruceEditorSubsystem::CreateImportTables(const FString& ImportDir, const FString& PackagePath)
{
	const UBruceBulkImporterSettings* Settings = GetDefault<UBruceBulkImporterSettings>();
	FString SanitizedImportDir = ImportDir;

	if (SanitizedImportDir.IsEmpty())
	{
		SanitizedImportDir = Settings->DefaultImportDir;
	}

	FString ImportDirFull = FPaths::Combine(FPaths::ProjectDir(), SanitizedImportDir);

	TArray<FString> FoundDirs;
	IFileManager::Get().FindFiles(FoundDirs, *(ImportDirFull / TEXT("*")), false, true);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UDataTableFactory* Factory = NewObject<UDataTableFactory>();
	Factory->Struct = FBruceImportData::StaticStruct();

	for (const FString& DirName : FoundDirs)
	{
		FString SeqNum, ShotNum;
		if (DirName.Split(TEXT("_"), &SeqNum, &ShotNum))
		{
			FString AssetName = FString::Printf(TEXT("DT_Bruce_%s"), *DirName);

			FString FullPackagePath = PackagePath / AssetName;
			if (FindObject<UDataTable>(nullptr, *FullPackagePath) || FPackageName::DoesPackageExist(FullPackagePath))
			{
				UE_LOG(LogTemp, Log, TEXT("CreateImportTables: Asset %s already exists, skipping."), *FullPackagePath);
				continue;
			}

			UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), Factory);
			if (NewAsset)
			{
				UE_LOG(LogTemp, Log, TEXT("CreateImportTables: Created %s"), *FullPackagePath);
			}
		}
	}
}

void UBruceEditorSubsystem::UpdateShotSequence(ULevelSequence* Sequence, TArray<UAnimSequence*> Animations)
{
	if (!Sequence)
	{
		return;
	}

	UMovieScene* ParentMovieScene = Sequence->GetMovieScene();
	if (!ParentMovieScene)
	{
		return;
	}

	ULevelSequence* TargetSequence = Sequence;
	UMovieSceneSubSection* TargetSubSection = nullptr;

	FString ExpectedSubSeqName = Sequence->GetName() + TEXT("_Anim");

	
	for (const TObjectPtr<UMovieSceneTrack>& Track : ParentMovieScene->GetTracks())
	{
		UMovieSceneSubTrack* SubTrack = Cast<UMovieSceneSubTrack>(Track);
		if (SubTrack)
		{
			for (UMovieSceneSection* Section : SubTrack->GetAllSections())
			{
				UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
				if (SubSection && SubSection->GetSequence())
				{
					if (SubSection->GetSequence()->GetName() == ExpectedSubSeqName)
					{
						TargetSequence = Cast<ULevelSequence>(SubSection->GetSequence());
						TargetSubSection = SubSection;
						break;
					}
				}
			}
		}
	}

	if (Animations.Num() == 0)
	{
		// Try to find animations using DestFolderFormatStr
		const UBruceBulkImporterSettings* Settings = GetDefault<UBruceBulkImporterSettings>();
		if (!Settings->DestFolderFormatStr.IsEmpty())
		{
			FString SequenceName = Sequence->GetName();
			FString SeqNum, ShotNum;
			if (SequenceName.Split(TEXT("_"), &SeqNum, &ShotNum))
			{
				if (SeqNum.StartsWith(TEXT("Seq"), ESearchCase::IgnoreCase))
				{
					SeqNum.RightChopInline(3);
				}

				int32 SeqInt = FCString::Atoi(*SeqNum);
				SeqNum = FString::Printf(TEXT("%02d"), SeqInt);

				FStringFormatOrderedArguments Args;
				Args.Add(SeqNum);
				Args.Add(ShotNum);
				FString DestFolder = FString::Format(*Settings->DestFolderFormatStr, Args);

				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

				FARFilter Filter;
				Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
				Filter.PackagePaths.Add(*DestFolder);
				Filter.bRecursivePaths = true;

				TArray<FAssetData> FoundAssets;
				AssetRegistry.GetAssets(Filter, FoundAssets);

				for (const FAssetData& AssetData : FoundAssets)
				{
					if (UAnimSequence* Anim = Cast<UAnimSequence>(AssetData.GetAsset()))
					{
						Animations.Add(Anim);
					}
				}
			}
		}
	}

	if (Animations.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateShotSequence: No animations provided or found for sequence %s"), *Sequence->GetName());
		return;
	}

	UMovieScene* MovieScene = TargetSequence->GetMovieScene();
	if (!MovieScene)
	{
		return;
	}

	float MaxLength = 0.0f;

	FFrameNumber StartFrameTick = MovieScene->GetPlaybackRange().GetLowerBoundValue();

	for (UAnimSequence* AnimSequence : Animations)
	{
		if (!AnimSequence) continue;

		float AnimLength = AnimSequence->GetPlayLength();
		MaxLength = FMath::Max(MaxLength, AnimLength);

		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FFrameNumber EndFrame = TickResolution.AsFrameTime(AnimLength).RoundToFrame();

		// 1. Parse Character Name from AnimName (e.g., 02_110_Palmi -> Palmi)
		FString AnimName = AnimSequence->GetName();
		FString CharacterBaseName = AnimName;
		int32 LastUnderscoreIdx;
		if (AnimName.FindLastChar('_', LastUnderscoreIdx))
		{
			CharacterBaseName = AnimName.RightChop(LastUnderscoreIdx + 1);
		}

		// 2. Try to find a binding matching this name
		FGuid CharacterBindingGuid;
		bool bFoundBinding = false;
		int32 BestScore = -1;

		auto GetMatchScore = [](const FString& InName, const FString& SearchTerm) -> int32
		{
			if (!InName.Contains(SearchTerm))
			{
				return -1;
			}
			return 1000 - FMath::Abs(InName.Len() - SearchTerm.Len());
		};

		for (int32 i = 0; i < MovieScene->GetSpawnableCount(); ++i)
		{
			FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(i);
			int32 Score = GetMatchScore(Spawnable.GetName(), CharacterBaseName);
			if (Score > BestScore)
			{
				BestScore = Score;
				CharacterBindingGuid = Spawnable.GetGuid();
				bFoundBinding = true;
			}
		}

		for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
		{
			FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
			int32 Score = GetMatchScore(Possessable.GetName(), CharacterBaseName);
			if (Score > BestScore)
			{
				BestScore = Score;
				CharacterBindingGuid = Possessable.GetGuid();
				bFoundBinding = true;
			}
		}

		// 3. If no binding exists, search for the BP and add a spawnable
		if (!bFoundBinding)
		{
			const UBruceBulkImporterSettings* Settings = GetDefault<UBruceBulkImporterSettings>();
			FString BPNameToFind = FString::Printf(TEXT("BP_%s"), *CharacterBaseName);

			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			FARFilter Filter;
			Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
			for (const FString& Path : Settings->CharacterSearchPaths)
			{
				Filter.PackagePaths.Add(*Path);
			}
			Filter.bRecursivePaths = true;

			TArray<FAssetData> FoundAssets;
			AssetRegistry.GetAssets(Filter, FoundAssets);

			UObject* TemplateObject = nullptr;
			for (const FAssetData& AssetData : FoundAssets)
			{
				if (AssetData.AssetName.ToString() == BPNameToFind)
				{
					if (UBlueprint* FoundBP = Cast<UBlueprint>(AssetData.GetAsset()))
					{
						if (FoundBP->GeneratedClass)
						{
							TemplateObject = FoundBP->GeneratedClass->GetDefaultObject();
							break;
						}
					}
				}
			}

			if (TemplateObject)
			{
				CharacterBindingGuid = MovieScene->AddSpawnable(BPNameToFind, *TemplateObject);
				bFoundBinding = CharacterBindingGuid.IsValid();
				UE_LOG(LogTemp, Log, TEXT("Created spawnable for %s in %s"), *BPNameToFind, *TargetSequence->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Could not find Blueprint %s to create spawnable in %s"), *BPNameToFind, *TargetSequence->GetName());
			}
		}

		// 4. Find or Create Animation Track and assign animation
		if (bFoundBinding)
		{
			UMovieSceneSkeletalAnimationTrack* AnimTrack = Cast<UMovieSceneSkeletalAnimationTrack>(MovieScene->FindTrack(UMovieSceneSkeletalAnimationTrack::StaticClass(), CharacterBindingGuid, NAME_None));
			if (!AnimTrack)
			{
				AnimTrack = MovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(CharacterBindingGuid);
			}

			if (AnimTrack)
			{
				UMovieSceneSkeletalAnimationSection* AnimSection = nullptr;
				for (UMovieSceneSection* Section : AnimTrack->GetAllSections())
				{
					AnimSection = Cast<UMovieSceneSkeletalAnimationSection>(Section);
					if (AnimSection) break;
				}

				if (!AnimSection)
				{
					AnimSection = Cast<UMovieSceneSkeletalAnimationSection>(AnimTrack->CreateNewSection());
					AnimTrack->AddSection(*AnimSection);
				}

				if (AnimSection)
				{
					AnimSection->Params.Animation = AnimSequence;
					AnimSection->SetRange(TRange<FFrameNumber>(StartFrameTick, StartFrameTick + EndFrame));
				}
			}
		}
	}

	// 5. Update Sequence Length
	if (MaxLength > 0.0f)
	{
		FFrameRate TargetTickResolution = MovieScene->GetTickResolution();
		FFrameNumber MaxEndFrame = TargetTickResolution.AsFrameTime(MaxLength).RoundToFrame();
		
		MovieScene->SetPlaybackRange(StartFrameTick.Value, (StartFrameTick + MaxEndFrame).Value);
		TargetSequence->MarkPackageDirty();

		if (ParentMovieScene)
		{
			FFrameRate ParentTickResolution = ParentMovieScene->GetTickResolution();
			FFrameNumber ParentStartFrameTick = ParentMovieScene->GetPlaybackRange().GetLowerBoundValue();
			FFrameNumber ParentMaxEndFrame = ParentTickResolution.AsFrameTime(MaxLength).RoundToFrame();

			ParentMovieScene->SetPlaybackRange(ParentStartFrameTick.Value, (ParentStartFrameTick + ParentMaxEndFrame).Value);
			Sequence->MarkPackageDirty();

			for (const TObjectPtr<UMovieSceneTrack>& Track : ParentMovieScene->GetTracks())
			{
				UMovieSceneSubTrack* SubTrack = Cast<UMovieSceneSubTrack>(Track);
				if (SubTrack)
				{
					for (UMovieSceneSection* Section : SubTrack->GetAllSections())
					{
						UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
						if (SubSection)
						{
							SubSection->SetRange(TRange<FFrameNumber>(SubSection->GetRange().GetLowerBoundValue(), SubSection->GetRange().GetLowerBoundValue() + ParentMaxEndFrame.Value));
							
							if (ULevelSequence* SubSequenceObj = Cast<ULevelSequence>(SubSection->GetSequence()))
							{
								if (UMovieScene* SubMovieScene = SubSequenceObj->GetMovieScene())
								{
									FFrameNumber SubStart = SubMovieScene->GetPlaybackRange().GetLowerBoundValue();
									FFrameRate SubTickRes = SubMovieScene->GetTickResolution();
									FFrameNumber SubMaxEnd = SubTickRes.AsFrameTime(MaxLength).RoundToFrame();
									
									SubMovieScene->SetPlaybackRange(SubStart.Value, (SubStart + SubMaxEnd).Value);
									SubSequenceObj->MarkPackageDirty();
								}
							}
						}
					}
				}
			}
		}
	}
}
