// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MeshEditorStyle.h"

/**
 * This class contains info about the full set of commands used in this editor mode.
 */
class FMeshEditorEditorModeCommands : public TCommands<FMeshEditorEditorModeCommands>
{
public:
	FMeshEditorEditorModeCommands()
	: TCommands<FMeshEditorEditorModeCommands>(TEXT("MeshEditor"), NSLOCTEXT("Contexts", "MeshEditor", "MeshEditor Plugin"), NAME_None, FMeshEditorStyle::GetStyleSetName()){}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> OpenMeshEditorMode;
};
