// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshDataIterators.h"

namespace FMeshDataIterators
{
	FStaticMeshVertexIterator::FStaticMeshVertexIterator(UStaticMeshComponent* SMC)
	: ComponentToWorldIT(SMC->GetComponentTransform().ToInverseMatrixWithScale().GetTransposed())
	  , StaticMeshComponent(SMC)
	, PositionBuffer(SMC->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer)
	, VertexBuffer(SMC->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer),
	  CurrentVertexIndex(0)
	{
	}

	FVector FStaticMeshVertexIterator::Position() const
	{
		return StaticMeshComponent->GetComponentTransform().TransformPosition(
			UE::Math::TVector<double>{PositionBuffer.VertexPosition(CurrentVertexIndex)});
	}

	FVector FStaticMeshVertexIterator::Normal() const
	{
		return ComponentToWorldIT.TransformVector(static_cast<FVector4>(VertexBuffer.VertexTangentZ(CurrentVertexIndex)));
	}

	void FStaticMeshVertexIterator::Advance()
	{
		++CurrentVertexIndex;
	}

	bool FStaticMeshVertexIterator::HasMoreVertices() const
	{
		return CurrentVertexIndex < PositionBuffer.GetNumVertices();
	}
	
	FStaticMeshEdgeIterator::FStaticMeshEdgeIterator(UStaticMeshComponent* SMC)
		: ComponentToWorldIT(SMC->GetComponentTransform().ToInverseMatrixWithScale().GetTransposed())
		  , StaticMeshComponent(SMC)
		  , PositionBuffer(SMC->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer)
		  , IndexBuffer(SMC->GetStaticMesh()->GetRenderData()->LODResources[0].IndexBuffer)
		  , CurrentEdgeIndex(0)
		  , CurrentTriangeVertexIndex(0), MaxEdgeIndex(0)
	{
	}

	void FStaticMeshEdgeIterator::Advance()
	{
		CurrentTriangeVertexIndex = (CurrentTriangeVertexIndex + 1) % 3;
		if (CurrentTriangeVertexIndex == 0)
		{
			CurrentEdgeIndex ++;
		}
	}

	bool FStaticMeshEdgeIterator::HasMoreEdges() const
	{
		return (CurrentEdgeIndex * 3 + CurrentTriangeVertexIndex) < IndexBuffer.GetNumIndices();
	}

	FVector FStaticMeshEdgeIterator::FirstEndpoint() const
	{
		uint32 FirstPositionIndex = FirstEndpointIndex();
		return StaticMeshComponent->GetComponentTransform().TransformPosition(
			UE::Math::TVector<double>{PositionBuffer.VertexPosition(FirstPositionIndex)});
	}

	FVector FStaticMeshEdgeIterator::SecondEndpoint() const
	{
		uint32 SecondPositionIndex = SecondEndpointIndex();
		return StaticMeshComponent->GetComponentTransform().TransformPosition(
			UE::Math::TVector<double>{PositionBuffer.VertexPosition(SecondPositionIndex)});
	}

	int32 FStaticMeshEdgeIterator::FirstEndpointIndex() const
	{
		return IndexBuffer.GetArrayView()[CurrentTriangeVertexIndex + CurrentEdgeIndex * 3];
	}

	int32 FStaticMeshEdgeIterator::SecondEndpointIndex() const
	{
		return IndexBuffer.GetArrayView()[(CurrentTriangeVertexIndex + 1) % 3 + CurrentEdgeIndex * 3];
	}

	TSharedPtr<FVertexIterator> MakeVertexIterator(UPrimitiveComponent* Component)
	{
		UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Component);
		if (SMC && SMC->GetStaticMesh())
		{
			return MakeShareable(new FStaticMeshVertexIterator(SMC));
		}
		return nullptr;
	}
	
	TSharedPtr<FEdgeIterator> MakeEdgeIterator(UPrimitiveComponent* Component)
	{
		UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Component);
		if (SMC && SMC->GetStaticMesh())
		{
			return MakeShareable(new FStaticMeshEdgeIterator(SMC));
		}

		return nullptr;
	}
}
