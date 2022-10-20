// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
// #include "DragTransaction.generated.h"

struct MESHEDITOR_API FDragTransaction
{
	FDragTransaction();

	~FDragTransaction();

	void Begin(const FText& Description);

	void End();

private:
	UTypedElementSelectionSet* GetMutableSelectionSet() const;

	class FScopedTransaction* ScopedTransaction = nullptr;
};
