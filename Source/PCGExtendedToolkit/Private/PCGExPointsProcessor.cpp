﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "PCGExPointsProcessor.h"

#include "PCGContext.h"
#include "PCGPin.h"

#define LOCTEXT_NAMESPACE "PCGExRelationalSettings"

#pragma region UPCGSettings interface

UPCGExPointsProcessorSettings::UPCGExPointsProcessorSettings(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (ChunkSize <= 0) { ChunkSize = UPCGExPointsProcessorSettings::GetPreferredChunkSize(); }
}

TArray<FPCGPinProperties> UPCGExPointsProcessorSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& PinPropertySource = PinProperties.Emplace_GetRef(PCGEx::SourcePointsLabel, EPCGDataType::Point);

#if WITH_EDITOR
	PinPropertySource.Tooltip = LOCTEXT("PCGExSourcePointsPinTooltip", "The point data to be processed.");
#endif // WITH_EDITOR

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGExPointsProcessorSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& PinPointsOutput = PinProperties.Emplace_GetRef(PCGEx::OutputPointsLabel, EPCGDataType::Point);

#if WITH_EDITOR
	PinPointsOutput.Tooltip = LOCTEXT("PCGExOutputPointsPinTooltip", "The processed points.");
#endif // WITH_EDITOR

	return PinProperties;
}

PCGEx::EIOInit UPCGExPointsProcessorSettings::GetPointOutputInitMode() const { return PCGEx::EIOInit::NewOutput; }

int32 UPCGExPointsProcessorSettings::GetPreferredChunkSize() const { return 256; }

bool FPCGExPointsProcessorContext::AdvancePointsIO()
{
	CurrentPointsIndex++;
	if (Points->Pairs.IsValidIndex(CurrentPointsIndex))
	{
		CurrentIO = Points->Pairs[CurrentPointsIndex];
		return true;
	}
	CurrentIO = nullptr;
	return false;
}

void FPCGExPointsProcessorContext::SetState(PCGExMT::EState OperationId)
{
	PCGExMT::EState PreviousOperation = CurrentState;
	CurrentState = OperationId;
}

void FPCGExPointsProcessorContext::Reset() { CurrentState = PCGExMT::EState::Setup; }

bool FPCGExPointsProcessorContext::IsValid() { return !Points->IsEmpty(); }

bool FPCGExPointsProcessorContext::ValidatePointDataInput(UPCGPointData* PointData) { return true; }

void FPCGExPointsProcessorContext::PostInitPointDataInput(UPCGExPointIO* PointData)
{
}

#pragma endregion

FPCGContext* FPCGExPointsProcessorElementBase::Initialize(
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node)
{
	FPCGExPointsProcessorContext* Context = new FPCGExPointsProcessorContext();
	InitializeContext(Context, InputData, SourceComponent, Node);
	return Context;
}

void FPCGExPointsProcessorElementBase::InitializeContext(
	FPCGExPointsProcessorContext* InContext,
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node) const
{
	InContext->InputData = InputData;
	InContext->SourceComponent = SourceComponent;
	InContext->Node = Node;

	const UPCGExPointsProcessorSettings* Settings = InContext->GetInputSettings<UPCGExPointsProcessorSettings>();
	check(Settings);

	InContext->ChunkSize = Settings->ChunkSize;

	InContext->Points = NewObject<UPCGExPointIOGroup>();
	TArray<FPCGTaggedData> Sources = InContext->InputData.GetInputsByPin(PCGEx::SourcePointsLabel);
	InContext->Points->Initialize(
		InContext,
		Sources,
		Settings->GetPointOutputInitMode(),
		[&InContext](UPCGPointData* Data) { return InContext->ValidatePointDataInput(Data); },
		[&InContext](UPCGExPointIO* IO) { return InContext->PostInitPointDataInput(IO); });
}

#undef LOCTEXT_NAMESPACE
