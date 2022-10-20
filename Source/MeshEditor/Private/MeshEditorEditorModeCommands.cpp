// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshEditorEditorModeCommands.h"
#include "MeshEditorEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "MeshEditorEditorModeCommands"

void FMeshEditorEditorModeCommands::RegisterCommands()
{
	UI_COMMAND(OpenMeshEditorMode, "Mesh Editor Mode", "On/Off Mesh Editor", EUserInterfaceActionType::ToggleButton,
	           FInputChord(EKeys::Three, EModifierKey::Alt | EModifierKey::Shift));
}


#undef LOCTEXT_NAMESPACE
