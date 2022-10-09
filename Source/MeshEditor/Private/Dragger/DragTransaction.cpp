// Fill out your copyright notice in the Description page of Project Settings.


#include "DragTransaction.h"

#include "Selection.h"
#include "Engine/StaticMeshActor.h"


FDragTransaction::FDragTransaction()
{
}

FDragTransaction::~FDragTransaction()
{
	End();
}

void FDragTransaction::Begin(const FText& Description)
{
	End();
	ScopedTransaction = new FScopedTransaction(Description);

	if (UTypedElementSelectionSet* SelectionSet = GetMutableSelectionSet())
	{
		auto ModifyObject = [this](UObject* InObject)
		{
			AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(InObject);
			if (MeshActor != nullptr)
			{
				MeshActor->Modify();
			}
			return true;
		};

		SelectionSet->ForEachSelectedObject(ModifyObject);
	}
}

void FDragTransaction::End()
{
	if (ScopedTransaction)
	{
		delete ScopedTransaction;
		ScopedTransaction = nullptr;
	}
}


UTypedElementSelectionSet* FDragTransaction::GetMutableSelectionSet() const
{
	return (GEditor && GEditor->GetSelectedActors() ? GEditor->GetSelectedActors()->GetElementSelectionSet() : nullptr);
}
