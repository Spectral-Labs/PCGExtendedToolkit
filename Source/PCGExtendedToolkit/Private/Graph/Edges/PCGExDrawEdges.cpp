﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/Edges/PCGExDrawEdges.h"

#include "Data/Blending/PCGExMetadataBlender.h"
#include "Graph/Edges/Promoting/PCGExEdgePromoteToPoint.h"

#define LOCTEXT_NAMESPACE "PCGExEdgesToPaths"
#define PCGEX_NAMESPACE DrawEdges

UPCGExDrawEdgesSettings::UPCGExDrawEdgesSettings(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

PCGExData::EInit UPCGExDrawEdgesSettings::GetMainOutputInitMode() const { return PCGExData::EInit::NoOutput; }
PCGExData::EInit UPCGExDrawEdgesSettings::GetEdgeOutputInitMode() const { return PCGExData::EInit::NoOutput; }


PCGEX_INITIALIZE_ELEMENT(DrawEdges)

FPCGExDrawEdgesContext::~FPCGExDrawEdgesContext()
{
	PCGEX_TERMINATE_ASYNC
}

bool FPCGExDrawEdgesElement::Boot(FPCGContext* InContext) const
{
	if (!FPCGExEdgesProcessorElement::Boot(InContext)) { return false; }

#if WITH_EDITOR

	PCGEX_CONTEXT_AND_SETTINGS(DrawEdges)

	if (!Settings->PCGExDebug) { return false; }

#endif

	return true;
}

bool FPCGExDrawEdgesElement::ExecuteInternal(
	FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExDrawEdgesElement::Execute);

#if WITH_EDITOR

	PCGEX_CONTEXT_AND_SETTINGS(DrawEdges)

	if (Context->IsSetup())
	{
		if (!Boot(Context))
		{
			Context->OutputPointsAndEdges();
			return true;
		}
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsState(PCGExMT::State_ReadyForNextPoints))
	{
		if (!Context->AdvancePointsIO()) { Context->Done(); }
		else
		{
			if (!Context->TaggedEdges)
			{
				PCGE_LOG(Warning, GraphAndLog, FTEXT("Some input points have no bound edges."));
				Context->SetState(PCGExMT::State_ReadyForNextPoints);
			}
			else
			{
				Context->SetState(PCGExGraph::State_ReadyForNextEdges);
			}
		}
	}

	if (Context->IsState(PCGExGraph::State_ReadyForNextEdges))
	{
		while (Context->AdvanceEdges(true))
		{
			if (!Context->CurrentCluster) { continue; }
			for (const PCGExGraph::FIndexedEdge& Edge : Context->CurrentCluster->Edges)
			{
				if (!Edge.bValid) { continue; }
				FVector Start = Context->CurrentCluster->GetNodeFromPointIndex(Edge.Start).Position;
				FVector End = Context->CurrentCluster->GetNodeFromPointIndex(Edge.End).Position;
				DrawDebugLine(Context->World, Start, End, Settings->Color, true, -1, Settings->DepthPriority, Settings->Thickness);
			}
		}

		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsDone()) { DisabledPassThroughData(Context); }

	return Context->IsDone();

#endif

	DisabledPassThroughData(Context);

	return true;
}

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
