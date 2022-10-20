// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
// #include "MeshDataIterators.generated.h"

namespace FMeshDataIterators
{
	class FVertexIterator
	{
	public:
		virtual ~FVertexIterator()
		{
		};

		/** Advances to the next vertex */
		void operator++()
		{
			Advance();
		}

		/** @return True if there are more vertices on the component */
		explicit operator bool() const
		{
			return HasMoreVertices();
		}

		/**
		* @return The position in world space of the current vertex
		*/
		virtual FVector Position() const = 0;

		/**
		* @return The position in world space of the current vertex normal
		*/
		virtual FVector Normal() const = 0;

	protected:
		/**
		* @return True if there are more vertices on the component
		*/
		virtual bool HasMoreVertices() const = 0;

		/**
		* Advances to the next vertex
		*/
		virtual void Advance() = 0;
	};

	class FEdgeIterator
	{
	public:
		virtual ~FEdgeIterator()
		{
		};

		/** Advances to the next vertex */
		void operator++()
		{
			Advance();
		}

		/** @return True if there are more vertices on the component */
		explicit operator bool() const
		{
			return HasMoreEdges();
		}

		virtual FVector FirstEndpoint() const = 0;

		virtual FVector SecondEndpoint() const = 0;

		virtual int32 FirstEndpointIndex() const = 0;

		virtual int32 SecondEndpointIndex() const = 0;

		/**
		* @return The position in world space of the current vertex
		*/
		// virtual FVector Position() const = 0;

		/**
		* @return The position in world space of the current vertex normal
		*/
		// virtual FVector Normal() const = 0;

	protected:
		/**
		* @return True if there are more vertices on the component
		*/
		virtual bool HasMoreEdges() const = 0;

		/**
		* Advances to the next edge
		*/
		virtual void Advance() = 0;
	};

	class FStaticMeshVertexIterator : public FVertexIterator
	{
	public:
		FStaticMeshVertexIterator(UStaticMeshComponent* SMC);

		/** FVertexIterator interface */
		virtual FVector Position() const override;
		virtual FVector Normal() const override;

	protected:
		virtual void Advance() override;
		virtual bool HasMoreVertices() const override;

	private:
		/** Component To World Inverse Transpose matrix */
		FMatrix ComponentToWorldIT;
		/** Component containing the mesh that we are getting vertices from */
		UStaticMeshComponent* StaticMeshComponent;
		/** The static meshes position vertex buffer */
		FPositionVertexBuffer& PositionBuffer;
		/** The static meshes vertex buffer for normals */
		FStaticMeshVertexBuffer& VertexBuffer;
		/** Current vertex index */
		uint32 CurrentVertexIndex;
	};

	class FStaticMeshEdgeIterator : public FEdgeIterator
	{
	public:
		FStaticMeshEdgeIterator(UStaticMeshComponent* SMC);

		virtual FVector FirstEndpoint() const override;

		virtual FVector SecondEndpoint() const override;

		virtual int32 FirstEndpointIndex() const override;

		virtual int32 SecondEndpointIndex() const override;

	protected:
		virtual void Advance() override;
		virtual bool HasMoreEdges() const override;

	private:
		/** Component To World Inverse Transpose matrix */
		FMatrix ComponentToWorldIT;
		/** Component containing the mesh that we are getting vertices from */
		UStaticMeshComponent* StaticMeshComponent;
		/** The static meshes position vertex buffer */
		FPositionVertexBuffer& PositionBuffer;
		// TODO delete
		/** The static meshes vertex buffer for normals */
		// FStaticMeshVertexBuffer& VertexBuffer;
		/** Current vertex index */
		int32 CurrentEdgeIndex;
		int32 CurrentTriangeVertexIndex;
		int32 MaxEdgeIndex;
		FRawStaticIndexBuffer& IndexBuffer;
	};

	/**
	* Makes a vertex iterator from the specified component
	*/
	static TSharedPtr<FVertexIterator> MakeVertexIterator(UPrimitiveComponent* Component);

	/**
	* Makes a edge iterator from the specified component
	*/
	static TSharedPtr<FEdgeIterator> MakeEdgeIterator(UPrimitiveComponent* Component);
}
