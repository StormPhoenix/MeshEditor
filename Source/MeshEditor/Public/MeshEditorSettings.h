// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "MeshEditorSettings.generated.h"

UCLASS(Config = Plugins)
class MESHEDITOR_API UMeshEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category = "ColorSettings|MeshEdgeSettings")
	FColor MeshEdgeColor {FColor::Orange};
	
	UPROPERTY(Config, EditAnywhere, Category = "ColorSettings|LineSettings")
	float MeshEdgeThickness {1.0f};
	
	static const UMeshEditorSettings* Get();
};
