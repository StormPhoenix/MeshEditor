#include "AxisDragger.h"
#include "Materials/MaterialInstanceDynamic.h"


FAxisDragger::FAxisDragger()
{
	// FLinearColor AxisColor = FLinearColor(0.594f, 0.0197f, 0.0f);
	FLinearColor AxisColor = FLinearColor(0.0f, 1.0f, 0.0f);
	FLinearColor CurrentColor = FColor::Yellow;

	UMaterial* AxisMaterialBase = GEngine->ArrowMaterial;
	AxisMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, nullptr);
	AxisMaterial->SetVectorParameterValue("GizmoColor", AxisColor);

	CurrentAxisMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, nullptr);
	CurrentAxisMaterial->SetVectorParameterValue("GizmoColor", CurrentColor);

	bAbsoluteTranslationInitialOffsetCached = false;
	InitialTranslationOffset = FVector::ZeroVector;
	InitialTranslationPosition = FVector(0, 0, 0);

	CurrentAxisType = EAxisList::Type::None;
	CurrentAxisFlipped = false;
}

FMatrix CalculateAxisHeadRotationMatrix(EAxisList::Type InAxis, FVector3d Scale3D, bool bFlipped)
{
	float ReverseFactor[3] = {1.0f, 1.0f, 1.0f};
	ReverseFactor[0] *= (Scale3D[0] < 0.f ? -1.f : 1.f);
	ReverseFactor[1] *= (Scale3D[1] < 0.f ? -1.f : 1.f);
	ReverseFactor[2] *= (Scale3D[2] < 0.f ? -1.f : 1.f);

	FMatrix AxisRotation = FMatrix::Identity;
	if (InAxis == EAxisList::Y)
	{
		if (bFlipped)
		{
			AxisRotation = FRotationMatrix::MakeFromXZ(FVector(0, -1 * ReverseFactor[1], 0), FVector(0, 0, 1));
		}
		else
		{
			AxisRotation = FRotationMatrix::MakeFromXZ(FVector(0, 1 * ReverseFactor[1], 0), FVector(0, 0, 1));
		}
	}
	else if (InAxis == EAxisList::Z)
	{
		if (bFlipped)
		{
			AxisRotation = FRotationMatrix::MakeFromXY(FVector(0, 0, -1 * ReverseFactor[2]), FVector(0, 1, 0));
		}
		else
		{
			AxisRotation = FRotationMatrix::MakeFromXY(FVector(0, 0, 1 * ReverseFactor[2]), FVector(0, 1, 0));
		}
	}
	else
	{
		if (bFlipped)
		{
			AxisRotation = FMatrix::Identity * FScaleMatrix(FVector(-1.0f * ReverseFactor[0], 1.0f, 1.0f));
		}
		else
		{
			AxisRotation = FMatrix::Identity * FScaleMatrix(FVector(1.0f * ReverseFactor[0], 1.0f, 1.0f));
		}
	}
	return AxisRotation;
}

FRotationMatrix CalculateCylinderRotationMatrix(EAxisList::Type InAxis, FVector3d Scale3D, bool bFlipped)
{
	float ReverseFactor[3] = {1.0f, 1.0f, 1.0f};
	ReverseFactor[0] *= (Scale3D[0] < 0.f ? -1.f : 1.f);
	ReverseFactor[1] *= (Scale3D[1] < 0.f ? -1.f : 1.f);
	ReverseFactor[2] *= (Scale3D[2] < 0.f ? -1.f : 1.f);

	FRotationMatrix RotationMatrix = FRotationMatrix(FRotator(-90, 0.f, 0.f));
	if (InAxis == EAxisList::X)
	{
		if (bFlipped)
		{
			RotationMatrix = FRotationMatrix(FRotator(90 * ReverseFactor[0], 0.f, 0));
		}
		else
		{
			RotationMatrix = FRotationMatrix(FRotator(-90 * ReverseFactor[0], 0.f, 0));
		}
	}
	else if (InAxis == EAxisList::Y)
	{
		if (bFlipped)
		{
			RotationMatrix = FRotationMatrix(FRotator(0.f, 0.f, -90 * ReverseFactor[1]));
		}
		else
		{
			RotationMatrix = FRotationMatrix(FRotator(0.f, 0.f, 90 * ReverseFactor[1]));
		}
	}
	else
	{
		float PitchAngle = -180.f;
		if (bFlipped)
		{
			if (ReverseFactor[2] < 0.f)
			{
				PitchAngle = 0.f;
			}
			RotationMatrix = FRotationMatrix(FRotator(PitchAngle, 0.f, 0.f));
		}
		else
		{
			if (ReverseFactor[2] < 0.f)
			{
				PitchAngle = -180.f;
			}
			else
			{
				PitchAngle = 0.f;
			}
			RotationMatrix = FRotationMatrix(FRotator(PitchAngle, 0.f, 0.f));
		}
	}
	return RotationMatrix;
}

void FAxisDragger::RenderAxis(FPrimitiveDrawInterface* PDI, const FSceneView* View, const FVector& InLocation,
                              EAxisList::Type InAxis, bool bFlipped, bool bCubeHead)
{
	float UniformScale = 1.0f * View->WorldToScreen(InLocation).W * (4.0f / View->UnscaledViewRect.Width() / View
		->ViewMatrices.GetProjectionMatrix().M[0][0]);
	FScaleMatrix Scale(UniformScale);
	FVector FlattenScale = FVector(UniformScale == 1.0f ? 1.0f / UniformScale : 1.0f,
	                               UniformScale == 1.0f ? 1.0f / UniformScale : 1.0f,
	                               UniformScale == 1.0f ? 1.0f / UniformScale : 1.0f);

	FRotationMatrix RotationMatrix = CalculateCylinderRotationMatrix(InAxis, WidgetTransform.GetScale3D(), bFlipped);
	FMatrix WidgetMatrix = WidgetTransform.GetRotation().ToMatrix() * FTranslationMatrix(InLocation);

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

	FMatrix AxisRotation = CalculateAxisHeadRotationMatrix(InAxis, WidgetTransform.GetScale3D(), bFlipped);
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
		DrawCone(PDI, (FScaleMatrix(-13) * FTranslationMatrix(RootPos) * ArrowToWorld),
		         Angle, Angle, 32, false, FColor::White, InMaterial->GetRenderProxy(), SDPG_Foreground);
	}
	PDI->SetHitProxy(nullptr);
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

FVector FAxisDragger::GetAbsoluteTranslationInitialOffset(const FVector& InNewPosition,
                                                          const FVector& InCurrentPosition)
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

void ComputeMeshBaseVerts(TWeakObjectPtr<AStaticMeshActor> MeshPtr, TArray<FVector>& OutBaseVerts)
{
	// 注意，计算的是世界坐标包围盒，而不是局部坐标包围盒

	FBox ActorBounds(EForceInit::ForceInit);
	if (MeshPtr.Get())
	{
		for (const UActorComponent* ActorComp : MeshPtr->GetComponents())
		{
			const UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(ActorComp);
			if (MeshComp != nullptr)
			{
				ActorBounds += MeshComp->Bounds.GetBox();
			}
		}
	}

	FVector MinVector, MaxVector;
	MinVector = FVector(BIG_NUMBER);
	MaxVector = FVector(-BIG_NUMBER);

	// MinVector
	MinVector.X = FMath::Min<float>(ActorBounds.Min.X, MinVector.X);
	MinVector.Y = FMath::Min<float>(ActorBounds.Min.Y, MinVector.Y);
	MinVector.Z = FMath::Min<float>(ActorBounds.Min.Z, MinVector.Z);
	// MaxVector
	MaxVector.X = FMath::Max<float>(ActorBounds.Max.X, MaxVector.X);
	MaxVector.Y = FMath::Max<float>(ActorBounds.Max.Y, MaxVector.Y);
	MaxVector.Z = FMath::Max<float>(ActorBounds.Max.Z, MaxVector.Z);

	TArray<FVector> BracketCorners;

	// Bottom Corners
	BracketCorners.Add(FVector(MinVector.X, MinVector.Y, MinVector.Z));
	BracketCorners.Add(FVector(MinVector.X, MaxVector.Y, MinVector.Z));
	BracketCorners.Add(FVector(MaxVector.X, MaxVector.Y, MinVector.Z));
	BracketCorners.Add(FVector(MaxVector.X, MinVector.Y, MinVector.Z));

	// Top Corners
	BracketCorners.Add(FVector(MinVector.X, MinVector.Y, MaxVector.Z));
	BracketCorners.Add(FVector(MinVector.X, MaxVector.Y, MaxVector.Z));
	BracketCorners.Add(FVector(MaxVector.X, MaxVector.Y, MaxVector.Z));
	BracketCorners.Add(FVector(MaxVector.X, MinVector.Y, MaxVector.Z));


	// 计算 BaseVerts
	// Bottom
	OutBaseVerts.Add((BracketCorners[0] + BracketCorners[1] + BracketCorners[2] + BracketCorners[3]) / 4.0f);
	// Top
	OutBaseVerts.Add((BracketCorners[4] + BracketCorners[5] + BracketCorners[6] + BracketCorners[7]) / 4.0f);
	// Left
	OutBaseVerts.Add((BracketCorners[0] + BracketCorners[1] + BracketCorners[5] + BracketCorners[4]) / 4.0f);
	// Right
	OutBaseVerts.Add((BracketCorners[3] + BracketCorners[2] + BracketCorners[6] + BracketCorners[7]) / 4.0f);
	// Back
	OutBaseVerts.Add((BracketCorners[0] + BracketCorners[3] + BracketCorners[4] + BracketCorners[7]) / 4.0f);
	// Front
	OutBaseVerts.Add((BracketCorners[1] + BracketCorners[2] + BracketCorners[5] + BracketCorners[6]) / 4.0f);
}

FVector FAxisDragger::GetMeshBaseVertex(const TArray<FVector>& InBaseVerts) const 
{
	if (InBaseVerts.Num() <= 0)
	{
		return FVector::ZeroVector;
	}

	if (CurrentAxisType == EAxisList::X)
	{
		if (CurrentAxisFlipped)
		{
			return InBaseVerts[3];
		}
		return InBaseVerts[2];
	}
	if (CurrentAxisType == EAxisList::Y)
	{
		if (CurrentAxisFlipped)
		{
			return InBaseVerts[5];
		}
		return InBaseVerts[4];
	}
	if (CurrentAxisType == EAxisList::Z)
	{
		if (CurrentAxisFlipped)
		{
			return InBaseVerts[1];
		}
		return InBaseVerts[0];
	}
	return FVector::Zero();
}


FVector FAxisDragger::GetMeshBaseVertex(int MeshIndex) const
{
	if (IsMeshDragger())
	{
		return GetOppositeAxisBaseVertex();
	}
	else if (IsGroupDragger())
	{
		if (MeshIndex >= 0 && MeshIndex < GroupMeshInfo.EachMeshBaseVerts.Num())
		{
			const TArray<FVector>& MeshBaseVerts = GroupMeshInfo.EachMeshBaseVerts[MeshIndex];
			return GetMeshBaseVertex(MeshBaseVerts);
		}
	}
	else
	{
		// Do nothing
	}
	return FVector::Zero();
}


void FAxisDragger::UpdateControlledMeshGroup(TArray<TWeakObjectPtr<AStaticMeshActor>> InActors)
{
	if (InActors.Num() == 1)
	{
		UpdateControlledMesh(InActors[0]);

		// TODO 是否要设置其他信息
	}
	else if (InActors.Num() > 1)
	{
		GroupMeshInfo.GroupMeshArray.Empty();
		GroupMeshInfo.EachMeshBaseVerts.Empty();
		for (auto MeshActor : InActors)
		{
			GroupMeshInfo.GroupMeshArray.Add(MeshActor);
			TArray<FVector> EachBaseVerts;
			ComputeMeshBaseVerts(MeshActorPtr, EachBaseVerts);
			GroupMeshInfo.EachMeshBaseVerts.Add(EachBaseVerts);
		}
		MeshActorPtr = nullptr;
		WidgetTransform.SetComponents(UE::Math::TQuat<double>(EForceInit::ForceInit),
		                              UE::Math::TVector<double>(0.0f, 0.0f, 0.0f),
		                              UE::Math::TVector<double>(1.0f, 1.0f, 1.0f));
		// TODO 是否要设置其他信息
	}
	else
	{
		// Clean controlled mesh infos
		MeshActorPtr = nullptr;
		GroupMeshInfo.GroupMeshArray.Empty();
		GroupMeshInfo.EachMeshBaseVerts.Empty();
	}
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
		return BaseVerts[2];
	}
	if (CurrentAxisType == EAxisList::Y)
	{
		if (!CurrentAxisFlipped)
		{
			return BaseVerts[5];
		}
		return BaseVerts[4];
	}
	if (CurrentAxisType == EAxisList::Z)
	{
		if (!CurrentAxisFlipped)
		{
			return BaseVerts[1];
		}
		return BaseVerts[0];
	}
	return FVector::Zero();
}

FVector FAxisDragger::GetOppositeAxisBaseVertex() const
{
	return GetMeshBaseVertex(BaseVerts);
}

IMPLEMENT_HIT_PROXY(HAxisDraggerProxy, HHitProxy);
