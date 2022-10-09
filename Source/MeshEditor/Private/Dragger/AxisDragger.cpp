#include "AxisDragger.h"
#include "Materials/MaterialInstanceDynamic.h"


FAxisDragger::FAxisDragger()
{
	// FLinearColor AxisColor = FLinearColor(0.594f, 0.0197f, 0.0f);
	FLinearColor AxisColor = FLinearColor(0.0f, 1.0f, 0.0f);
	FLinearColor CurrentColor = FColor::Yellow;

	UMaterial* AxisMaterialBase = GEngine->ArrowMaterial;
	AxisMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterial->SetVectorParameterValue("GizmoColor", AxisColor);

	CurrentAxisMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	CurrentAxisMaterial->SetVectorParameterValue("GizmoColor", CurrentColor);

	bAbsoluteTranslationInitialOffsetCached = false;
	InitialTranslationOffset = FVector::ZeroVector;
	InitialTranslationPosition = FVector(0, 0, 0);
}

FMatrix CalculateAxisHeadRotationMatrix(EAxisList::Type InAxis, bool bFlipped)
{
	FMatrix AxisRotation = FMatrix::Identity;
	if (InAxis == EAxisList::Y)
	{
		if (bFlipped)
		{
			AxisRotation = FRotationMatrix::MakeFromXZ(FVector(0, -1, 0), FVector(0, 0, 1));
		}
		else
		{
			AxisRotation = FRotationMatrix::MakeFromXZ(FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	else if (InAxis == EAxisList::Z)
	{
		if (bFlipped)
		{
			AxisRotation = FRotationMatrix::MakeFromXY(FVector(0, 0, -1), FVector(0, 1, 0));
		}
		else
		{
			AxisRotation = FRotationMatrix::MakeFromXY(FVector(0, 0, 1), FVector(0, 1, 0));
		}
	}
	else
	{
		if (bFlipped)
		{
			AxisRotation = FMatrix::Identity * FScaleMatrix(FVector(-1.0f, 1.0f, 1.0f));
		}
		else
		{
			AxisRotation = FMatrix::Identity;
		}
	}
	return AxisRotation;
}

FRotationMatrix CalculateCylinderRotationMatrix(EAxisList::Type InAxis, bool bFlipped)
{
	FRotationMatrix RotationMatrix = FRotationMatrix(FRotator(-90, 0.f, 0.f));
	if (InAxis == EAxisList::X)
	{
		if (bFlipped)
		{
			RotationMatrix = FRotationMatrix(FRotator(90, 0.f, 0));
		}
		else
		{
			RotationMatrix = FRotationMatrix(FRotator(-90, 0.f, 0));
		}
	}
	else if (InAxis == EAxisList::Y)
	{
		if (bFlipped)
		{
			RotationMatrix = FRotationMatrix(FRotator(0.f, 0.f, -90));
		}
		else
		{
			RotationMatrix = FRotationMatrix(FRotator(0.f, 0.f, 90));
		}
	}
	else
	{
		if (bFlipped)
		{
			RotationMatrix = FRotationMatrix(FRotator(-180.f, 0.f, 0.f));
		}
		else
		{
			RotationMatrix = FRotationMatrix(FRotator(0.f, 0.f, 0.f));
		}
	}
	return RotationMatrix;
}

void FAxisDragger::RenderAxis(FPrimitiveDrawInterface* PDI, const FSceneView* View,
                             FTransform& InTransform, const FVector& InLocation,
                             EAxisList::Type InAxis, bool bFlipped, bool bCubeHead)
{
	float UniformScale = 1.0f * View->WorldToScreen(InLocation).W * (4.0f / View->UnscaledViewRect.Width() / View
		->ViewMatrices.GetProjectionMatrix().M[0][0]);
	FScaleMatrix Scale(UniformScale);
	FVector FlattenScale = FVector(UniformScale == 1.0f ? 1.0f / UniformScale : 1.0f,
	                               UniformScale == 1.0f ? 1.0f / UniformScale : 1.0f,
	                               UniformScale == 1.0f ? 1.0f / UniformScale : 1.0f);

	FRotationMatrix RotationMatrix = CalculateCylinderRotationMatrix(InAxis, bFlipped);
	FMatrix WidgetMatrix = InTransform.GetRotation().ToMatrix() * FTranslationMatrix(InLocation);

	const float HalfHeight = 20.5;
	const float AxisLength = 2.0f * HalfHeight;
	const float CylinderRadius = 2.4;
	const FVector Offset(0, 0, HalfHeight);

	PDI->SetHitProxy(new HAxisDraggerProxy(InAxis, bFlipped));
	UMaterialInstanceDynamic* InMaterial = AxisMaterial;
	if (InAxis == CurrentAxisType && bFlipped == CurrentAxisFlipped)
	{
		InMaterial = CurrentAxisMaterial;
	}

	{
		// Draw cylinder
		DrawCylinder(
			PDI, Scale * RotationMatrix * WidgetMatrix, Offset, FVector(1, 0, 0),
			FVector(0, 1, 0), FVector(0, 0, 1), CylinderRadius, HalfHeight, 16,
			InMaterial->GetRenderProxy(), SDPG_Foreground);
	}

	FMatrix AxisRotation = CalculateAxisHeadRotationMatrix(InAxis, bFlipped);
	FMatrix ArrowToWorld = Scale * AxisRotation * WidgetMatrix;
	if (bCubeHead)
	{
		// Draw cube head
		const float CubeHeadOffset = 3.0f;
		FVector RootPos(AxisLength + CubeHeadOffset, 0, 0);
		const FMatrix CubeToWorld = FScaleMatrix(FVector(6.0f)) * (FTranslationMatrix(RootPos) * ArrowToWorld) *
			FScaleMatrix(FlattenScale);
		// const FMatrix CubeToWorld = FScaleMatrix(FVector(6.0f)) * (FTranslationMatrix(RootPos) * ArrowToWorld);
		DrawBox(PDI, CubeToWorld, FVector(1, 1, 1), InMaterial->GetRenderProxy(), SDPG_Foreground);
	}
	else
	{
		// Draw cone head
		const float ConeHeadOffset = 12.0f;
		// const float ConeHeadOffset = 0.0f;
		FVector RootPos(AxisLength + ConeHeadOffset, 0, 0);
		float Angle = FMath::DegreesToRadians(PI * 5);
		// DrawCone(PDI, (FScaleMatrix(-13) * FTranslationMatrix(RootPos) * ArrowToWorld) * FScaleMatrix(FlattenScale),
		DrawCone(PDI, (FScaleMatrix(-13) * FTranslationMatrix(RootPos) * ArrowToWorld),
		         Angle, Angle, 32, false, FColor::White, InMaterial->GetRenderProxy(), SDPG_Foreground);
	}
	PDI->SetHitProxy(NULL);
}

void FAxisDragger::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(AxisMaterial);
	Collector.AddReferencedObject(CurrentAxisMaterial);
}

void FAxisDragger::SetAxisBaseVerts(const TArray<FVector>& InBaseVerts)
{
	BaseVerts.Empty();
	for (const FVector& Base : InBaseVerts)
	{
		BaseVerts.Add(Base);
	}
}

void FAxisDragger::AbsoluteTranslationConvertMouseToDragRot(const FSceneView* InView,
                                                           FEditorViewportClient* InViewportClient,
                                                           FVector& OutDrag, FRotator& OutRotation,
                                                           FVector& OutScale)
{
	OutDrag = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	OutScale = FVector::ZeroVector;

	check(CurrentAxisType != EAxisList::None);

	//calculate mouse position
	check(InViewportClient->Viewport);
	FVector2D MousePosition(InViewportClient->Viewport->GetMouseX(), InViewportClient->Viewport->GetMouseY());

	AbsoluteTranslationConvertMouseMovementToAxisMovement(InView, InViewportClient, GetCurrentAxisBaseVertex(),
	                                                      MousePosition, OutDrag, OutRotation, OutScale);
}

FVector FAxisDragger::GetAbsoluteTranslationInitialOffset(const FVector& InNewPosition, const FVector& InCurrentPosition)
{
	if (!bAbsoluteTranslationInitialOffsetCached)
	{
		bAbsoluteTranslationInitialOffsetCached = true;
		InitialTranslationOffset = InNewPosition - InCurrentPosition;
		InitialTranslationPosition = InCurrentPosition;
	}
	return InitialTranslationOffset;
}

FVector FAxisDragger::GetAbsoluteTranslationDelta(const FMovementParams& InParams)
{
	FPlane MovementPlane(InParams.Position, InParams.PlaneNormal);
	FVector ProposedEndofEyeVector =
		InParams.EyePos + (InParams.PixelDir * (InParams.Position - InParams.EyePos).Size());

	//default to not moving
	FVector RequestedPosition = InParams.Position;

	float DotProductWithPlaneNormal = InParams.PixelDir | InParams.PlaneNormal;
	//check to make sure we're not co-planar
	if (FMath::Abs(DotProductWithPlaneNormal) > DELTA)
	{
		//Get closest point on plane
		RequestedPosition = FMath::LinePlaneIntersection(InParams.EyePos, ProposedEndofEyeVector, MovementPlane);
	}

	//drag is a delta position, so just update the different between the previous position and the new position
	FVector DeltaPosition = RequestedPosition - InParams.Position;

	//Retrieve the initial offset, passing in the current requested position and the current position
	FVector InitialOffset = GetAbsoluteTranslationInitialOffset(RequestedPosition, InParams.Position);

	//subtract off the initial offset (where the widget was clicked) to prevent popping
	DeltaPosition -= InitialOffset;

	//remove the component along the normal we want to mute
	float MovementAlongMutedAxis = DeltaPosition | InParams.NormalToRemove;
	FVector OutDrag = DeltaPosition - (InParams.NormalToRemove * MovementAlongMutedAxis);

	//Get the vector from the eye to the proposed new position (to make sure it's not behind the camera
	FVector EyeToNewPosition = (InParams.Position + OutDrag) - InParams.EyePos;
	float BehindTheCameraDotProduct = EyeToNewPosition | InParams.CameraDir;

	//Don't let the requested position go behind the camera
	if (BehindTheCameraDotProduct <= 0)
	{
		OutDrag = OutDrag.ZeroVector;
	}
	return OutDrag;
}

void FAxisDragger::GetAxisPlaneNormalAndMask(const FTransform& InTransform, const FVector& InAxis,
                                            const FVector& InDirToPixel, FVector& OutPlaneNormal,
                                            FVector& NormalToRemove)
{
	FVector XAxis = InTransform.TransformVectorNoScale(FVector(1, 0, 0));
	FVector YAxis = InTransform.TransformVectorNoScale(FVector(0, 1, 0));
	FVector ZAxis = InTransform.TransformVectorNoScale(FVector(0, 0, 1));

	float XDot = FMath::Abs(InDirToPixel | XAxis);
	float YDot = FMath::Abs(InDirToPixel | YAxis);
	float ZDot = FMath::Abs(InDirToPixel | ZAxis);

	if ((InAxis | XAxis) > .1f)
	{
		OutPlaneNormal = (YDot > ZDot) ? YAxis : ZAxis;
		NormalToRemove = (YDot > ZDot) ? ZAxis : YAxis;
	}
	else if ((InAxis | YAxis) > .1f)
	{
		OutPlaneNormal = (XDot > ZDot) ? XAxis : ZAxis;
		NormalToRemove = (XDot > ZDot) ? ZAxis : XAxis;
	}
	else
	{
		OutPlaneNormal = (XDot > YDot) ? XAxis : YAxis;
		NormalToRemove = (XDot > YDot) ? YAxis : XAxis;
	}
}

void FAxisDragger::AbsoluteConvertMouseToAxis_Translate(const FSceneView* InView, const FTransform& InTransform,
                                                       EAxisList::Type InAxis, FMovementParams& InOutParams,
                                                       FVector& OutDrag)
{
	switch (InAxis)
	{
	case EAxisList::X:
		GetAxisPlaneNormalAndMask(InTransform, InOutParams.XAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
		                          InOutParams.NormalToRemove);
		break;
	case EAxisList::Y:
		GetAxisPlaneNormalAndMask(InTransform, InOutParams.YAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
		                          InOutParams.NormalToRemove);
		break;
	case EAxisList::Z:
		GetAxisPlaneNormalAndMask(InTransform, InOutParams.ZAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
		                          InOutParams.NormalToRemove);
		break;
	}

	OutDrag = GetAbsoluteTranslationDelta(InOutParams);
}

void FAxisDragger::AbsoluteTranslationConvertMouseMovementToAxisMovement(const FSceneView* InView,
                                                                        FEditorViewportClient* InViewportClient,
                                                                        const FVector& InLocation,
                                                                        const FVector2D& InMousePosition,
                                                                        FVector& OutDrag,
                                                                        FRotator& OutRotation, FVector& OutScale)
{
	// Compute a world space ray from the screen space mouse coordinates
	FViewportCursorLocation MouseViewportRay(InView, InViewportClient, InMousePosition.X, InMousePosition.Y);

	FMovementParams Params;
	Params.EyePos = MouseViewportRay.GetOrigin();
	Params.PixelDir = MouseViewportRay.GetDirection();
	Params.CameraDir = InView->GetViewDirection();
	Params.Position = InLocation;
	//dampen by
	Params.bMovementLockedToCamera = InViewportClient->IsShiftPressed();
	Params.bPositionSnapping = true;

	const FTransform& WTransfrom = GetTransform();
	Params.XAxis = WTransfrom.TransformVectorNoScale(FVector(1, 0, 0));
	Params.YAxis = WTransfrom.TransformVectorNoScale(FVector(0, 1, 0));
	Params.ZAxis = WTransfrom.TransformVectorNoScale(FVector(0, 0, 1));

	AbsoluteConvertMouseToAxis_Translate(InView, WTransfrom, GetCurrentAxisType(), Params, OutDrag);
}

FVector FAxisDragger::GetCurrentAxisBaseVertex() const
{
	if (BaseVerts.Num() <= 0)
	{
		return FVector::ZeroVector;
	}
	
	if (CurrentAxisType == EAxisList::X)
	{
		if (!CurrentAxisFlipped)
		{
			return BaseVerts[3];
		}
		else
		{
			return BaseVerts[2];
		}
	}
	else if (CurrentAxisType == EAxisList::Y)
	{
		if (!CurrentAxisFlipped)
		{
			return BaseVerts[5];
		}
		else
		{
			return BaseVerts[4];
		}
	}
	else if (CurrentAxisType == EAxisList::Z)
	{
		if (!CurrentAxisFlipped)
		{
			return BaseVerts[1];
		}
		else
		{
			return BaseVerts[0];
		}
	}
	else
	{
		return FVector::Zero();
	}
}

FVector FAxisDragger::GetOppositeAxisBaseVertex() const
{
	if (BaseVerts.Num() <= 0)
	{
		return FVector::ZeroVector;		
	}
	
	if (CurrentAxisType == EAxisList::X)
	{
		if (CurrentAxisFlipped)
		{
			return BaseVerts[3];
		}
		else
		{
			return BaseVerts[2];
		}
	}
	else if (CurrentAxisType == EAxisList::Y)
	{
		if (CurrentAxisFlipped)
		{
			return BaseVerts[5];
		}
		else
		{
			return BaseVerts[4];
		}
	}
	else if (CurrentAxisType == EAxisList::Z)
	{
		if (CurrentAxisFlipped)
		{
			return BaseVerts[1];
		}
		else
		{
			return BaseVerts[0];
		}
	}
	else
	{
		return FVector::Zero();
	}
}

IMPLEMENT_HIT_PROXY(HAxisDraggerProxy, HHitProxy);
