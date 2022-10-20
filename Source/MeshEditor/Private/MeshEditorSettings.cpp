// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshEditorSettings.h"

const UMeshEditorSettings* UMeshEditorSettings::Get()
{
	return GetDefault<UMeshEditorSettings>();
}
