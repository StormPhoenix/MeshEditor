#include "MeshEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FMeshEditorStyle::StyleInstance{nullptr};

namespace FMeshEditorStyleLocal
{
	const FName StyleSetName(TEXT("MeshEditorStyle"));
}

void FMeshEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMeshEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FMeshEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FMeshEditorStyle::Get()
{
	return *StyleInstance;
}

FName FMeshEditorStyle::GetStyleSetName()
{
	return FMeshEditorStyleLocal::StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);

TSharedRef<FSlateStyleSet> FMeshEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("MeshEditorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MeshEditor")->GetBaseDir() / TEXT("Resources"));

#if ENGINE_MAJOR_VERSION == 5
	Style->Set("MeshEditor.OpenMeshEditorMode", new IMAGE_BRUSH(TEXT("CheckBox3d_UE5_16x"), Icon16x16));
	Style->Set("MeshEditor.Gray", new IMAGE_BRUSH(TEXT("Gray_UE5_16x"), Icon16x16));
#else
	Style->Set("MeshEditor.MeshEditorMode", new IMAGE_BRUSH(TEXT("CheckBox3d_16x"), Icon16x16));
#endif
	return Style;
}

#undef IMAGE_BRUSH
