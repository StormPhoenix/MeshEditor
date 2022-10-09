// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshEditorEditorMode.h"
#include "MeshEditorEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "MeshEditorEditorModeCommands.h"
#include "UnrealEd.h"
#include "Dragger/AxisDragger.h"
#include "Tools/MeshEditorSimpleTool.h"
#include "Tools/MeshEditorInteractiveTool.h"

// step 2: register a ToolBuilder in FMeshEditorEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "MeshEditorEditorMode"

const FEditorModeID FMeshEditorEditorMode::EM_MeshEditorEditorModeId = TEXT("EM_MeshEditorEditorMode");

// FString FMeshEditorEditorMode::SimpleToolName = TEXT("MeshEditor_ActorInfoTool");
// FString FMeshEditorEditorMode::InteractiveToolName = TEXT("MeshEditor_MeasureDistanceTool");

FMeshEditorEditorMode::FMeshEditorEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(FMeshEditorEditorMode::EM_MeshEditorEditorModeId,
	                       LOCTEXT("ModeName", "MeshEditor"),
	                       FSlateIcon(),
	                       true);
}


FMeshEditorEditorMode::~FMeshEditorEditorMode()
{
}

void FMeshEditorEditorMode::UpdateInitialSelection()
{
	TArray<AActor*> NewSelectedActors;

	USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
	CurrentEditorSelection->GetSelectedObjects<AActor>(NewSelectedActors);

	if (NewSelectedActors.Num() > 0)
	{
		CurrentMeshData->SelectedActors = NewSelectedActors;
	}
}

void FMeshEditorEditorMode::Enter()
{
	FEdMode::Enter();
	bIsModeOn = true;

	if (!IsValid(CurrentMeshData))
	{
		CurrentMeshData = NewObject<UMeshGeoData>();
		CurrentMeshData->AddToRoot();
		CurrentMeshData->SetFlags(RF_Transactional);
	}

	// TODO remove
	UE_LOG(LogTemp, Warning, TEXT("MeshEditorEditorMode - Enter() "));
	AxisDragger = new FAxisDragger();
	UpdateInitialSelection();
}


void FMeshEditorEditorMode::Exit()
{
	bIsModeOn = false;

	CurrentMeshData->EraseSelection();
	delete AxisDragger;

	FEdMode::Exit();
}

bool FMeshEditorEditorMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                        const FViewportClick& Click)
{
	UE_LOG(LogTemp, Warning, TEXT("MeshEditorEditorMode - HandleClick() "));

	bool bSnapHelperHandle{false};
	bool bPrefabHandle(false);

#ifdef WITH_EDITOR
	// Process HAxisWidget proxy
	if (HitProxy != nullptr && HitProxy->IsA(HAxisDraggerProxy::StaticGetType()))
	{
		const HAxisDraggerProxy* AxisWidgetProxy = static_cast<HAxisDraggerProxy*>(HitProxy);
		if (Click.GetEvent() == IE_Pressed)
		{
			UE_LOG(LogTemp, Warning, TEXT("HAxisWidget IE_Pressed. "));
		}
		else if (Click.GetEvent() == IE_Released)
		{
			// AxisWidget->SetCurrentAxis(AxisWidgetProxy->Axis, AxisWidgetProxy->bFlipped);
			UE_LOG(LogTemp, Warning, TEXT("HAxisWidget IE_Released. "));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HAxisWidget Clicked. "));
		}
	}
#endif
	return false;
}

float ComputeScaleFactor(FVector& Base, FVector& DragDelta, int Axis)
{
	float Result = 1.0;
	if (FMath::Abs(DragDelta[Axis]) > 0.001 && FMath::Abs(Base[Axis]) > 0.001)
	{
		float ScaleFactor = FMath::Abs(
			(Base[Axis] + DragDelta[Axis]) / Base[Axis]);
		if (!FMath::IsNaN(ScaleFactor))
		{
			Result = ScaleFactor;
		}
	}
	return Result;
}

bool FMeshEditorEditorMode::HandleAxisWidgetDelta(FEditorViewportClient* InViewportClient, const FVector& NotUsed0,
                                                  const FRotator& NotUsed1, const FVector& NotUsed2)
{
	if (AxisDragger->GetCurrentAxisType() == EAxisList::None)
	{
		return false;
	}

	FVector Drag(ForceInitToZero);
	FRotator Rot(ForceInitToZero);
	FVector Scale(ForceInitToZero);
	AxisDragger->AbsoluteTranslationConvertMouseToDragRot(EdModeView, InViewportClient, Drag, Rot, Scale);

	bool bDragSuccess = true;
	FVector DragInLocal = AxisDragger->GetTransform().InverseTransformVector(Drag);
	FVector BaseVertex = AxisDragger->GetOppositeAxisBaseVertex();

	// Compute scale factors
	FVector ScaleFactor{1.0f, 1.0f, 1.0f};
	{
		FVector AxisBaseVertex = AxisDragger->GetCurrentAxisBaseVertex();

		FVector OriginAxisVec = AxisBaseVertex - BaseVertex;
		// transform to local space
		FVector OriginAxisVecInLocal = AxisDragger->GetTransform().InverseTransformVector(OriginAxisVec);

		float DotValue = FMath::Abs(
			FVector::DotProduct(Drag.GetSafeNormal(), OriginAxisVec.GetSafeNormal()));
		bool bIsPerpendicular = (DotValue < 0.001);
		if (bIsPerpendicular)
		{
			bDragSuccess = false;
		}
		else
		{
			float DragLenLocal = FVector::DotProduct(OriginAxisVecInLocal.GetSafeNormal(), DragInLocal);
			DragInLocal = OriginAxisVecInLocal.GetSafeNormal() * DragLenLocal;
			for (int Axis = 0; Axis < 3; Axis ++)
			{
				ScaleFactor[Axis] = ComputeScaleFactor(OriginAxisVecInLocal, DragInLocal, Axis);
			}
		}
	}

	if (bDragSuccess)
	{
		TWeakObjectPtr<AStaticMeshActor> MeshActorPtr = AxisDragger->GetMeshActor();
		if (MeshActorPtr.Get())
		{
			FVector OldScale3D = MeshActorPtr->GetActorRelativeScale3D();
			MeshActorPtr->SetActorRelativeScale3D(ScaleFactor * OldScale3D);

			FVector ActorLocationToBaseVec = MeshActorPtr->GetActorLocation() - BaseVertex;
			FVector ActorLocationToBaseVecInLocal =
				AxisDragger->GetTransform().InverseTransformVector(ActorLocationToBaseVec);
			ActorLocationToBaseVecInLocal *= ScaleFactor;

			FVector FinalOffset = AxisDragger->GetTransform().TransformVector(ActorLocationToBaseVecInLocal);
			MeshActorPtr->SetActorLocation(BaseVertex + FinalOffset);
			MeshActorPtr->SetPivotOffset(FVector::ZeroVector);
			CurrentMeshData->SelectedLocation = MeshActorPtr->GetActorLocation();
			GUnrealEd->UpdatePivotLocationForSelection(true);
		}
	}
	return true;
}

void FMeshEditorEditorMode::ActorSelectionChangeNotify()
{
	UpdateSelection();
}

void UMeshGeoData::EraseSelection()
{
	SelectedActors.Empty();
}

void UMeshGeoData::SaveStatus()
{
	for (TWeakObjectPtr<AActor> SelectedActor : SelectedActors)
	{
		if (SelectedActor.Get())
		{
			SelectedActor->Modify();
		}
	}
	Modify();
}

bool FMeshEditorEditorMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	DragTransaction.Begin(LOCTEXT("MeshDragTransaction", "Mesh drag transaction"));
	return true;
}

bool FMeshEditorEditorMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	DragTransaction.End();
	return true;
}


void FMeshEditorEditorMode::UpdateSelection()
{
	TArray<AActor*> NewSelectedActors;
	USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
	CurrentEditorSelection->GetSelectedObjects<AActor>(NewSelectedActors);

	//	Select none
	if (NewSelectedActors.Num() == 0 && CurrentMeshData != nullptr)
	{
		CurrentMeshData->EraseSelection();
	}

	//	Selection not changed
	if (NewSelectedActors == CurrentMeshData->SelectedActors)
	{
	}

	for (AActor* SelectedActor : CurrentMeshData->SelectedActors)
	{
		//	Actor deselected
		if (!NewSelectedActors.Contains(SelectedActor))
		{
			// Reset actor pivot
			if (IsValid(SelectedActor))
			{
				SelectedActor->SetPivotOffset(FVector::ZeroVector);
				GUnrealEd->UpdatePivotLocationForSelection(true);
			}
		}
	}

	// TODO remove
	/*
	for (AActor* NewSelectedActor : NewSelectedActors)
	{
		if (!SelectedActors.Contains(NewSelectedActor))
		{
			//	Added new actor
			if (IsSelectedVertexValid())
			{
				SetTemporaryPivotPoint(NewSelectedActor, SelectedVertex);
			}
		}
	}
	*/

	CurrentMeshData->SelectedActors = NewSelectedActors;
}

bool FMeshEditorEditorMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
                                     EInputEvent Event)
{
	const int32 HitX = Viewport->GetMouseX();
	const int32 HitY = Viewport->GetMouseY();

	HHitProxy* HitProxy = Viewport->GetHitProxy(HitX, HitY);
	if (HitProxy != nullptr && HitProxy->IsA(HAxisDraggerProxy::StaticGetType()))
	{
		HAxisDraggerProxy* AWProxy = static_cast<HAxisDraggerProxy*>(HitProxy);

		if (Event == IE_Pressed)
		{
			UE_LOG(LogTemp, Warning, TEXT("MeshEditorEditorMode - IE_Pressed() "));
			AxisDragger->SetCurrentAxis(AWProxy->Axis, AWProxy->bFlipped);
			AxisDragger->ResetInitialTranslationOffset();
			ViewportClient->SetCurrentWidgetAxis(EAxisList::Type::None);
			// GEditor->BeginTransaction(LOCTEXT("TransformMesh", "Transform Mesh"));
			// CurrentMeshData->SaveStatus();
			// CurrentMeshData->Modify();
			// GEditor->EndTransaction();
		}
		else if (Event == IE_Released)
		{
			UE_LOG(LogTemp, Warning, TEXT("MeshEditorEditorMode - IE_Released() "));
			AxisDragger->SetCurrentAxis(EAxisList::None);
			// GEditor->BeginTransaction(LOCTEXT("TransformMesh", "Transform Mesh"));
			// CurrentMeshData->SaveStatus();
			// CurrentMeshData->Modify();
			// GEditor->EndTransaction();
		}
	}
	return false;
}

bool FMeshEditorEditorMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag,
                                       FRotator& InRot, FVector& InScale)
{
	if (HandleAxisWidgetDelta(InViewportClient, InDrag, InRot, InScale))
	{
		return true;
	}
	return false;
}

void FMeshEditorEditorMode::EraseDroppingPreview()
{
	bPreviousDroppingPreview = false;
}


void FMeshEditorEditorMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);

	const bool bCurrentDroppingPreview{FLevelEditorViewportClient::GetDropPreviewActors().Num() > 0};

	if (bPreviousDroppingPreview && !bCurrentDroppingPreview)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateRaw(this, &FMeshEditorEditorMode::EraseDroppingPreview));
	}
	bPreviousDroppingPreview = bCurrentDroppingPreview;
}

FVector FMeshEditorEditorMode::GetWidgetLocation() const
{
	if (CurrentMeshData != nullptr && CurrentMeshData->SelectedActors.Num() > 0 && CurrentMeshData->
		SelectedLocation != FVector::ZeroVector)
	{
		if (AxisDragger->GetCurrentAxisType() != EAxisList::None)
		{
			return CurrentMeshData->SelectedLocation;
		}
	}
	return FEdMode::GetWidgetLocation();
}


void FMeshEditorEditorMode::CollectPressedKeysData(const FViewport* InViewport)
{
	// TODO remove
	// bIsCtrlKeyDown = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);
	// bIsShiftKeyDown = InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift);
	// bIsAltKeyDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);
	bIsLeftMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);

	// bool bLastSKeyDown = bIsSKeyDown;
	// bIsSKeyDown = InViewport->KeyState(EKeys::S);

	// if (bIsSKeyDown)
	// {
	// 	if (!bLastSKeyDown)
	// 	{
	// 		LastCoordSystemMode = GetModeManager()->GetCoordSystem();
	// 	}
	// 	GetModeManager()->SetCoordSystem(COORD_Local);
	// }
	// else
	// {
	// 	if (bLastSKeyDown)
	// 	{
	// 		GetModeManager()->SetCoordSystem(LastCoordSystemMode);
	// 	}
	// }
}

void FMeshEditorEditorMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	if (bPreviousDroppingPreview)
	{
		return;
	}

	CollectPressedKeysData(Viewport);

	EdModeView = View;

	const auto EditorViewportClient = static_cast<FEditorViewportClient*>(Viewport->GetClient());
	if (EditorViewportClient)
	{
		// Draw bracket box for selected actors
		for (int k = CurrentMeshData->SelectedActors.Num() - 1; k >= 0; k --)
		{
			AActor* SelectedActor = CurrentMeshData->SelectedActors[k];
			TWeakObjectPtr<AActor> TmpActor = SelectedActor;
			if (TmpActor.Get())
			{
				if (TmpActor->IsA(AStaticMeshActor::StaticClass()))
				{
					TWeakObjectPtr<AStaticMeshActor> MeshActorPtr = Cast<AStaticMeshActor>(SelectedActor);
					DrawBoxDraggerForStaticMeshActor(PDI, View, Viewport, MeshActorPtr);
					break;
				}
			}
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, "This message must be of a certain length", true,
	                                 FVector2D::ZeroVector);
}

void FMeshEditorEditorMode::DrawBoxDraggerForStaticMeshActor(FPrimitiveDrawInterface* PDI, const FSceneView* View,
                                                             FViewport* Viewport,
                                                             TWeakObjectPtr<AStaticMeshActor> MeshActorPtr)
{
	if (!MeshActorPtr.Get())
	{
		return;
	}

	if (MeshActorPtr->GetWorld() != PDI->View->Family->Scene->GetWorld())
	{
		return;
	}

	uint64 HiddenClients = MeshActorPtr->HiddenEditorViews;
	bool bActorHiddenForViewport = false;

	if (!MeshActorPtr->IsHiddenEd())
	{
		if (Viewport)
		{
			for (int32 ViewIndex = 0; ViewIndex < GEditor->GetLevelViewportClients().Num(); ++ViewIndex)
			{
				// If the current viewport is hiding this actor, don't draw brackets around it
				if (Viewport->GetClient() == GEditor->GetLevelViewportClients()[ViewIndex] && HiddenClients & ((uint64)1
					<< ViewIndex))
				{
					bActorHiddenForViewport = true;
					break;
				}
			}
		}

		if (!bActorHiddenForViewport)
		{
			AxisDragger->SetMeshActor(MeshActorPtr);
			MeshActorPtr->ForEachComponent<UStaticMeshComponent>(false, [&](const UStaticMeshComponent* InPrimComp)
			{
				TArray<FVector> MeshCorners;
				DrawBracketForMeshComp(PDI, InPrimComp, MeshCorners);
				DrawDraggerForMeshComp(PDI, View, Viewport, InPrimComp, MeshCorners);
			});
		}
	}
}

void FMeshEditorEditorMode::DrawBracketForMeshComp(FPrimitiveDrawInterface* PDI, const UStaticMeshComponent* InMeshComp,
                                                   TArray<FVector>& OutVerts)
{
	if (InMeshComp == nullptr)
	{
		return;
	}

	const FLinearColor GROUP_COLOR = {0.0f, 1.0f, 0.0f};
	FBox LocalBox(ForceInit);

	// Only use collidable components to find collision bounding box.
	if (InMeshComp->IsRegistered() && (true || InMeshComp->IsCollisionEnabled()))
	{
		LocalBox = InMeshComp->CalcBounds(FTransform::Identity).GetBox();
	}

	FVector MinVector, MaxVector;
	MinVector = FVector(BIG_NUMBER);
	MaxVector = FVector(-BIG_NUMBER);

	// MinVector
	MinVector.X = FMath::Min<float>(LocalBox.Min.X, MinVector.X);
	MinVector.Y = FMath::Min<float>(LocalBox.Min.Y, MinVector.Y);
	MinVector.Z = FMath::Min<float>(LocalBox.Min.Z, MinVector.Z);
	// MaxVector
	MaxVector.X = FMath::Max<float>(LocalBox.Max.X, MaxVector.X);
	MaxVector.Y = FMath::Max<float>(LocalBox.Max.Y, MaxVector.Y);
	MaxVector.Z = FMath::Max<float>(LocalBox.Max.Z, MaxVector.Z);

	// Calculate bracket corners based on min/max vectors
	TArray<FVector> BracketCorners;
	const FTransform& CompTransform = InMeshComp->GetComponentTransform();

	// Bottom Corners
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MinVector.X, MinVector.Y, MinVector.Z)));
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MinVector.X, MaxVector.Y, MinVector.Z)));
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MaxVector.X, MaxVector.Y, MinVector.Z)));
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MaxVector.X, MinVector.Y, MinVector.Z)));

	// Top Corners
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MinVector.X, MinVector.Y, MaxVector.Z)));
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MinVector.X, MaxVector.Y, MaxVector.Z)));
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MaxVector.X, MaxVector.Y, MaxVector.Z)));
	BracketCorners.Add(CompTransform.TransformPosition(FVector(MaxVector.X, MinVector.Y, MaxVector.Z)));

	FVector GlobalAxisX = CompTransform.TransformVectorNoScale(FVector{1.0f, 0.0f, 0.0f});
	FVector GlobalAxisY = CompTransform.TransformVectorNoScale(FVector{0.0f, 1.0f, 0.0f});
	FVector GlobalAxisZ = CompTransform.TransformVectorNoScale(FVector{0.0f, 0.0f, 1.0f});

	const float BracketOffsetFactor = 0.2;
	float BracketOffset;
	float BracketPadding;
	{
		const float DeltaX = (BracketCorners[3] - BracketCorners[0]).Length();
		const float DeltaY = (BracketCorners[1] - BracketCorners[0]).Length();
		const float DeltaZ = (BracketCorners[4] - BracketCorners[0]).Length();
		BracketOffset = FMath::Min(FMath::Min(DeltaX, DeltaY), DeltaZ) * BracketOffsetFactor;
		BracketPadding = BracketOffset * 0.08;
	}

	const int32 DIR_X[] = {1, 1, -1, -1, 1, 1, -1, -1};
	const int32 DIR_Y[] = {1, -1, -1, 1, 1, -1, -1, 1};
	const int32 DIR_Z[] = {1, 1, 1, 1, -1, -1, -1, -1};

	for (int32 BracketCornerIndex = 0; BracketCornerIndex < BracketCorners.Num(); ++BracketCornerIndex)
	{
		// Direction corner axis should be pointing based on min/max
		const FVector CORNER = BracketCorners[BracketCornerIndex]
			- (BracketPadding * DIR_X[BracketCornerIndex] * GlobalAxisX)
			- (BracketPadding * DIR_Y[BracketCornerIndex] * GlobalAxisY)
			- (BracketPadding * DIR_Z[BracketCornerIndex] * GlobalAxisZ);

		PDI->DrawLine(CORNER, CORNER + (BracketOffset * DIR_X[BracketCornerIndex] * GlobalAxisX), GROUP_COLOR,
		              SDPG_Foreground);
		PDI->DrawLine(CORNER, CORNER + (BracketOffset * DIR_Y[BracketCornerIndex] * GlobalAxisY), GROUP_COLOR,
		              SDPG_Foreground);
		PDI->DrawLine(CORNER, CORNER + (BracketOffset * DIR_Z[BracketCornerIndex] * GlobalAxisZ), GROUP_COLOR,
		              SDPG_Foreground);
	}

	for (int32 BracketCornerIndex = 0; BracketCornerIndex < BracketCorners.Num(); ++BracketCornerIndex)
	{
		OutVerts.Add(BracketCorners[BracketCornerIndex]);
	}
}

void FMeshEditorEditorMode::DrawDraggerForMeshComp(FPrimitiveDrawInterface* PDI, const FSceneView* View,
                                                   FViewport* Viewport, const UStaticMeshComponent* InMeshComp,
                                                   const TArray<FVector>& InCorners)
{
	check(InCorners.Num() == 8);

	TArray<FVector> AxisBases;
	// Bottom
	AxisBases.Add((InCorners[0] + InCorners[1] + InCorners[2] + InCorners[3]) / 4.0f);
	// Top
	AxisBases.Add((InCorners[4] + InCorners[5] + InCorners[6] + InCorners[7]) / 4.0f);
	// Left
	AxisBases.Add((InCorners[0] + InCorners[1] + InCorners[5] + InCorners[4]) / 4.0f);
	// Right
	AxisBases.Add((InCorners[3] + InCorners[2] + InCorners[6] + InCorners[7]) / 4.0f);
	// Back
	AxisBases.Add((InCorners[0] + InCorners[3] + InCorners[4] + InCorners[7]) / 4.0f);
	// Front
	AxisBases.Add((InCorners[1] + InCorners[2] + InCorners[5] + InCorners[6]) / 4.0f);

	TArray<EAxisList::Type> AxisDir = {
		EAxisList::Type::Z, EAxisList::Type::Z,
		EAxisList::Type::X, EAxisList::Type::X,
		EAxisList::Type::Y, EAxisList::Type::Y
	};

	TArray<bool> AxisFlip = {
		true, false,
		true, false,
		true, false
	};

	if (!bIsLeftMouseButtonDown)
	{
		AxisDragger->SetCurrentAxis(EAxisList::None);
	}
	else
	{
		AxisDragger->SetAxisBaseVerts(AxisBases);
		AxisDragger->SetTransform(InMeshComp->GetComponentTransform());
	}

	FTransform CompTransform = InMeshComp->GetComponentTransform();
	for (int32 BaseIndex = 0; BaseIndex < AxisBases.Num(); BaseIndex ++)
	{
		AxisDragger->RenderAxis(PDI, View, CompTransform, AxisBases[BaseIndex], AxisDir[BaseIndex], AxisFlip[BaseIndex],
		                        true);
	}
}

#undef LOCTEXT_NAMESPACE
