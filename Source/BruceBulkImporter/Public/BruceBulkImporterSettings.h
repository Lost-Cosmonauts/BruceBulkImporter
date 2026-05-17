// Copyright Lost Cosmonauts

#pragma once

#include "Engine/DeveloperSettings.h"
#include "BruceBulkImporterSettings.generated.h"

UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Bruce Bulk Importer"))
class BRUCEBULKIMPORTER_API UBruceBulkImporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBruceBulkImporterSettings();

	/** 
	 * Format string for the destination folder. 
	 * {0} is SeqNum, {1} is ShotNum. 
	 * Example: /Game/Animation/Seq{0}/{1} 
	 */
	UPROPERTY(Config, EditAnywhere, Category="Import Settings")
	FString DestFolderFormatStr;

	/** Default import directory if the provided string is empty. */
	UPROPERTY(Config, EditAnywhere, Category="Import Settings")
	FString DefaultImportDir;

	/** Directories to search when attempting to automatically find a character Blueprint to spawn in a sequence. */
	UPROPERTY(Config, EditAnywhere, Category="Shot Automation")
	TArray<FString> CharacterSearchPaths = { TEXT("/Game/") };
};
