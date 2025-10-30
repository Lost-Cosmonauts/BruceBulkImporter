// Copyright Lost Cosmonauts

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "BruceBulkImporterStyle.h"

class FBruceBulkImporterCommands : public TCommands<FBruceBulkImporterCommands>
{
public:

	FBruceBulkImporterCommands()
		: TCommands<FBruceBulkImporterCommands>(TEXT("BruceBulkImporter"), NSLOCTEXT("Contexts", "BruceBulkImporter", "BruceBulkImporter Plugin"), NAME_None, FBruceBulkImporterStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};