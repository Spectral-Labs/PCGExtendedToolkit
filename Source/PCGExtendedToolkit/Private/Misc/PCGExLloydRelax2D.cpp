﻿// Copyright 2024 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#include "Misc/PCGExLloydRelax2D.h"


#include "Geometry/PCGExGeoDelaunay.h"

#define LOCTEXT_NAMESPACE "PCGExLloydRelax2DElement"
#define PCGEX_NAMESPACE LloydRelax2D

PCGExData::EIOInit UPCGExLloydRelax2DSettings::GetMainOutputInitMode() const { return PCGExData::EIOInit::None; }

PCGEX_INITIALIZE_ELEMENT(LloydRelax2D)

bool FPCGExLloydRelax2DElement::Boot(FPCGExContext* InContext) const
{
	if (!FPCGExPointsProcessorElement::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(LloydRelax2D)

	return true;
}

bool FPCGExLloydRelax2DElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExLloydRelax2DElement::Execute);

	PCGEX_CONTEXT_AND_SETTINGS(LloydRelax2D)
	PCGEX_EXECUTION_CHECK

	PCGEX_ON_INITIAL_EXECUTION
	{
		PCGEX_ON_INVALILD_INPUTS(FTEXT("Some inputs have less than 3 points and won't be processed."))

		if (!Context->StartBatchProcessingPoints<PCGExPointsMT::TBatch<PCGExLloydRelax2D::FProcessor>>(
			[&](const TSharedPtr<PCGExData::FPointIO>& Entry)
			{
				if (Entry->GetNum() <= 3)
				{
					Entry->InitializeOutput(PCGExData::EIOInit::Forward);
					bHasInvalidInputs = true;
					return false;
				}
				return true;
			},
			[&](const TSharedPtr<PCGExPointsMT::TBatch<PCGExLloydRelax2D::FProcessor>>& NewBatch)
			{
			}))
		{
			Context->CancelExecution(TEXT("Could not find any paths to relax."));
		}
	}

	PCGEX_POINTS_BATCH_PROCESSING(PCGEx::State_Done)

	Context->MainPoints->StageOutputs();

	return Context->TryComplete();
}

namespace PCGExLloydRelax2D
{
	bool FProcessor::Process(const TSharedPtr<PCGExMT::FTaskManager> InAsyncManager)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PCGExLloydRelax2D::Process);

		if (!FPointsProcessor::Process(InAsyncManager)) { return false; }

		PCGEX_INIT_IO(PointDataFacade->Source, PCGExData::EIOInit::Duplicate)

		ProjectionDetails = Settings->ProjectionDetails;
		ProjectionDetails.Init(ExecutionContext, PointDataFacade);

		InfluenceDetails = Settings->InfluenceDetails;
		if (!InfluenceDetails.Init(ExecutionContext, PointDataFacade)) { return false; }

		PCGExGeo::PointsToPositions(PointDataFacade->GetIn()->GetPoints(), ActivePositions);

		PCGEX_SHARED_THIS_DECL
		PCGEX_LAUNCH(FLloydRelaxTask, 0, ThisPtr, &InfluenceDetails, Settings->Iterations)

		return true;
	}

	void FProcessor::ProcessSinglePoint(const int32 Index, FPCGPoint& Point, const PCGExMT::FScope& Scope)
	{
		FVector TargetPosition = Point.Transform.GetLocation();
		TargetPosition.X = ActivePositions[Index].X;
		TargetPosition.Y = ActivePositions[Index].Y;

		Point.Transform.SetLocation(
			InfluenceDetails.bProgressiveInfluence ?
				TargetPosition :
				FMath::Lerp(Point.Transform.GetLocation(), TargetPosition, InfluenceDetails.GetInfluence(Index)));
	}

	void FProcessor::CompleteWork()
	{
		StartParallelLoopForPoints();
	}

	void FLloydRelaxTask::ExecuteTask(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager)
	{
		NumIterations--;

		TUniquePtr<PCGExGeo::TDelaunay2> Delaunay = MakeUnique<PCGExGeo::TDelaunay2>();
		TArray<FVector>& Positions = Processor->ActivePositions;

		//FPCGExPointsProcessorContext* Context = static_cast<FPCGExPointsProcessorContext*>(Manager->Context);

		const TArrayView<FVector> View = MakeArrayView(Positions);
		if (!Delaunay->Process(View, Processor->ProjectionDetails)) { return; }

		const int32 NumPoints = Positions.Num();

		TArray<FVector> Sum;
		TArray<double> Counts;
		Sum.Append(Processor->ActivePositions);
		Counts.SetNum(NumPoints);
		for (int i = 0; i < NumPoints; i++) { Counts[i] = 1; }

		FVector Centroid;
		for (const PCGExGeo::FDelaunaySite2& Site : Delaunay->Sites)
		{
			PCGExGeo::GetCentroid(Positions, Site.Vtx, Centroid);
			for (const int32 PtIndex : Site.Vtx)
			{
				Counts[PtIndex] += 1;
				Sum[PtIndex] += Centroid;
			}
		}

		if (InfluenceSettings->bProgressiveInfluence)
		{
			for (int i = 0; i < NumPoints; i++)
			{
				Positions[i] = FMath::Lerp(Positions[i], Sum[i] / Counts[i], InfluenceSettings->GetInfluence(i));
			}
		}

		Delaunay.Reset();

		if (NumIterations > 0)
		{
			PCGEX_LAUNCH_INTERNAL(FLloydRelaxTask, TaskIndex + 1, Processor, InfluenceSettings, NumIterations)
		}
	}
}

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
