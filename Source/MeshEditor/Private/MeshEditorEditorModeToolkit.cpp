// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshEditorEditorModeToolkit.h"
#include "MeshEditorEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "MeshEditorEditorModeToolkit"

FMeshEditorEditorModeToolkit::FMeshEditorEditorModeToolkit()
{
}

void FMeshEditorEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FMeshEditorEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FMeshEditorEditorModeToolkit::GetToolkitFName() const
{
	return FName("MeshEditorEditorMode");
}

FText FMeshEditorEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "MeshEditorEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
