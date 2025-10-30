// Copyright Lost Cosmonauts

#include "BruceBulkImporter.h"
#include "BruceBulkImporterStyle.h"
#include "BruceBulkImporterCommands.h"
#include "BruceEditorSubsystem.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName BruceBulkImporterTabName("BruceBulkImporter");

#define LOCTEXT_NAMESPACE "FBruceBulkImporterModule"

void FBruceBulkImporterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FBruceBulkImporterStyle::Initialize();
	FBruceBulkImporterStyle::ReloadTextures();

	FBruceBulkImporterCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FBruceBulkImporterCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FBruceBulkImporterModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBruceBulkImporterModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(BruceBulkImporterTabName, FOnSpawnTab::CreateRaw(this, &FBruceBulkImporterModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FBruceBulkImporterTabTitle", "Bruce Bulk Importer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FBruceBulkImporterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FBruceBulkImporterStyle::Shutdown();

	FBruceBulkImporterCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BruceBulkImporterTabName);
}

TSharedRef<SDockTab> FBruceBulkImporterModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const UBruceEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UBruceEditorSubsystem>();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
		];
}

void FBruceBulkImporterModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(BruceBulkImporterTabName);
}

void FBruceBulkImporterModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FBruceBulkImporterCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FBruceBulkImporterCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBruceBulkImporterModule, BruceBulkImporter)