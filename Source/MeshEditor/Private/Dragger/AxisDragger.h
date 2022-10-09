#pragma once

#include "CoreMinimal.h"
#include "HitProxies.h"
#include "UObject/GCObject.h"

struct FMovementParams
{
	/** The normal of the plane to project onto */
	FVector PlaneNormal;
	/** A vector that represent any displacement we want to mute (remove an axis if we're doing axis movement)*/
	FVector NormalToRemove;
	/** The current position of the widget */
	FVector Position;

	//Coordinate System Axes
	FVector XAxis;
	FVector YAxis;
	FVector ZAxis;

	//true if camera movement is locked to the object
	bool bMovementLockedToCamera;

	//Direction in world space to the current mouse location
	FVector PixelDir;
	//Direction in world space of the middle of the camera
	FVector CameraDir;
	FVector EyePos;

	//whether to snap the requested positionto the grid
	bool bPositionSnapping;
};

class FAxisDragger : public FGCObject
{
public:
	FAxisDragger();

	void RenderAxis(FPrimitiveDrawInterface* PDI, const FSceneView* View, FTransform& InTransform,
	                const FVector& InLocation, EAxisList::Type InAxis, bool bFlipped, bool bCubeHead = false);

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetReferencerName() const override
	{
		return "FAxisWidget";
	}

	void SetCurrentAxis(EAxisList::Type InAixsType, bool InFlipped = true)
	{
		CurrentAxisType = InAixsType;
		CurrentAxisFlipped = InFlipped;
	}

	EAxisList::Type GetCurrentAxisType() const
	{
		return CurrentAxisType;
	}

	bool IsAxisFlipped() const
	{
		return CurrentAxisFlipped;
	}

	void SetAxisBaseVerts(const TArray<FVector>& InBaseVerts);

	FVector GetCurrentAxisBaseVertex() const;

	FVector GetOppositeAxisBaseVertex() const;

	void SetTransform(const FTransform& InTransform)
	{
		WidgetTransform = InTransform;
	}

	const FTransform& GetTransform() const
	{
		return WidgetTransform;
	}

	void SetMeshActor(TWeakObjectPtr<AStaticMeshActor> InActor)
	{
		MeshActorPtr = InActor;
	}

	TWeakObjectPtr<AStaticMeshActor> GetMeshActor()
	{
		return MeshActorPtr;
	}

	void AbsoluteTranslationConvertMouseToDragRot(const FSceneView* InView, FEditorViewportClient* InViewportClient,
	                                              FVector& OutDrag, FRotator& OutRotation, FVector& OutScale);

	void ResetInitialTranslationOffset(void)
	{
		bAbsoluteTranslationInitialOffsetCached = false;
	}

private:
	void AbsoluteTranslationConvertMouseMovementToAxisMovement(const FSceneView* InView,
	                                                           FEditorViewportClient* InViewportClient,
	                                                           const FVector& InLocation,
	                                                           const FVector2D& InMousePosition,
	                                                           FVector& OutDrag, FRotator& OutRotation,
	                                                           FVector& OutScale);

	FVector GetAbsoluteTranslationDelta(const FMovementParams& InParams);

	void GetAxisPlaneNormalAndMask(const FTransform& InTransform, const FVector& InAxis, const FVector& InDirToPixel,
	                               FVector& OutPlaneNormal, FVector& NormalToRemove);

	void AbsoluteConvertMouseToAxis_Translate(const FSceneView* InView, const FTransform& InTransform,
	                                          EAxisList::Type InAxis, FMovementParams& InOutParams,
	                                          FVector& OutDrag);

	FVector GetAbsoluteTranslationInitialOffset(const FVector& InNewPosition, const FVector& InCurrentPosition);

private:
	UMaterialInstanceDynamic* AxisMaterial;
	UMaterialInstanceDynamic* CurrentAxisMaterial;

	FTransform WidgetTransform;

	EAxisList::Type CurrentAxisType;
	bool CurrentAxisFlipped;

	TArray<FVector> BaseVerts;
	TWeakObjectPtr<AStaticMeshActor> MeshActorPtr;

	bool bAbsoluteTranslationInitialOffsetCached;
	FVector InitialTranslationOffset;
	FVector InitialTranslationPosition;
};

class HAxisDraggerProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()

	EAxisList::Type Axis;
	bool bFlipped;

	HAxisDraggerProxy(EAxisList::Type InAxis, bool InFlipped) : Axis(InAxis), bFlipped(InFlipped)
	{
	}
};
