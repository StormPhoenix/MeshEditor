// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawHelper.h"

namespace DrawHelper
{
	void ComputeBracketCornerAndAxisForSingleMesh(const UStaticMeshComponent* InComp, TArray<FVector>& OutCorners,
												  TArray<FVector>& OutAxis)
	{
		FBox LocalBox(ForceInit);

		// Only use collidable components to find collision bounding box.
		if (InComp->IsRegistered() && (true || InComp->IsCollisionEnabled()))
		{
			LocalBox = InComp->CalcBounds(FTransform::Identity).GetBox();
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
		const FTransform& CompTransform = InComp->GetComponentTransform();

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

		{
			// Reverse axis direction
			FVector3d ScaleVec = CompTransform.GetScale3D();
			GlobalAxisX *= (ScaleVec[0] < 0.f ? -1.f : 1.f);
			GlobalAxisY *= (ScaleVec[1] < 0.f ? -1.f : 1.f);
			GlobalAxisZ *= (ScaleVec[2] < 0.f ? -1.f : 1.f);
		}

		// Store corners and axis
		OutCorners.Empty();
		for (FVector Corner : BracketCorners)
		{
			OutCorners.Add(Corner);
		}

		OutAxis.Empty();
		OutAxis.Add(GlobalAxisX);
		OutAxis.Add(GlobalAxisY);
		OutAxis.Add(GlobalAxisZ);
	}

	void ComputeBracketCornerAndAxisForMultipleMesh(TArray<TWeakObjectPtr<AStaticMeshActor>>& MeshActorArray,
													TArray<FVector>& OutCorners, TArray<FVector>& OutAxis)
	{
		FBox ActorBounds(EForceInit::ForceInit);
		for (TWeakObjectPtr<AStaticMeshActor> MeshActor : MeshActorArray)
		{
			if (MeshActor.Get())
			{
				for (const UActorComponent* ActorComp : MeshActor->GetComponents())
				{
					const UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(ActorComp);
					if (MeshComp != nullptr)
					{
						ActorBounds += MeshComp->Bounds.GetBox();
					}
				}
			}
		}

		FVector Min = ActorBounds.Min;
		FVector Max = ActorBounds.Max;

		OutCorners.Empty();
		OutCorners.Add(FVector(Min.X, Min.Y, Min.Z));
		OutCorners.Add(FVector(Min.X, Max.Y, Min.Z));
		OutCorners.Add(FVector(Max.X, Max.Y, Min.Z));
		OutCorners.Add(FVector(Max.X, Min.Y, Min.Z));

		OutCorners.Add(FVector(Min.X, Min.Y, Max.Z));
		OutCorners.Add(FVector(Min.X, Max.Y, Max.Z));
		OutCorners.Add(FVector(Max.X, Max.Y, Max.Z));
		OutCorners.Add(FVector(Max.X, Min.Y, Max.Z));

		OutAxis.Empty();
		OutAxis.Add(FVector(1.0f, 0.0f, 0.0f));
		OutAxis.Add(FVector(0.0f, 1.0f, 0.0f));
		OutAxis.Add(FVector(0.0f, 0.0f, 1.0f));
	}

	void DrawBrackets(FPrimitiveDrawInterface* PDI, const FLinearColor DrawColor, const float LineThickness,
					  TArray<FVector>& InBracketCorners, TArray<FVector>& InGlobalAxis)
	{
		const float BracketOffsetFactor = 0.2;
		float BracketOffset;
		float BracketPadding;
		{
			const float DeltaX = (InBracketCorners[3] - InBracketCorners[0]).Length();
			const float DeltaY = (InBracketCorners[1] - InBracketCorners[0]).Length();
			const float DeltaZ = (InBracketCorners[4] - InBracketCorners[0]).Length();
			BracketOffset = FMath::Min(FMath::Min(DeltaX, DeltaY), DeltaZ) * BracketOffsetFactor;
			BracketPadding = BracketOffset * 0.08;
		}

		const int32 DIR_X[] = {1, 1, -1, -1, 1, 1, -1, -1};
		const int32 DIR_Y[] = {1, -1, -1, 1, 1, -1, -1, 1};
		const int32 DIR_Z[] = {1, 1, 1, 1, -1, -1, -1, -1};

		for (int32 BracketCornerIndex = 0; BracketCornerIndex < InBracketCorners.Num(); ++BracketCornerIndex)
		{
			// Direction corner axis should be pointing based on min/max
			const FVector CORNER = InBracketCorners[BracketCornerIndex]
				- (BracketPadding * DIR_X[BracketCornerIndex] * InGlobalAxis[0])
				- (BracketPadding * DIR_Y[BracketCornerIndex] * InGlobalAxis[1])
				- (BracketPadding * DIR_Z[BracketCornerIndex] * InGlobalAxis[2]);

			PDI->DrawLine(CORNER, CORNER + (BracketOffset * DIR_X[BracketCornerIndex] * InGlobalAxis[0]),
						  DrawColor, SDPG_Foreground, LineThickness);
			PDI->DrawLine(CORNER, CORNER + (BracketOffset * DIR_Y[BracketCornerIndex] * InGlobalAxis[1]),
						  DrawColor, SDPG_Foreground, LineThickness);
			PDI->DrawLine(CORNER, CORNER + (BracketOffset * DIR_Z[BracketCornerIndex] * InGlobalAxis[2]),
						  DrawColor, SDPG_Foreground, LineThickness);
		}
	}

	void DrawBracketForDragger(FPrimitiveDrawInterface* PDI, FViewport* Viewport,
							   FAxisDragger* Dragger, TArray<FVector>& OutVerts)
	{
		if (Dragger == nullptr)
		{
			return;
		}

		if (Dragger->IsMeshDragger())
		{
			TWeakObjectPtr<AStaticMeshActor> MeshActorPtr = Dragger->GetMeshActor();

			if (!MeshActorPtr.Get())
			{
				return;
			}

			if (MeshActorPtr->GetWorld() != PDI->View->Family->Scene->GetWorld())
			{
				return;
			}

			// TODO 默认一个 MeshActor 只有一个 AStaticMeshComp，之后在做修改
			MeshActorPtr->ForEachComponent<UStaticMeshComponent>(
				false, [&](const UStaticMeshComponent* InPrimComp)
				{
					// Draw bracket for each component
					if (InPrimComp == nullptr)
					{
						return;
					}

					const FLinearColor MESH_BRACKET_COLOR = {0.0f, 1.0f, 0.0f};
					TArray<FVector> BracketCorners;
					TArray<FVector> GlobalAxis;

					ComputeBracketCornerAndAxisForSingleMesh(InPrimComp, BracketCorners, GlobalAxis);
					DrawBrackets(PDI, MESH_BRACKET_COLOR, 0.0f, BracketCorners, GlobalAxis);

					for (int32 BracketCornerIndex = 0; BracketCornerIndex < BracketCorners.Num(); ++
						 BracketCornerIndex)
					{
						OutVerts.Add(BracketCorners[BracketCornerIndex]);
					}
				});
		}
		else if (Dragger->IsGroupDragger())
		{
			// Draw bracket for group
			const FLinearColor GROUP_MESH_BRACKET_COLOR = {0.0f, 0.5f, 0.0f};
			const float GROUP_BRACKET_THICKNESS = 8.0f;
			TArray<FVector> BracketCorners;
			TArray<FVector> GlobalAxis;

			ComputeBracketCornerAndAxisForMultipleMesh(Dragger->GetGroupMeshInfo().GroupMeshArray, BracketCorners,
													   GlobalAxis);
			DrawBrackets(PDI, GROUP_MESH_BRACKET_COLOR, GROUP_BRACKET_THICKNESS, BracketCorners, GlobalAxis);

			for (int32 BracketCornerIndex = 0; BracketCornerIndex < BracketCorners.Num(); ++BracketCornerIndex)
			{
				OutVerts.Add(BracketCorners[BracketCornerIndex]);
			}
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("FMeshEditorEditorMode::DrawBracket() failed. "));
		}
	}
}