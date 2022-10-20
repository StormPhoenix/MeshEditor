// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SMeshEditorComboButton.h"
#include "MeshEditorStyle.h"
#include "MeshEditorEditorModeCommands.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMeshEditorComboButton::Construct(const FArguments& InArgs)
{
	const FSlateIcon& Icon3D = FMeshEditorEditorModeCommands::Get().OpenMeshEditorMode->GetIcon();

#if ENGINE_MAJOR_VERSION == 5

	const FCheckBoxStyle& CheckBoxStyle = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>(
		"EditorViewportToolBar.ComboMenu.ToggleButton");

	TSharedPtr<SCheckBox> MeshEditorCheckBox;
	{
		MeshEditorCheckBox = SNew(SCheckBox)
			.Cursor(EMouseCursor::Default)
			.Padding(FMargin(4.0f))
			.Style(&CheckBoxStyle)
			.IsChecked(InArgs._IsMeshEditorChecked)
			.OnCheckStateChanged(InArgs._OnMeshEditorCheckStateChanged)
			.ToolTipText(FMeshEditorEditorModeCommands::Get().OpenMeshEditorMode->GetDescription())
			.Content()
		[
			SNew(SBox)
			.WidthOverride(16)
			.HeightOverride(16)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(Icon3D.GetIcon())
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("EditorViewportToolBar.Group"))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MeshEditorCheckBox->AsShared()
			]
			// + SHorizontalBox::Slot()
			// .AutoWidth()
			// [
			// SNew(SBorder)
			// .Padding(FMargin(1.0f, 0.0f, 0.0f, 0.0f))
			// .BorderImage(FMeshEditorStyle::Get().GetBrush("MeshEditor.Gray"))
			// ]
		]
	];

#else

	const FName CheckboxStyle = FEditorStyle::Join("ViewportMenu", ".ToggleButton");

	TSharedPtr<SCheckBox> Snapping3DCheckBox;
	{
		Snapping3DCheckBox = SNew(SCheckBox)
			.Cursor(EMouseCursor::Default)
			.Padding(FMargin(4.0f))
			.Style(FEditorStyle::Get(), ToName(CheckboxStyle, EMultiBlockLocation::Start))
			.IsChecked(InArgs._Is3DChecked)
			.OnCheckStateChanged(InArgs._On3DCheckStateChanged)
			.ToolTipText(FMeshEditorEditorModeCommands::Get().SnappingHelper3D->GetDescription())
			.Content()
		[
			SNew(SBox)
			.WidthOverride(16)
			.HeightOverride(16)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(Icon3D.GetIcon())
			]
		];
	}

	TSharedPtr<SCheckBox> Snapping2DCheckBox;
	{
		Snapping2DCheckBox = SNew(SCheckBox)
			.Cursor(EMouseCursor::Default)
			.Padding(FMargin(4.0f))
			.Style(FEditorStyle::Get(), ToName(CheckboxStyle, EMultiBlockLocation::End))
			.IsEnabled(InArgs._Is2DEnabled)
			.IsChecked(InArgs._Is2DChecked)
			.OnCheckStateChanged(InArgs._On2DCheckStateChanged)
			.ToolTipText(FMeshEditorEditorModeCommands::Get().SnappingHelper2D->GetDescription())
			.Content()
		[
			SNew(SBox)
			.WidthOverride(16)
			.HeightOverride(16)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(Icon2D.GetIcon())
			]
		];
	}

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Snapping3DCheckBox->AsShared()
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBorder)
			.Padding(FMargin(1.0f, 0.0f, 0.0f, 0.0f))
			.BorderImage(FEditorStyle::GetDefaultBrush())
			.BorderBackgroundColor(FLinearColor::Black)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Snapping2DCheckBox->AsShared()
		]
	];
#endif
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
