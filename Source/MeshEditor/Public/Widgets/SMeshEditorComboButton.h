// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class MESHEDITOR_API SMeshEditorComboButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMeshEditorComboButton)
		{
		}

		SLATE_EVENT(FOnCheckStateChanged, OnMeshEditorCheckStateChanged)

		SLATE_ATTRIBUTE(ECheckBoxState, IsMeshEditorChecked)

		SLATE_ATTRIBUTE(bool, IsMeshEditorEnabled)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
};
