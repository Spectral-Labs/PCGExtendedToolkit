﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/


#include "Misc/PCGExUberFilterCollections.h"

#include "Data/PCGExData.h"
#include "Data/PCGExPointFilter.h"


#define LOCTEXT_NAMESPACE "PCGExUberFilterCollections"
#define PCGEX_NAMESPACE UberFilterCollections

TArray<FPCGPinProperties> UPCGExUberFilterCollectionsSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PCGEX_PIN_POINTS(PCGExPointFilter::OutputInsideFiltersLabel, "Collections that passed the filters.", Required, {})
	PCGEX_PIN_POINTS(PCGExPointFilter::OutputOutsideFiltersLabel, "Collections that didn't pass the filters.", Required, {})
	return PinProperties;
}

PCGExData::EInit UPCGExUberFilterCollectionsSettings::GetMainOutputInitMode() const { return PCGExData::EInit::NoOutput; }

FPCGExUberFilterCollectionsContext::~FPCGExUberFilterCollectionsContext()
{
	PCGEX_TERMINATE_ASYNC
}

PCGEX_INITIALIZE_ELEMENT(UberFilterCollections)

FName UPCGExUberFilterCollectionsSettings::GetMainOutputLabel() const
{
	// Ensure proper forward when node is disabled
	return PCGExPointFilter::OutputInsideFiltersLabel;
}

bool FPCGExUberFilterCollectionsElement::Boot(FPCGExContext* InContext) const
{
	if (!FPCGExPointsProcessorElement::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(UberFilterCollections)

	Context->Inside = MakeUnique<PCGExData::FPointIOCollection>(Context);
	Context->Outside = MakeUnique<PCGExData::FPointIOCollection>(Context);

	Context->Inside->DefaultOutputLabel = PCGExPointFilter::OutputInsideFiltersLabel;
	Context->Outside->DefaultOutputLabel = PCGExPointFilter::OutputOutsideFiltersLabel;

	if (Settings->bSwap)
	{
		Context->Inside->DefaultOutputLabel = PCGExPointFilter::OutputOutsideFiltersLabel;
		Context->Outside->DefaultOutputLabel = PCGExPointFilter::OutputInsideFiltersLabel;
	}

	return true;
}

bool FPCGExUberFilterCollectionsElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExUberFilterCollectionsElement::Execute);

	PCGEX_CONTEXT_AND_SETTINGS(UberFilterCollections)

	if (Context->IsSetup())
	{
		if (!Boot(Context)) { return true; }

		Context->NumPairs = Context->MainPoints->Pairs.Num();

		if (!Context->StartBatchProcessingPoints<PCGExPointsMT::TBatch<PCGExUberFilterCollections::FProcessor>>(
			[&](PCGExData::FPointIO* Entry) { return true; },
			[&](PCGExPointsMT::TBatch<PCGExUberFilterCollections::FProcessor>* NewBatch)
			{
			},
			PCGExMT::State_Done))
		{
			PCGE_LOG(Error, GraphAndLog, FTEXT("Could not find any points to filter."));
			return true;
		}
	}

	if (!Context->ProcessPointsBatch()) { return false; }

	Context->MainBatch->Output();

	Context->Inside->OutputToContext();
	Context->Outside->OutputToContext();

	return Context->TryComplete();
}

namespace PCGExUberFilterCollections
{
	FProcessor::~FProcessor()
	{
	}

	bool FProcessor::Process(TSharedPtr<PCGExMT::FTaskManager> InAsyncManager)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PCGExUberFilterCollections::Process);


		// Must be set before process for filters
		PointDataFacade->bSupportsScopedGet = Context->bScopedAttributeGet;

		if (!FPointsProcessor::Process(InAsyncManager)) { return false; }

		NumPoints = PointIO->GetNum();

		if (Settings->Measure == EPCGExMeanMeasure::Discrete)
		{
			if ((Settings->Comparison == EPCGExComparison::StrictlyGreater ||
					Settings->Comparison == EPCGExComparison::EqualOrGreater) &&
				NumPoints < Settings->IntThreshold)
			{
				// Not enough points to meet requirements.
				Context->Outside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward);
				return true;
			}
		}

		StartParallelLoopForPoints(PCGExData::ESource::In);

		return true;
	}

	void FProcessor::PrepareSingleLoopScopeForPoints(const uint32 StartIndex, const int32 Count)
	{
		PointDataFacade->Fetch(StartIndex, Count);
		FilterScope(StartIndex, Count);
	}

	void FProcessor::ProcessSinglePoint(const int32 Index, FPCGPoint& Point, const int32 LoopIdx, const int32 LoopCount)
	{
		if (PointFilterCache[Index]) { FPlatformAtomics::InterlockedAdd(&NumInside, 1); }
		else { FPlatformAtomics::InterlockedAdd(&NumOutside, 1); }
	}

	void FProcessor::Output()
	{
		FPointsProcessor::Output();

		switch (Settings->Mode)
		{
		default:
		case EPCGExUberFilterCollectionsMode::All:
			if (NumInside == NumPoints) { Context->Inside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
			else { Context->Outside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
			break;
		case EPCGExUberFilterCollectionsMode::Any:
			if (NumInside != 0) { Context->Inside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
			else { Context->Outside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
			break;
		case EPCGExUberFilterCollectionsMode::Partial:
			if (Settings->Measure == EPCGExMeanMeasure::Discrete)
			{
				if (PCGExCompare::Compare(Settings->Comparison, NumInside, Settings->IntThreshold, 0)) { Context->Inside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
				else { Context->Outside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
			}
			else
			{
				const double Ratio = static_cast<double>(NumInside) / static_cast<double>(NumPoints);
				if (PCGExCompare::Compare(Settings->Comparison, Ratio, Settings->DblThreshold, Settings->Tolerance)) { Context->Inside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
				else { Context->Outside->Emplace_GetRef(PointIO, PCGExData::EInit::Forward); }
			}
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
