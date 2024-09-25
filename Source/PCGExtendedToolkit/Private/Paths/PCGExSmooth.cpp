﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Paths/PCGExSmooth.h"

#include "Data/Blending/PCGExMetadataBlender.h"


#include "Paths/Smoothing/PCGExMovingAverageSmoothing.h"

#define LOCTEXT_NAMESPACE "PCGExSmoothElement"
#define PCGEX_NAMESPACE Smooth

PCGExData::EInit UPCGExSmoothSettings::GetMainOutputInitMode() const { return PCGExData::EInit::DuplicateInput; }

PCGEX_INITIALIZE_ELEMENT(Smooth)

FPCGExSmoothContext::~FPCGExSmoothContext()
{
	PCGEX_TERMINATE_ASYNC
}

bool FPCGExSmoothElement::Boot(FPCGExContext* InContext) const
{
	if (!FPCGExPathProcessorElement::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(Smooth)
	PCGEX_OPERATION_BIND(SmoothingMethod, UPCGExSmoothingOperation)

	return true;
}

bool FPCGExSmoothElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExSmoothElement::Execute);

	PCGEX_CONTEXT_AND_SETTINGS(Smooth)

	if (Context->IsSetup())
	{
		if (!Boot(Context)) { return true; }

		bool bInvalidInputs = false;

		if (!Context->StartBatchProcessingPoints<PCGExPointsMT::TBatch<PCGExSmooth::FProcessor>>(
			[&](PCGExData::FPointIO* Entry)
			{
				if (Entry->GetNum() < 2)
				{
					bInvalidInputs = true;
					return false;
				}
				return true;
			},
			[&](PCGExPointsMT::TBatch<PCGExSmooth::FProcessor>* NewBatch)
			{
				NewBatch->PrimaryOperation = Context->SmoothingMethod;
			},
			PCGExMT::State_Done))
		{
			PCGE_LOG(Warning, GraphAndLog, FTEXT("Could not find any paths to smooth."));
			return true;
		}

		if (bInvalidInputs)
		{
			PCGE_LOG(Warning, GraphAndLog, FTEXT("Some inputs have less than 2 points and won't be processed."));
		}
	}

	if (!Context->ProcessPointsBatch()) { return false; }

	Context->MainPoints->OutputToContext();

	return Context->TryComplete();
}

namespace PCGExSmooth
{
	FProcessor::~FProcessor()
	{
	}

	bool FProcessor::Process(TSharedPtr<PCGExMT::FTaskManager> InAsyncManager)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PCGExSmooth::Process);

		PointDataFacade->bSupportsScopedGet = Context->bScopedAttributeGet;

		if (!FPointsProcessor::Process(InAsyncManager)) { return false; }


		bClosedLoop = Context->ClosedLoop.IsClosedLoop(PointIO);
		NumPoints = PointIO->GetNum();

		MetadataBlender = MakeUnique<PCGExDataBlending::FMetadataBlender>(&Settings->BlendingSettings);
		MetadataBlender->PrepareForData(PointDataFacade);

		if (Settings->InfluenceType == EPCGExFetchType::Attribute)
		{
			Influence = PointDataFacade->GetScopedBroadcaster<double>(Settings->InfluenceAttribute);
			if (!Influence)
			{
				PCGE_LOG_C(Error, GraphAndLog, ExecutionContext, FText::Format(FTEXT("Input missing influence attribute: \"{0}\"."), FText::FromName(Settings->InfluenceAttribute.GetName())));
				return false;
			}
		}

		if (Settings->SmoothingAmountType == EPCGExFetchType::Attribute)
		{
			Smoothing = PointDataFacade->GetScopedBroadcaster<double>(Settings->SmoothingAmountAttribute);
			if (!Smoothing)
			{
				PCGE_LOG_C(Error, GraphAndLog, ExecutionContext, FText::Format(FTEXT("Input missing smoothing amount attribute: \"{0}\"."), FText::FromName(Settings->InfluenceAttribute.GetName())));
				return false;
			}
		}

		TypedOperation = Cast<UPCGExSmoothingOperation>(PrimaryOperation);

		StartParallelLoopForPoints();

		return true;
	}

	void FProcessor::PrepareSingleLoopScopeForPoints(const uint32 StartIndex, const int32 Count)
	{
		PointDataFacade->Fetch(StartIndex, Count);
		FilterScope(StartIndex, Count);
	}

	void FProcessor::ProcessSinglePoint(const int32 Index, FPCGPoint& Point, const int32 LoopIdx, const int32 Count)
	{
		if (!PointFilterCache[Index]) { return; }

		PCGExData::FPointRef PtRef = PointIO->GetOutPointRef(Index);
		const double LocalSmoothing = Smoothing ? FMath::Clamp(Smoothing->Read(Index), 0, TNumericLimits<double>::Max()) * Settings->ScaleSmoothingAmountAttribute : Settings->SmoothingAmountConstant;

		if ((Settings->bPreserveEnd && Index == NumPoints - 1) ||
			(Settings->bPreserveStart && Index == 0))
		{
			TypedOperation->SmoothSingle(PointIO.Get(), PtRef, LocalSmoothing, 0, MetadataBlender.Get(), bClosedLoop);
			return;
		}

		const double LocalInfluence = Influence ? Influence->Read(Index) : Settings->InfluenceConstant;
		TypedOperation->SmoothSingle(PointIO.Get(), PtRef, LocalSmoothing, LocalInfluence, MetadataBlender.Get(), bClosedLoop);
	}

	void FProcessor::CompleteWork()
	{
		PointDataFacade->Write(AsyncManager);
	}
}
#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
