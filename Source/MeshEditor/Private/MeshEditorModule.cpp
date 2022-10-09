// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshEditorModule.h"
#include "MeshEditorEditorMode.h"
#include "MeshEditorEditorModeCommands.h"
#include "Widgets/SMeshEditorComboButton.h"

#include "LevelEditor.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "MeshEditorModule"

void FMeshEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FMeshEditorStyle::Initialize();
	FMeshEditorStyle::ReloadTextures();

	BindCommands();

	FEditorModeRegistry::Get().RegisterMode<FMeshEditorEditorMode>(FMeshEditorEditorMode::EM_MeshEditorEditorModeId, LOCTEXT("FMeshEditorEditorModeName", "MeshEditor"), FSlateIcon(), false);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetGlobalLevelEditorActions()->Append(MeshEditorActions.ToSharedRef());

	const TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

#if ENGINE_MAJOR_VERSION == 5
	ToolbarExtender->AddToolBarExtension(
		"LocationGridSnap",
		EExtensionHook::Before,
		MeshEditorActions,
		FToolBarExtensionDelegate::CreateRaw(this, &FMeshEditorModule::CreateToolbarExtension));
#else
	ToolbarExtender->AddToolBarExtension(
		"LocalToWorld",
		EExtensionHook::After,
		SnappingHelperActions,
		FToolBarExtensionDelegate::CreateRaw(this, &FSnappingHelperModule::CreateToolbarExtension));
#endif

	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	// OnPerspectiveTypeActive.BindRaw(this, &FSnappingHelperModule::PerspectiveTypeChanged);
}

void FMeshEditorModule::OnMeshEditorModeStateChanged(ECheckBoxState InState)
{
	if (InState == ECheckBoxState::Checked)
	{
		ActivateEdMode();
	}
	else
	{
		DeactivateEdMode();
	}	
}


void FMeshEditorModule::CreateToolbarExtension(FToolBarBuilder& InToolbarBuilder)
{
	InToolbarBuilder.BeginSection("MeshEditor");

	InToolbarBuilder.AddWidget(SNew(SMeshEditorComboButton)
		.Cursor(EMouseCursor::Default)
		.IsMeshEditorChecked_Raw(this, &FMeshEditorModule::IsMeshEditorModeChecked)
		.OnMeshEditorCheckStateChanged_Raw(this, &FMeshEditorModule::OnMeshEditorModeStateChanged)
		);
	
	InToolbarBuilder.EndSection();	
}


void FMeshEditorModule::ShutdownModule()
{
	FMeshEditorStyle::Shutdown();

	FEditorModeRegistry::Get().UnregisterMode(FMeshEditorEditorMode::EM_MeshEditorEditorModeId);
	// OnPerspectiveTypeActive.Unbind();
}

void FMeshEditorModule::OnMeshEditorModeToggle()
{
	if (IsMeshEditorModeChecked() == ECheckBoxState::Unchecked)
	{
		ActivateEdMode();
	}
	else
	{
		DeactivateEdMode();
	}
}

ECheckBoxState FMeshEditorModule::IsMeshEditorModeChecked() const
{
	ECheckBoxState OutState;
	GLevelEditorModeTools().IsModeActive(FMeshEditorEditorMode::EM_MeshEditorEditorModeId)
		? OutState = ECheckBoxState::Checked
		: OutState = ECheckBoxState::Unchecked;

	if (const FMeshEditorEditorMode* EdMode{GetMeshEditorEditorMode()})
	{
		EdMode->bIsModeOn ? OutState = ECheckBoxState::Checked : OutState = ECheckBoxState::Unchecked;
	}

	return OutState;
}

void FMeshEditorModule::ActivateEdMode()
{
	GLevelEditorModeTools().ActivateMode(FMeshEditorEditorMode::EM_MeshEditorEditorModeId);
	if (FMeshEditorEditorMode* EdMode {GetMeshEditorEditorMode()})
	{
		EdMode->bIsModeOn = true;
	}	
}

void FMeshEditorModule::DeactivateEdMode()
{
	GLevelEditorModeTools().DeactivateMode(FMeshEditorEditorMode::EM_MeshEditorEditorModeId);
}

FMeshEditorEditorMode* FMeshEditorModule::GetMeshEditorEditorMode() const
{
	return static_cast<FMeshEditorEditorMode*>(GLevelEditorModeTools().GetActiveMode(FMeshEditorEditorMode::EM_MeshEditorEditorModeId));
}

void FMeshEditorModule::BindCommands()
{
	FMeshEditorEditorModeCommands::Register();
	MeshEditorActions = MakeShareable(new FUICommandList);

	MeshEditorActions->MapAction(
		FMeshEditorEditorModeCommands::Get().OpenMeshEditorMode,
		FExecuteAction::CreateRaw(this, &FMeshEditorModule::OnMeshEditorModeToggle),
		FCanExecuteAction());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMeshEditorModule, MeshEditorEditorMode)
