﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/PCGExGraphProcessor.h"

#include "IPCGExDebug.h"
#include "Data/PCGExGraphParamsData.h"

#define LOCTEXT_NAMESPACE "PCGExGraphSettings"

#pragma region UPCGSettings interface


TArray<FPCGPinProperties> UPCGExGraphProcessorSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::InputPinProperties();
	FPCGPinProperties& PinPropertyParams = PinProperties.Emplace_GetRef(PCGExGraph::SourceParamsLabel, EPCGDataType::Param);

#if WITH_EDITOR
	PinPropertyParams.Tooltip = LOCTEXT("PCGExSourceParamsPinTooltip", "Graph Params. Data is de-duped internally.");
#endif // WITH_EDITOR

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGExGraphProcessorSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::OutputPinProperties();
	FPCGPinProperties& PinParamsOutput = PinProperties.Emplace_GetRef(PCGExGraph::OutputParamsLabel, EPCGDataType::Param);

#if WITH_EDITOR
	PinParamsOutput.Tooltip = LOCTEXT("PCGExOutputParamsTooltip", "Graph Params forwarding. Data is de-duped internally.");
#endif // WITH_EDITOR

	return PinProperties;
}

FName UPCGExGraphProcessorSettings::GetMainPointsInputLabel() const { return PCGExGraph::SourceGraphsLabel; }
FName UPCGExGraphProcessorSettings::GetMainPointsOutputLabel() const { return PCGExGraph::OutputGraphsLabel; }

FPCGExGraphProcessorContext::~FPCGExGraphProcessorContext()
{
	PCGEX_DELETE(CachedIndexReader)
	PCGEX_DELETE(CachedIndexWriter)
	SocketInfos.Empty();
}

#pragma endregion

bool FPCGExGraphProcessorContext::AdvanceGraph(bool bResetPointsIndex)
{
	if (bResetPointsIndex) { CurrentPointsIndex = -1; }
	CurrentParamsIndex++;
	if (Graphs.Params.IsValidIndex(CurrentParamsIndex))
	{
		CurrentGraph = Graphs.Params[CurrentParamsIndex];
		CurrentGraph->GetSocketsInfos(SocketInfos);
		return true;
	}

	CurrentGraph = nullptr;
	return false;
}

bool FPCGExGraphProcessorContext::AdvancePointsIO(bool bResetParamsIndex)
{
	if (bResetParamsIndex) { CurrentParamsIndex = -1; }
	return FPCGExPointsProcessorContext::AdvancePointsIO();
}

void FPCGExGraphProcessorContext::Reset()
{
	FPCGExPointsProcessorContext::Reset();
	CurrentParamsIndex = -1;
}

void FPCGExGraphProcessorContext::SetCachedIndex(const int32 PointIndex, const int32 Index) const
{
	check(!bReadOnly)
	(*CachedIndexWriter)[PointIndex] = Index;
}

int32 FPCGExGraphProcessorContext::GetCachedIndex(const int32 PointIndex) const
{
	if (bReadOnly) { return (*CachedIndexReader)[PointIndex]; }
	return (*CachedIndexWriter)[PointIndex];
}

void FPCGExGraphProcessorContext::PrepareCurrentGraphForPoints(const PCGExData::FPointIO& PointIO, const bool ReadOnly)
{
	bReadOnly = ReadOnly;
	if (bReadOnly)
	{
		PCGEX_DELETE(CachedIndexWriter)
		if (!CachedIndexReader) { CachedIndexReader = new PCGEx::TFAttributeReader<int32>(CurrentGraph->CachedIndexAttributeName); }
		CachedIndexReader->Bind(const_cast<PCGExData::FPointIO&>(PointIO));
	}
	else
	{
		PCGEX_DELETE(CachedIndexReader)
		if (!CachedIndexWriter) { CachedIndexWriter = new PCGEx::TFAttributeWriter<int32>(CurrentGraph->CachedIndexAttributeName, -1, false); }
		CachedIndexWriter->BindAndGet(const_cast<PCGExData::FPointIO&>(PointIO));
	}

	CurrentGraph->PrepareForPointData(PointIO, bReadOnly);
}

FPCGContext* FPCGExGraphProcessorElement::Initialize(
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node)
{
	FPCGExGraphProcessorContext* Context = new FPCGExGraphProcessorContext();
	InitializeContext(Context, InputData, SourceComponent, Node);
	return Context;
}

bool FPCGExGraphProcessorElement::Validate(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Validate(InContext)) { return false; }

	PCGEX_CONTEXT(FPCGExGraphProcessorContext)

	if (Context->Graphs.IsEmpty())
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingParams", "Missing Input Params."));
		return false;
	}

	return true;
}

void FPCGExGraphProcessorElement::InitializeContext(
	FPCGExPointsProcessorContext* InContext,
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node) const
{
	FPCGExPointsProcessorElementBase::InitializeContext(InContext, InputData, SourceComponent, Node);

	FPCGExGraphProcessorContext* Context = static_cast<FPCGExGraphProcessorContext*>(InContext);

	TArray<FPCGTaggedData> Sources = Context->InputData.GetInputsByPin(PCGExGraph::SourceParamsLabel);
	Context->Graphs.Initialize(InContext, Sources);
}


#undef LOCTEXT_NAMESPACE
