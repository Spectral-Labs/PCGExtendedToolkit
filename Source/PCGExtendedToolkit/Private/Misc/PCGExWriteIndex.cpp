﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Misc/PCGExWriteIndex.h"

#define LOCTEXT_NAMESPACE "PCGExWriteIndexElement"

PCGExData::EInit UPCGExWriteIndexSettings::GetPointOutputInitMode() const { return PCGExData::EInit::DuplicateInput; }

FPCGElementPtr UPCGExWriteIndexSettings::CreateElement() const { return MakeShared<FPCGExWriteIndexElement>(); }

FPCGExWriteIndexContext::~FPCGExWriteIndexContext()
{
	IndicesBuffer.Empty();
	NormalizedIndicesBuffer.Empty();
	PCGEX_DELETE(NormalizedIndexAccessor)
	PCGEX_DELETE(IndexAccessor)
}

FPCGContext* FPCGExWriteIndexElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
{
	FPCGExWriteIndexContext* Context = new FPCGExWriteIndexContext();
	InitializeContext(Context, InputData, SourceComponent, Node);

	PCGEX_SETTINGS(UPCGExWriteIndexSettings)

	PCGEX_FWD(bOutputNormalizedIndex)
	PCGEX_FWD(OutputAttributeName)

	return Context;
}

bool FPCGExWriteIndexElement::Validate(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Validate(InContext)) { return false; }

	PCGEX_CONTEXT(FPCGExWriteIndexContext)

	PCGEX_VALIDATE_NAME(Context->OutputAttributeName)

	return true;
}

bool FPCGExWriteIndexElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExWriteIndexElement::Execute);

	PCGEX_CONTEXT(FPCGExWriteIndexContext)

	if (Context->IsSetup())
	{
		if (!Validate(Context)) { return true; }
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsState(PCGExMT::State_ReadyForNextPoints))
	{
		if (!Context->AdvancePointsIO()) { Context->Done(); }
		else { Context->SetState(PCGExMT::State_ProcessingPoints); }
	}

	if (Context->IsState(PCGExMT::State_ProcessingPoints))
	{
		if (Context->bOutputNormalizedIndex)
		{
			auto Initialize = [&](PCGExData::FPointIO& PointIO)
			{
				Context->NormalizedIndicesBuffer.Reset(PointIO.GetNum());
				Context->NormalizedIndexAccessor = PCGEx::FAttributeAccessor<double>::FindOrCreate(PointIO, Context->OutputAttributeName, -1, false);
				Context->NormalizedIndexAccessor->GetRange(Context->NormalizedIndicesBuffer, 0);
			};

			if (Context->ProcessCurrentPoints(
				Initialize, [&](const int32 Index, const PCGExData::FPointIO& PointIO)
				{
					Context->NormalizedIndicesBuffer[Index] = static_cast<double>(Index) / static_cast<double>(Context->NormalizedIndicesBuffer.Num());
				}))
			{
				Context->NormalizedIndexAccessor->SetRange(Context->NormalizedIndicesBuffer);
				PCGEX_DELETE(Context->NormalizedIndexAccessor)
				Context->SetState(PCGExMT::State_ReadyForNextPoints);
				Context->CurrentIO->OutputTo(Context);
			}
		}
		else
		{
			auto Initialize = [&](PCGExData::FPointIO& PointIO)
			{
				Context->IndicesBuffer.Reset(PointIO.GetNum());
				Context->IndexAccessor = PCGEx::FAttributeAccessor<int32>::FindOrCreate(PointIO, Context->OutputAttributeName, -1, false);
				Context->IndexAccessor->GetRange(Context->IndicesBuffer, 0);
			};

			if (Context->ProcessCurrentPoints(
				Initialize, [&](const int32 Index, const PCGExData::FPointIO& PointIO)
				{
					Context->IndicesBuffer[Index] = Index;
				}))
			{
				Context->IndexAccessor->SetRange(Context->IndicesBuffer);
				PCGEX_DELETE(Context->IndexAccessor)
				Context->SetState(PCGExMT::State_ReadyForNextPoints);
				Context->CurrentIO->OutputTo(Context);
			}
		}
	}

	return Context->IsDone();
}

#undef LOCTEXT_NAMESPACE
