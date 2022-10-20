// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MeshEditorEditorMode.h"
#include "Modules/ModuleManager.h"

/**
 * This is the module definition for the editor mode. You can implement custom functionality
 * as your plugin module starts up and shuts down. See IModuleInterface for more extensibility options.
 */
class FMeshEditorModule : public IModuleInterface
{
public:
	TSharedPtr<FUICommandList> MeshEditorActions;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void BindCommands();
	void CreateToolbarExtension(FToolBarBuilder& InToolbarBuilder);

	ECheckBoxState IsMeshEditorModeChecked() const;
	void OnMeshEditorModeStateChanged(ECheckBoxState InState);
	void OnMeshEditorModeToggle();
	void ActivateEdMode();
	void DeactivateEdMode();

	FMeshEditorEditorMode* GetMeshEditorEditorMode() const;
};
