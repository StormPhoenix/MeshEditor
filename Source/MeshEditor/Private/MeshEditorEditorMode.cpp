// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshEditorEditorMode.h"
#include "EdModeInteractiveToolsContext.h"
#include "MeshEditorSettings.h"
#include "UnrealEd.h"
#include "Algo/Copy.h"
#include "Helper/DrawHelper.h"
#include "Dragger/AxisDragger.h"
#include "Helper/MeshDataIterators.h"
#include "Tools/MeshEditorSimpleTool.h"

#define USING_PREFAB_PLUGIN
#ifdef USING_PREFAB_PLUGIN
#include "Prefab/PrefabActor.h"
#endif

// #define HANDLE_DRAW_EDGE

#define LOCTEXT_NAMESPACE "MeshEditorEditorMode"

struct HMeshEdgeProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()

	HMeshEdgeProxy(FVector InPosition, AActor* InActor)
		: HHitProxy(HPP_UI), RefActor(InActor), RefVector(InPosition)
	{
	}

	HMeshEdgeProxy(FVector InFirstEndpoint, FVector InSecondEndpoint, AActor* InActor)
		: HHitProxy(HPP_UI), RefActor(InActor), FirstRefVector(InFirstEndpoint),
		  SecondRefVector(InSecondEndpoint)
	{
	}

	AActor* RefActor;
	FVector RefVector;

	// edge info
	FVector FirstRefVector;
	FVector SecondRefVector;
};

IMPLEMENT_HIT_PROXY(HMeshEdgeProxy, HHitProxy);

const FEditorModeID FMeshEditorEditorMode::EM_MeshEditorEditorModeId = TEXT("EM_MeshEditorEditorMode");

FMeshEditorEditorMode::FMeshEditorEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(EM_MeshEditorEditorModeId,
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

	AxisDragger = new FAxisDragger();
	if (!OnCollectingDataFinished.IsBoundToObject(this))
	{
		OnCollectingDataFinished.BindRaw(this, &FMeshEditorEditorMode::CollectingMeshDataFinished);
	}

	GetWorld()->GetTimerManager().SetTimer(CollectVerticesTimerHandle,
	                                       FTimerDelegate::CreateRaw(
		                                       this, &FMeshEditorEditorMode::AsyncCollectMeshData), 0.2f,
	                                       false);
	GetWorld()->GetTimerManager().SetTimer(InvalidateHitProxiesTimerHandle,
	                                       FTimerDelegate::CreateRaw(
		                                       this, &FMeshEditorEditorMode::InvalidateHitProxies), 0.3f, true);
	UpdateInitialSelection();

	RefreshPrefabs();
}


void FMeshEditorEditorMode::Exit()
{
	bIsModeOn = false;

	if (CollectVerticesTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CollectVerticesTimerHandle);
	}

	if (InvalidateHitProxiesTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(InvalidateHitProxiesTimerHandle);
	}

	CurrentMeshData->EraseSelection();
	delete AxisDragger;

	FEdMode::Exit();
}

FVector BlendPositions(FVector Pos1, FVector Pos2, float factor = 1.0)
{
	return Pos1;
}

bool FMeshEditorEditorMode::HandleEdgeClickEvent(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                                 const FViewportClick& Click)
{
#ifdef WITH_EDITOR
	if (HitProxy)
	{
		const bool bIsLeftMouseButtonClick = (Click.GetKey() == EKeys::LeftMouseButton);
		if (bIsLeftMouseButtonClick && HitProxy->IsA(HMeshEdgeProxy::StaticGetType()))
		{
			const auto MeshEdgeHitProxy = static_cast<HMeshEdgeProxy*>(HitProxy);
			FVector SelectedVertex = BlendPositions(MeshEdgeHitProxy->FirstRefVector,
			                                        MeshEdgeHitProxy->SecondRefVector);

			TArray<AActor*> SelectedActors;
			USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
			CurrentEditorSelection->GetSelectedObjects<AActor>(SelectedActors);

			const FTransform NewPivotTransform = FTransform(SelectedVertex);
			for (AActor* Actor : SelectedActors)
			{
				Actor->SetPivotOffset(NewPivotTransform.GetRelativeTransform(Actor->GetActorTransform()).GetLocation());
				GUnrealEd->UpdatePivotLocationForSelection(true);
			}
			return true;
		}
	}
#endif
	return false;
}

FBox GetPrefabBounds(APrefabActor* PrefabPtr)
{
	FBox RetBox(EForceInit::ForceInit);
	if (PrefabPtr == nullptr)
	{
		return RetBox;
	}

	TArray<AActor*> AttachedActors;
	PrefabPtr->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AttachedActor->IsA(AStaticMeshActor::StaticClass()))
		{
			AttachedActor->ForEachComponent<UStaticMeshComponent>(false, [&](const UStaticMeshComponent* InComp)
			{
				if (InComp == nullptr)
				{
					return;
				}

				RetBox += InComp->Bounds.GetBox();
			});
		}
		else if (AttachedActor->IsA(APrefabActor::StaticClass()))
		{
			RetBox += GetPrefabBounds(Cast<APrefabActor>(AttachedActor));
		}
	}
	return RetBox;
}

bool IsBoxContainsPoint(const FBox& Box, const FVector& Point)
{
	FVector Min = Box.Min;
	FVector Max = Box.Max;

	if ((Point[0] > Min[0] &&
			Point[1] > Min[1] &&
			Point[2] > Min[2])
		&& (Point[0] < Max[0] &&
			Point[1] < Max[1] &&
			Point[2] < Max[2]))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool FMeshEditorEditorMode::HandlePrefabClickEvent(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                                   const FViewportClick& Click)
{
	bool bPrefabHandle = false;
#ifdef USING_PREFAB_PLUGIN
#ifdef WITH_EDITOR
	{
		if (HitProxy != nullptr && HitProxy->IsA(HActor::StaticGetType()))
		{
			HActor* ActorHitProxy = static_cast<HActor*>(HitProxy);
			AActor* ConsideredActor = ActorHitProxy->Actor;

			if (ConsideredActor != nullptr)
			{
				APrefabActor* FatherPrefabPtr = nullptr;
				AActor* SonActorPtr = ConsideredActor;

				// 首先检查当前选中的 Actor 属于哪一个 PrefabActor
				for (TWeakObjectPtr<APrefabActor> PrefabPtr : APrefabActor::PrefabContainer)
				{
					if (PrefabPtr.Get() && SonActorPtr->IsAttachedTo(PrefabPtr.Get()))
					{
						FatherPrefabPtr = PrefabPtr.Get();
					}
				}

				if (FatherPrefabPtr != nullptr && SonActorPtr != nullptr)
				{
					if (Click.GetEvent() == IE_DoubleClick)
					{
						// 处理双击全选事件
						if (!bIsCtrlKeyDown)
						{
							// 取消选中其他 Actor
							USelection* Selection = GEditor->GetSelectedActors();
							TArray<AActor*> SelectedActors;
							Selection->GetSelectedObjects(SelectedActors);
							for (AActor* SelectedActor : SelectedActors)
							{
								GEditor->SelectActor(SelectedActor, false, true);
							}

							// 取消选中其他 PrefabActor
							for (TWeakObjectPtr<APrefabActor> OtherPrefabPtr : APrefabActor::PrefabContainer)
							{
								if (OtherPrefabPtr.Get() && OtherPrefabPtr.Get() != FatherPrefabPtr)
								{
									OtherPrefabPtr->DeselectSubActors();
									OtherPrefabPtr->DeselectSelf();
								}
							}
						}
						FatherPrefabPtr->SelectSelf();
						bPrefabHandle = true;
					}
					else
					{
						// 处理单击事件
						if (bIsCtrlKeyDown)
						{
							FatherPrefabPtr->DeselectSelf();
							bool bInSelected = !SonActorPtr->IsSelected();
							GEditor->SelectActor(SonActorPtr, bInSelected, true, true, true);
							bPrefabHandle = true;
						}
						else
						{
							// Just single-one click
							TArray<APrefabActor*> SelectedPrefabs;
							TArray<AStaticMeshActor*> SelectedMeshes;
							USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
							CurrentEditorSelection->GetSelectedObjects<APrefabActor>(SelectedPrefabs);
							CurrentEditorSelection->GetSelectedObjects<AStaticMeshActor>(SelectedMeshes);

							// 选中所有子组件功能
							if ((SelectedPrefabs.Num() == 1) && (SelectedMeshes.Num() == 0) &&
								(SelectedPrefabs[0] == FatherPrefabPtr))
							{
								FatherPrefabPtr->SelectAllAttachedActors();
								bPrefabHandle = true;
							}
							else
							{
								bPrefabHandle = false;
							}
							FatherPrefabPtr->DeselectSelf();
							// GEditor->SelectActor(FatherPrefabPtr, false, true, false, true);
						}
					}
				}
			}
		}
	}
#endif
#endif
	return bPrefabHandle;
}


bool FMeshEditorEditorMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                        const FViewportClick& Click)
{
	bool bEdgeClickHandle = false;
	bool bPrefabClickHandle = false;

#ifdef HANDLE_DRAW_EDGE
	bEdgeClickHandle = HandleEdgeClickEvent(InViewportClient, HitProxy, Click);
#endif

	if (!bEdgeClickHandle)
	{
		bPrefabClickHandle = HandlePrefabClickEvent(InViewportClient, HitProxy, Click);
	}
	return bEdgeClickHandle || bPrefabClickHandle;
}

// TODO Move to helpers
void ParentActors(AActor* ParentActor, AActor* ChildActor)
{
	if (!ChildActor || !ParentActor)
	{
		return;
	}
	USceneComponent* ChildRoot = ChildActor->GetRootComponent();
	USceneComponent* ParentRoot = ParentActor->GetDefaultAttachComponent();

	if (!ChildRoot)
	{
		return;
	}

	if (!ParentRoot)
	{
		return;
	}

	check(ChildRoot); // CanParentActors() call should ensure this
	check(ParentRoot); // CanParentActors() call should ensure this

	ChildActor->Modify();
	ParentActor->Modify();

	// If child is already attached to something, modify the old parent and detach
	if (ChildRoot->GetAttachParent() != nullptr)
	{
		AActor* OldParentActor = ChildRoot->GetAttachParent()->GetOwner();
		OldParentActor->Modify();
		ChildRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	// If the parent is already attached to this child, modify its parent and detach so we can allow the attachment
	if (ParentRoot->IsAttachedTo(ChildRoot))
	{
		ParentRoot->GetAttachParent()->GetOwner()->Modify();
		ParentRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	// Snap to socket if a valid socket name was provided, otherwise attach without changing the relative transform
	ChildRoot->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepWorldTransform, NAME_None);
}

bool FMeshEditorEditorMode::HandleAttachMovement(FEditorViewportClient* InViewportClient, const FVector& InDrag,
                                                 const FRotator& InRot, const FVector& InScale)
{
	if (InViewportClient == nullptr)
	{
		return false;
	}

	auto World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	if (IsInAttachMovementMode())
	{
		TArray<AStaticMeshActor*> SelectedMesh{};
		USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
		CurrentEditorSelection->GetSelectedObjects<AStaticMeshActor>(SelectedMesh);

		if (SelectedMesh.Num() > 0)
		{
			// 首先从原有 PrefabActor (如果存在的话) 上 detch
			for (AStaticMeshActor* MeshActor : SelectedMesh)
			{
				MeshActor->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
			}

			TWeakObjectPtr<APrefabActor> TargetPrefab;
			bool bIsContained = false;
			for (auto& PrefabStateElem : PrefabStateMap)
			{
				TWeakObjectPtr<APrefabActor> Prefab = PrefabStateElem.Key;
				PrefabState& StateInfo = PrefabStateElem.Value;

				if (!Prefab.Get())
				{
					continue;
				}

				FBox Bound = GetPrefabBounds(Prefab.Get());
				for (AStaticMeshActor* Mesh : SelectedMesh)
				{
					bool Ret = IsBoxContainsPoint(Bound, Mesh->GetActorLocation());
					if (Ret)
					{
						bIsContained = true;
						TargetPrefab = Prefab;
						break;
					}
				}

				if (bIsContained)
				{
					break;
				}
			}

			if (bIsContained && TargetPrefab.Get())
			{
				// 其他 Prefab 的 Attach 状态全部清空未 false
				for (auto& PrefabElem : PrefabStateMap)
				{
					TWeakObjectPtr<APrefabActor> OtherPrefab = PrefabElem.Key;
					PrefabState& State = PrefabElem.Value;
					if (!OtherPrefab.Get())
					{
						continue;
					}

					if (TargetPrefab.Get() != OtherPrefab)
					{
						State.bIsContainAttachment = false;
					}
				}

				if (!PrefabStateMap.Contains(TargetPrefab))
				{
					PrefabStateMap[TargetPrefab] = PrefabState(false);
				}
				PrefabStateMap[TargetPrefab].bIsContainAttachment = true;
			}
			else
			{
				RefreshPrefabStatus();
			}
		}
	}
	return false;
}

bool FMeshEditorEditorMode::HandleAxisWidgetDelta(FEditorViewportClient* InViewportClient, const FVector& NotUsed0,
                                                  const FRotator& NotUsed1, const FVector& NotUsed2)
{
	if (AxisDragger->GetCurrentAxisType() == EAxisList::None)
	{
		return false;
	}

	if (!bIsTracking)
	{
		bIsTracking = true;
		DragTransaction.Begin(LOCTEXT("MeshDragTransaction", "Mesh drag transaction"));
	}

	if (AxisDragger->DoWork(InViewportClient, EdModeView))
	{
		GUnrealEd->UpdatePivotLocationForSelection(true);
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
	// DragTransaction.Begin(LOCTEXT("MeshDragTransaction", "Mesh drag transaction"));
	if (AxisDragger != nullptr && AxisDragger->GetCurrentAxisType() != EAxisList::Type::None)
	{
		return true;
	}
	return false;
}

bool FMeshEditorEditorMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (bIsTracking)
	{
		DragTransaction.End();
		bIsTracking = false;
		return true;
	}
	return false;
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
	UE_LOG(LogTemp, Display, TEXT("InputKey. "));

	// TODO 检查 Key 类型

	const int32 HitX = Viewport->GetMouseX();
	const int32 HitY = Viewport->GetMouseY();
	HHitProxy* HitProxy = Viewport->GetHitProxy(HitX, HitY);

	if (Event == IE_Pressed && Key.GetFName() == FName("T") && AxisDragger != nullptr)
	{
		AxisDragger->SwitchMode(); // 修改为 T; 改一下 handle 的样式
	}

	if (HitProxy != nullptr && HitProxy->IsA(HAxisDraggerProxy::StaticGetType()))
	{
		HAxisDraggerProxy* AWProxy = static_cast<HAxisDraggerProxy*>(HitProxy);

		if (Event == IE_Pressed)
		{
			UE_LOG(LogTemp, Display, TEXT("InputKey - IE_Pressed. "));
			AxisDragger->SetCurrentAxisType(AWProxy->Axis, AWProxy->bFlipped);
			AxisDragger->ResetInitialTranslationOffset();
			ViewportClient->SetCurrentWidgetAxis(EAxisList::Type::None);
		}
		else if (Event == IE_Released)
		{
			UE_LOG(LogTemp, Display, TEXT("InputKey - IE_Released. "));
			AxisDragger->SetCurrentAxisType(EAxisList::None);
		}
	}

	if (Event == IE_Released)
	{
		// 处理 AttachMode
		if (IsInAttachMovementMode())
		{
			TArray<AStaticMeshActor*> SelectedMeshes;
			USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
			CurrentEditorSelection->GetSelectedObjects<AStaticMeshActor>(SelectedMeshes);

			if (SelectedMeshes.Num() > 0)
			{
				for (auto& PrefabStateElem : PrefabStateMap)
				{
					TWeakObjectPtr<APrefabActor> Prefab = PrefabStateElem.Key;
					PrefabState& StateInfo = PrefabStateElem.Value;

					if (!Prefab.Get() || !StateInfo.bIsContainAttachment)
					{
						continue;
					}

					for (AStaticMeshActor* Mesh : SelectedMeshes)
					{
						ParentActors(Prefab.Get(), Mesh);
					}
					break;
				}
			}
		}
		RefreshPrefabStatus();
	}

	// TODO 没有利用到 UE 的框架，所以每次输入按键时间都要刷新 Prefab 保存状态
	RefreshPrefabs();
	return false;
}

bool FMeshEditorEditorMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag,
                                       FRotator& InRot, FVector& InScale)
{
	// FVector2D MousePosition(InViewportClient->Viewport->GetMouseX(), InViewportClient->Viewport->GetMouseY());
	// FIntPoint ViewportSize = InViewportClient->Viewport->GetSizeXY();
	// UE_LOG(LogTemp, Display, TEXT("MousePosition, X: %f, Y: %f, WindowSize, SizeX: %f, SizeY: %f"),
	// MousePosition[0], MousePosition[1], ViewportSize[0], ViewportSize[1]);

	bool bHandleAxisDragger = false;
	bool bHandleAttachMovement = false;

	bHandleAxisDragger = HandleAxisWidgetDelta(InViewportClient, InDrag, InRot, InScale);
	if (!bHandleAxisDragger)
	{
		bHandleAttachMovement = HandleAttachMovement(InViewportClient, InDrag, InRot, InScale);
	}
	return bHandleAttachMovement || bHandleAxisDragger;
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
	if (bIsTracking && AxisDragger != nullptr && AxisDragger->GetCurrentAxisType() != EAxisList::None)
	{
		return AxisDragger->GetExpectedPivot();
	}

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

FVector2D FMeshEditorEditorMode::GetMouseVector2D()
{
	return FVector2D{
		GEditor->GetActiveViewport()->GetMouseX() / DPIScale, GEditor->GetActiveViewport()->GetMouseY() / DPIScale
	};
}

void FMeshEditorEditorMode::CollectCursorData(const FSceneView* InSceneView)
{
	bIsMouseMove = GetMouseVector2D() != MouseOnScreenPosition;
	MouseOnScreenPosition = GetMouseVector2D();

	FVector MouseWorldPosition;
	FVector CameraDirection;
	InSceneView->DeprojectFVector2D(MouseOnScreenPosition, MouseWorldPosition, CameraDirection);

	FCollisionQueryParams TraceQueryParams;
	TraceQueryParams.bTraceComplex = true;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, MouseWorldPosition,
	                                     MouseWorldPosition + (CameraDirection * 10000000), ECC_Visibility,
	                                     TraceQueryParams);

	// MouseInWorld = HitResult.ImpactPoint;
}

void FMeshEditorEditorMode::CollectPressedKeysData(const FViewport* InViewport)
{
	// TODO remove
	bIsCtrlKeyDown = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);
	bIsBKeyDown = InViewport->KeyState(EKeys::B);
	// bIsShiftKeyDown = InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift);
	// bIsAltKeyDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);
	bIsLeftMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);
}

void FMeshEditorEditorMode::DrawBracketForPrefab(FPrimitiveDrawInterface* PDI, const FViewport* Viewport,
                                                 TWeakObjectPtr<APrefabActor> Prefab, PrefabState& State)
{
	if (!Prefab.Get())
	{
		return;
	}

	TArray<FVector> Corners;
	FBox ActorBounds = GetPrefabBounds(Prefab.Get());

	FVector Min = ActorBounds.Min;
	FVector Max = ActorBounds.Max;

	Corners.Add(FVector(Min.X, Min.Y, Min.Z));
	Corners.Add(FVector(Min.X, Max.Y, Min.Z));
	Corners.Add(FVector(Max.X, Max.Y, Min.Z));
	Corners.Add(FVector(Max.X, Min.Y, Min.Z));

	Corners.Add(FVector(Min.X, Min.Y, Max.Z));
	Corners.Add(FVector(Min.X, Max.Y, Max.Z));
	Corners.Add(FVector(Max.X, Max.Y, Max.Z));
	Corners.Add(FVector(Max.X, Min.Y, Max.Z));

	TArray<FVector> GlobalAxis;
	GlobalAxis.Add(FVector(1.0f, 0.0f, 0.0f));
	GlobalAxis.Add(FVector(0.0f, 1.0f, 0.0f));
	GlobalAxis.Add(FVector(0.0f, 0.0f, 1.0f));

	// TODO move to settings
	const FLinearColor PREFAB_BRACKET_COLOR_IN_ATTACH_MODE = {0.0f, 1.0f, 0.0f};
	const FLinearColor PREFAB_BRACKET_SELECTED_COLOR_IN_ATTACH_MODE = FColor::Yellow;

	const float Thickness = 10.0f;

	FLinearColor DrawColor = State.bIsContainAttachment
		                         ? PREFAB_BRACKET_SELECTED_COLOR_IN_ATTACH_MODE
		                         : PREFAB_BRACKET_COLOR_IN_ATTACH_MODE;
	DrawHelper::DrawBrackets(PDI, DrawColor, Thickness, Corners, GlobalAxis);
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
		const UMeshEditorSettings* Settings{UMeshEditorSettings::Get()};
		const bool bIsPerspectiveView{EditorViewportClient->IsPerspective()};
		const FVector EditorCameraLocation = EditorViewportClient->GetViewLocation();

#ifdef HANDLE_DRAW_EDGE
		// Draw edges
		for (int i = 0; i < LastCapturedEdgeData.Num(); i ++)
		{
			{
				const FMeshEdgeData& EdgeData{LastCapturedEdgeData[i]};

				FVector FirstEndpointLocation{EdgeData.FirstEndpointInWorldPosition};
				FVector SecondEndpointLocation{EdgeData.SecondEndpointInWorldPosition};

				if (bIsPerspectiveView)
				{
					//	Draw the sprite with a slight offset towards the camera to avoid gaps in the geometry
					FirstEndpointLocation += (EditorCameraLocation - EdgeData.FirstEndpointInWorldPosition).
						GetSafeNormal() * 3;
					SecondEndpointLocation += (EditorCameraLocation - EdgeData.SecondEndpointInWorldPosition).
						GetSafeNormal() * 3;
				}

				HMeshEdgeProxy* HitResult = new HMeshEdgeProxy(
					EdgeData.FirstEndpointInWorldPosition, EdgeData.SecondEndpointInWorldPosition,
					EdgeData.EdgeOwnerActor);

				PDI->SetHitProxy(HitResult);
				PDI->DrawLine(FirstEndpointLocation, SecondEndpointLocation, Settings->MeshEdgeColor,
				              SDPG_World, Settings->MeshEdgeThickness);
			}
		}
#endif

#ifdef USING_PREFAB_PLUGIN
		if (IsInAttachMovementMode())
		{
			// Draw bracket box for prefab if in attachment mode
			for (auto& PrefabStateElem : PrefabStateMap)
			{
				TWeakObjectPtr<APrefabActor> Prefab = PrefabStateElem.Key;
				PrefabState& State = PrefabStateElem.Value;
				if (!Prefab.Get())
				{
					continue;
				}

				DrawBracketForPrefab(PDI, Viewport, Prefab, State);
			}
		}
#endif

		// Draw bracket box for selected actors
		TArray<TWeakObjectPtr<AStaticMeshActor>> ActorsToBeDraw;
		for (int k = 0; k < CurrentMeshData->SelectedActors.Num(); k ++)
		{
			AActor* SelectedActor = CurrentMeshData->SelectedActors[k];
			TWeakObjectPtr<AActor> TmpActor = SelectedActor;
			if (TmpActor.Get())
			{
				if (TmpActor->IsA(AStaticMeshActor::StaticClass()))
				{
					TWeakObjectPtr<AStaticMeshActor> MeshActorPtr = Cast<AStaticMeshActor>(SelectedActor);
					ActorsToBeDraw.Add(MeshActorPtr);
				}
			}
		}
		DrawBoxDraggerForStaticMeshActors(PDI, View, Viewport, ActorsToBeDraw);
	}
}

void FMeshEditorEditorMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View,
                                    FCanvas* Canvas)
{
	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);

	if (bPreviousDroppingPreview)
	{
		return;
	}

#ifdef USING_PREFAB_PLUGIN
	TArray<APrefabActor*> Prefabs{};
	USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
	CurrentEditorSelection->GetSelectedObjects<APrefabActor>(Prefabs);

	FIntPoint ViewportSize = ViewportClient->Viewport->GetSizeXY();

	FString PrintMsg = "";
	for (int i = 0; i < Prefabs.Num(); i ++)
	{
		APrefabActor* Prefab = Prefabs[i];
		FString ActorName = Prefab->GetActorLabel();
		uint64 FaceCount = Prefab->GetAttachActorTotalFaceCount();
		uint64 SubActorCount = Prefab->GetAttachActorCount();
		FString PrefabInfo = FString::Printf(TEXT("%s, \nTriangles: %d\nComponents: %d"), *ActorName,
		                                     FaceCount, SubActorCount);
		PrintMsg.Append("\n").Append(PrefabInfo);
	}

	FCanvasTextItem DebugMsg(FVector2D(50.0f, 50.0f), FText::FromString(PrintMsg),
	                         GEngine->GetMediumFont(), FLinearColor::Yellow);
	Canvas->DrawItem(DebugMsg, FVector2D(50, 50));
#endif

	DPIScale = Canvas->GetDPIScale();
}

void FMeshEditorEditorMode::DrawBoxDraggerForStaticMeshActors(
	FPrimitiveDrawInterface* PDI, const FSceneView* View, FViewport* Viewport,
	TArray<TWeakObjectPtr<AStaticMeshActor>>& MeshActorArray)
{
	AxisDragger->UpdateControlledMeshGroup(MeshActorArray);

	TArray<FVector> DraggerBracketCorners;
	DrawBracketForDragger(PDI, Viewport, DraggerBracketCorners);
	DrawHandleForDragger(PDI, View, Viewport, DraggerBracketCorners);
}

void FMeshEditorEditorMode::DrawBracketForDragger(FPrimitiveDrawInterface* PDI, FViewport* Viewport,
                                                  TArray<FVector>& OutVerts)
{
	DrawHelper::DrawBracketForDragger(PDI, Viewport, AxisDragger, OutVerts);
}

void ComputeAxisBases(const TArray<FVector>& InCorners, TArray<FVector>& OutAxisBases)
{
	// Bottom
	OutAxisBases.Add((InCorners[0] + InCorners[1] + InCorners[2] + InCorners[3]) / 4.0f);
	// Top
	OutAxisBases.Add((InCorners[4] + InCorners[5] + InCorners[6] + InCorners[7]) / 4.0f);
	// Left
	OutAxisBases.Add((InCorners[0] + InCorners[1] + InCorners[5] + InCorners[4]) / 4.0f);
	// Right
	OutAxisBases.Add((InCorners[3] + InCorners[2] + InCorners[6] + InCorners[7]) / 4.0f);
	// Back
	OutAxisBases.Add((InCorners[0] + InCorners[3] + InCorners[4] + InCorners[7]) / 4.0f);
	// Front
	OutAxisBases.Add((InCorners[1] + InCorners[2] + InCorners[5] + InCorners[6]) / 4.0f);
}

void FMeshEditorEditorMode::DrawHandleForDragger(FPrimitiveDrawInterface* PDI, const FSceneView* View,
                                                 FViewport* Viewport, const TArray<FVector>& InCorners)
{
	if (InCorners.Num() == 0)
	{
		return;
	}

	check(InCorners.Num() == 8);

	TArray<FVector> AxisBases;
	ComputeAxisBases(InCorners, AxisBases);

	if (!bIsLeftMouseButtonDown)
	{
		AxisDragger->SetCurrentAxisType(EAxisList::None);
	}
	AxisDragger->SetAxisBaseVerts(AxisBases);
	AxisDragger->RenderHandler(PDI, View);
}

void FMeshEditorEditorMode::CollectingMeshDataFinished()
{
	LastCapturedEdgeData = CapturedEdgeData;
	bDataCollectionInProgress = false;

	if (bIsModeOn)
	{
		AsyncCollectMeshData();
	}
}

struct DistanceCMP
{
	DistanceCMP(FVector InTarget): Target(InTarget)
	{
	}

	FORCEINLINE bool operator()(const FVector& A, const FVector& B) const
	{
		return FVector::Distance(Target, A) > FVector::Distance(Target, B);
	}

	FVector Target;
};

/*
void ComputeMostKAdjacentVertics(uint32 K, uint32 ComparedIndex, FVector Target,
                                 UPrimitiveComponent* PrimitiveComponent,
                                 TArray<FVector>& OutAdjacentVertices)
{
	TArray<FVector> Heap;
	TSharedPtr<FMeshDataIterators::FVertexIterator> VertexGetter =
		FMeshDataIterators::MakeVertexIterator(PrimitiveComponent);
	if (VertexGetter.IsValid())
	{
		FMeshDataIterators::FVertexIterator& VertexGetterRef = *VertexGetter;
		uint32 i = 0;
		for (; VertexGetterRef; ++VertexGetterRef, i ++)
		{
			if (i == ComparedIndex)
			{
				continue;
			}
			Heap.Emplace(VertexGetterRef.Position());
		}

		uint32 TotalVertex = Heap.Num();
		Heap.Heapify(DistanceCMP(Target));
		for (uint32 j = 0; j < K && j < TotalVertex; j ++)
		{
			FVector V;
			Heap.HeapPop(V, DistanceCMP(Target));
			OutAdjacentVertices.Add(V);
		}
	}
}
*/

void FMeshEditorEditorMode::AsyncCollectMeshData()
{
	if (bDataCollectionInProgress)
	{
		return;
	}

	if (!EdModeView)
	{
		return;
	}

	TWeakPtr<FMeshEditorEditorMode> WeakThisPtr{SharedThis(this)};
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThisPtr]()
	{
		FMeshEditorEditorMode* ThisBackgroundThread{WeakThisPtr.Pin().Get()};
		if (!ThisBackgroundThread)
		{
			return;
		}

		ThisBackgroundThread->bDataCollectionInProgress = true;
		ThisBackgroundThread->CapturedEdgeData.Empty();

		//	Filter visible actors // TODO remove
		auto ActorWasRendered = [](const AActor* InActor)
		{
			return InActor && InActor->WasRecentlyRendered();
		};

		TArray<AStaticMeshActor*> ActorsOnScreen{};
		USelection* CurrentEditorSelection = GEditor->GetSelectedActors();
		CurrentEditorSelection->GetSelectedObjects<AStaticMeshActor>(ActorsOnScreen);

		for (int i = 0; i < ActorsOnScreen.Num(); ++i)
		{
			//	Take one hundred closest actors and collect data on their vertices
			const AActor* CurrentActor = ActorsOnScreen[i];

			// TODO remove
			if (!CurrentActor->IsA(AStaticMeshActor::StaticClass()))
			{
				continue;
			}

			TInlineComponentArray<UStaticMeshComponent*> PrimitiveComponents;
			CurrentActor->GetComponents<UStaticMeshComponent>(PrimitiveComponents);

			for (UStaticMeshComponent* PrimitiveComponent : PrimitiveComponents)
			{
				// Skip sky sphere
				if (IsValid(PrimitiveComponent))
				{
					TObjectPtr<UStaticMesh> StaticMesh = PrimitiveComponent->GetStaticMesh();
					if (StaticMesh.Get())
					{
						FString StrStaticMeshName = StaticMesh->GetName();
						if (StrStaticMeshName.Contains("SkySphere"))
						{
							continue;
						}
					}

					// Check selection
					// TODO remove selection check
					AActor* Owner = PrimitiveComponent->GetOwner();
					TSharedPtr<FMeshDataIterators::FEdgeIterator> EdgeGetter =
						FMeshDataIterators::MakeEdgeIterator(PrimitiveComponent);
					// Collect edges
					if ((Owner != nullptr && Owner->IsSelected() || PrimitiveComponent->IsSelected()) &&
						EdgeGetter.IsValid())
					{
						FMeshDataIterators::FEdgeIterator& EdgeGetterRef = *EdgeGetter;
						for (; EdgeGetterRef; ++EdgeGetterRef)
						{
							FVector2D FirstVertexOnScreen{};
							ThisBackgroundThread->EdModeView->WorldToPixel(
								EdgeGetterRef.FirstEndpoint(), FirstVertexOnScreen);
							FirstVertexOnScreen /= ThisBackgroundThread->DPIScale;

							FVector2D SecondVertexOnScreen{};
							ThisBackgroundThread->EdModeView->WorldToPixel(
								EdgeGetterRef.SecondEndpoint(), SecondVertexOnScreen);
							SecondVertexOnScreen /= ThisBackgroundThread->DPIScale;

							FMeshEdgeData CapturedEdgeData;
							CapturedEdgeData.EdgeOwnerActor = PrimitiveComponent->GetOwner();
							CapturedEdgeData.FirstEndpointInWorldPosition = EdgeGetterRef.FirstEndpoint();
							CapturedEdgeData.FirstEndpointOnScreenPosition = FirstVertexOnScreen;

							CapturedEdgeData.SecondEndpointInWorldPosition = EdgeGetterRef.SecondEndpoint();
							CapturedEdgeData.SecondEndpointOnScreenPosition = SecondVertexOnScreen;


							ThisBackgroundThread->CapturedEdgeData.Add(CapturedEdgeData);
						}
					}
				}
			}
		}

		// Algo::Sort(ThisBackgroundThread->CapturedEdgeData);

		AsyncTask(ENamedThreads::GameThread, [WeakThisPtr]()
		{
			const FMeshEditorEditorMode* ThisGameThread{WeakThisPtr.Pin().Get()};
			if (ThisGameThread)
			{
				ThisGameThread->OnCollectingDataFinished.ExecuteIfBound();
			}
		});
	});
}

void FMeshEditorEditorMode::InvalidateHitProxies()
{
	if (bIsMouseMove)
	{
		GEditor->GetActiveViewport()->InvalidateHitProxy();
	}
}

bool FMeshEditorEditorMode::IsInAttachMovementMode()
{
	return bIsBKeyDown;
}

void FMeshEditorEditorMode::RefreshPrefabs()
{
	// TODO 采用 UE 自带框架搜索 PrefabActor
	for (auto PrefabPtr : APrefabActor::PrefabContainer)
	{
		if (!PrefabPtr.Get())
		{
			continue;
		}

		if (!PrefabStateMap.Contains(PrefabPtr))
		{
			PrefabStateMap.Add(PrefabPtr, PrefabState(false));
		}
	}
}

void FMeshEditorEditorMode::RefreshPrefabStatus()
{
	// TODO 采用 UE 自带框架搜索 PrefabActor
	for (auto& PrefabElem : PrefabStateMap)
	{
		TWeakObjectPtr<APrefabActor> Prefab = PrefabElem.Key;
		PrefabState& State = PrefabElem.Value;
		if (!Prefab.Get())
		{
			continue;
		}

		State.bIsContainAttachment = false;
	}
}


#undef LOCTEXT_NAMESPACE
