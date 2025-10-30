// Copyright Lost Cosmonauts

#include "BruceBulkImporterCommands.h"

#define LOCTEXT_NAMESPACE "FBruceBulkImporterModule"

void FBruceBulkImporterCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "BruceBulkImporter", "Bring up BruceBulkImporter window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
