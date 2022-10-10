// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"
#include "Dragger/AxisDragger.h"
#include "Dragger/DragTransaction.h"
#include "MeshEditorEditorMode.generated.h"

DECLARE_DELEGATE(FOnCollectingMeshDataFinished);

struct FMeshEdgeData
{
	FVector FirstEndpointInWorldPosition{};
	FVector SecondEndpointInWorldPosition{};
	FVector2D FirstEndpointOnScreenPosition{};
	FVector2D SecondEndpointOnScreenPosition{};
	AActor* EdgeOwnerActor{nullptr};
};

UCLASS()
class UMeshGeoData : public UObject
{
	GENERATED_BODY()
public:
	void EraseSelection();

	void SaveStatus();
	
public:
	UPROPERTY()
	TArray<AActor*> SelectedActors;
	// TArray<TWeakObjectPtr<AActor>> SelectedActors;
	
	UPROPERTY()
	FVector SelectedLocation{FVector::ZeroVector};
};

class FMeshEditorEditorMode : public FEdMode
{
public:
	const static FEditorModeID EM_MeshEditorEditorModeId;

	FMeshEditorEditorMode();
	virtual ~FMeshEditorEditorMode();

	virtual void Enter() override;
	virtual void Exit() override;

	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
	                         const FViewportClick& Click) override;
	virtual bool HandleAxisWidgetDelta(FEditorViewportClient* InViewportClient, const FVector& InDrag,
	                                   const FRotator& InRot, const FVector& InScale);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
	                      EInputEvent Event) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag,
	                        FRotator& InRot, FVector& InScale) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View,
					 FCanvas* Canvas) override;

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;

	virtual FVector GetWidgetLocation() const override;

	virtual void ActorSelectionChangeNotify() override;
	
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;

	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;

	void AsyncCollectMeshData();

	void CollectingMeshDataFinished();

	void InvalidateHitProxies();

private:
	void EraseDroppingPreview();

	void CollectCursorData(const FSceneView* InSceneView);
	
	void CollectPressedKeysData(const FViewport* InViewport);

	void DrawBoxDraggerForStaticMeshActor(FPrimitiveDrawInterface* PDI, const FSceneView* View, FViewport* Viewport,
										  TWeakObjectPtr<AStaticMeshActor> MeshActorPtr);

	void DrawBracketForMeshComp(FPrimitiveDrawInterface* PDI, const UStaticMeshComponent* InMeshComp,
								TArray<FVector>& OutVerts);

	void DrawDraggerForMeshComp(FPrimitiveDrawInterface* PDI, const FSceneView* View, FViewport* Viewport,
								const UStaticMeshComponent* InMeshComp, const TArray<FVector>& InCorners);

	void UpdateSelection();

	void UpdateInitialSelection();

	FVector2D GetMouseVector2D();

	bool HandleEdgeClickEvent(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
										const FViewportClick& Click);
	
	bool HandlePrefabClickEvent(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
										const FViewportClick& Click);

public:
	TArray<FMeshEdgeData> LastCapturedEdgeData;
	TArray<FMeshEdgeData> CapturedEdgeData;
	FOnCollectingMeshDataFinished OnCollectingDataFinished{};
	FTimerHandle CollectVerticesTimerHandle{};
	FTimerHandle InvalidateHitProxiesTimerHandle{};
	bool bIsModeOn{false};
	
private:
	FAxisDragger* AxisDragger;
	const FSceneView* EdModeView;
	FDragTransaction DragTransaction;

	bool bPreviousDroppingPreview{false};
	bool bIsLeftMouseButtonDown{false};
	bool bDataCollectionInProgress{false};
	bool bIsMouseMove{false};
	bool bIsTracking = false;
	bool bIsCtrlKeyDown = {false};

	float DPIScale{1.f};
	FVector2D MouseOnScreenPosition{};

	UMeshGeoData* CurrentMeshData{nullptr};
};
