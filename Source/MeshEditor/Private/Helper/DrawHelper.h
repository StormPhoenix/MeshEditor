// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dragger/AxisDragger.h"
// #include "DrawHelper.generated.h"


namespace DrawHelper
{
	void DrawBrackets(FPrimitiveDrawInterface* PDI, const FLinearColor DrawColor, const float LineThickness,
	                  TArray<FVector>& InBracketCorners, TArray<FVector>& InGlobalAxis);

	void DrawBracketForDragger(FPrimitiveDrawInterface* PDI, FViewport* Viewport, FAxisDragger* Dragger,
	                           TArray<FVector>& OutVerts);
};
