#pragma once

#include "SceneView.h"
#include "Math/Axis.h"
#include "HitProxies.h"
#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "SceneManagement.h"
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

struct FGroupMeshInfo
{
	TArray<FVector> GroupBaseVerts;
	TArray<TWeakObjectPtr<AStaticMeshActor>> GroupMeshArray;
	TArray<TArray<FVector>> EachMeshBaseVerts;
};

enum EDragMode
{
	// Dragger 以包裹边界为基点放缩 
	WrapMode = 1,
	// Dragger 以中心点放缩
	CenterMode = 2,
};

class FAxisDragger : public FGCObject
{
public:
	FAxisDragger();

	void RenderHandler(FPrimitiveDrawInterface* PDI, const FSceneView* View);

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetReferencerName() const override
	{
		return "FAxisWidget";
	}

	void SetCurrentAxisType(EAxisList::Type InAixsType, bool InFlipped = true)
	{
		CurrentAxisType = InAixsType;
		CurrentAxisFlipped = InFlipped;
	}

	EAxisList::Type GetCurrentAxisType() const
	{
		return CurrentAxisType;
	}

	// 执行拖拽拉伸操作
	bool DoWork(FEditorViewportClient* InViewportClient, const FSceneView* EdModeView);

	void SetAxisBaseVerts(const TArray<FVector>& InBaseVerts);

	// 获取当前拖拽箭头的起始位置
	FVector GetCurrentAxisBaseVertex() const;

	// 获取当前箭头指向的包围盒的一面的中心点	
	FVector GetBoundFaceCenterByAxisDir() const;

	FVector GetOppositeAxisBaseVertex() const;

	const FTransform& GetTransform() const
	{
		return WidgetTransform;
	}

	void UpdateControlledMesh(TWeakObjectPtr<AStaticMeshActor> InActor)
	{
		MeshActorPtr = InActor;
		GroupMeshInfo.GroupMeshArray.Empty();
		GroupMeshInfo.EachMeshBaseVerts.Empty();
		WidgetTransform = MeshActorPtr->GetActorTransform();
	}

	void UpdateControlledMeshGroup(TArray<TWeakObjectPtr<AStaticMeshActor>> InActors);

	// TODO remove
	FVector GetMeshBaseVertex(int MeshIndex) const;

	TWeakObjectPtr<AStaticMeshActor> GetMeshActor()
	{
		return MeshActorPtr;
	}

	void SetDragMode(EDragMode Mode)
	{
		DragMode = Mode;
	}

	void ResetInitialTranslationOffset(void);

	// 是否处理一组 Mesh
	bool IsGroupDragger() const;

	// 是否处理单个 Mesh
	bool IsMeshDragger() const;

	bool IsWrapMode() const;

	bool IsCenterMode() const;
	
	void SwitchMode() {
		if (DragMode == WrapMode) {
			DragMode = CenterMode;
		} else {
			DragMode = WrapMode;
		}
	}

	FGroupMeshInfo& GetGroupMeshInfo()
	{
		return GroupMeshInfo;
	}

	// 获取 AxisDrager 的移动基点
	FVector GetExpectedPivot();

private:
	FVector GetControlledCenter() const;

	FVector GetCurrentBaseVertexInWarpMode(bool bAxisFlipped) const;

	FVector GetCurrentBaseVertexInCenterMode() const;

	void AbsoluteTranslationConvertMouseToDragRot(const FSceneView* InView, FEditorViewportClient* InViewportClient,
	                                              FVector& OutDrag, FRotator& OutRotation, FVector& OutScale);

	void AbsoluteTranslationConvertMouseMovementToAxisMovement(
		const FSceneView* InView, FEditorViewportClient* InViewportClient, const FVector& InLocation,
		const FVector2D& InMousePosition, FVector& OutDrag, FRotator& OutRotation, FVector& OutScale);

	FVector GetAbsoluteTranslationDelta(const FMovementParams& InParams);

	void GetAxisPlaneNormalAndMask(const FTransform& InTransform, const FVector& InAxis, const FVector& InDirToPixel,
	                               FVector& OutPlaneNormal, FVector& NormalToRemove);

	void AbsoluteConvertMouseToAxis_Translate(const FSceneView* InView, const FTransform& InTransform,
	                                          EAxisList::Type InAxis, FMovementParams& InOutParams,
	                                          FVector& OutDrag);

	FVector GetAbsoluteTranslationInitialOffset(const FVector& InNewPosition, const FVector& InCurrentPosition);

	// TODO remove
	FVector GetMeshBaseVertex(const TArray<FVector>& InBaseVerts) const;

	void RenderAxis(FPrimitiveDrawInterface* PDI, const FSceneView* View, const FVector& InLocation,
	                EAxisList::Type InAxis, bool bFlipped, bool bCubeHead = false);

private:
	UMaterialInstanceDynamic* WrapAxisMaterial;
	UMaterialInstanceDynamic* CenterAxisMaterial;
	UMaterialInstanceDynamic* CurrentAxisMaterial;

	// Dragger 从局部空间到世界坐标空间的变换
	FTransform WidgetTransform;

	EAxisList::Type CurrentAxisType;
	bool CurrentAxisFlipped;

	// Content = [ Bottom, Top, Left, Right, Back, Front ]
	TArray<FVector> BaseVerts;
	TWeakObjectPtr<AStaticMeshActor> MeshActorPtr;

	FGroupMeshInfo GroupMeshInfo;

	bool bAbsoluteTranslationInitialOffsetCached;
	FVector InitialTranslationOffset;
	FVector InitialTranslationPosition;

	EDragMode DragMode;
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
