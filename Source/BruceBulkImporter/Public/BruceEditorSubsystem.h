// Copyright Lost Cosmonauts

#pragma once

#include "BruceEditorSubsystem.generated.h"

struct FBruceImportData;
enum class EImportType : uint8;

UCLASS(BlueprintType)
class UBruceEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UBruceEditorSubsystem();

	UFUNCTION(BlueprintCallable, Category = Bruce)
	void ImportAssets(const TArray<FBruceImportData>& ImportList);

	UFUNCTION(BlueprintCallable, Category = Bruce)
	void ImportDefaultDataTable();

	UFUNCTION(BlueprintCallable, Category = Bruce)
	void EnableFbxContentType(UFbxImportUI* FbxImportUI, bool bEnable = true);

	TMap<EImportType, TSubclassOf<UObject>> ImportToOptionsClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bruce, meta = (RowType = "BruceImportData"))
	TObjectPtr<UDataTable> ImportDataTable;
};