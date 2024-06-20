﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/PCGExCompoundHelpers.h"

namespace PCGExGraph
{
	// Need :
	// - GraphBuilder
	// - CompoundGraph
	// - CompoundPoints

	// - PointEdgeIntersectionSettings


	FCompoundProcessor::FCompoundProcessor(
		FPCGExPointsProcessorContext* InContext,
		FPCGExPointPointIntersectionSettings InPointPointIntersectionSettings,
		FPCGExBlendingSettings InDefaultPointsBlending,
		FPCGExBlendingSettings InDefaultEdgesBlending):
		Context(InContext),
		PointPointIntersectionSettings(InPointPointIntersectionSettings),
		DefaultPointsBlendingSettings(InDefaultPointsBlending),
		DefaultEdgesBlendingSettings(InDefaultEdgesBlending)
	{
	}

	FCompoundProcessor::~FCompoundProcessor()
	{
		PCGEX_DELETE(GraphBuilder)
		PCGEX_DELETE(PointEdgeIntersections)
		PCGEX_DELETE(EdgeEdgeIntersections)
		PCGEX_DELETE(MetadataBlender)
	}

	void FCompoundProcessor::InitPointEdge(
		const FPCGExPointEdgeIntersectionSettings& InSettings,
		const bool bUseCustom,
		const FPCGExBlendingSettings* InOverride)
	{
		bDoPointEdge = true;
		PointEdgeIntersectionSettings = InSettings;
		bUseCustomPointEdgeBlending = bUseCustom;
		if (InOverride) { CustomPointEdgeBlendingSettings = *InOverride; }
	}

	void FCompoundProcessor::InitEdgeEdge(
		const FPCGExEdgeEdgeIntersectionSettings& InSettings,
		const bool bUseCustom,
		const FPCGExBlendingSettings* InOverride)
	{
		bDoEdgeEdge = true;
		EdgeEdgeIntersectionSettings = InSettings;
		bUseCustomEdgeEdgeBlending = bUseCustom;
		if (InOverride) { CustomEdgeEdgeBlendingSettings = *InOverride; }
	}

	bool FCompoundProcessor::Execute()
	{
		if (!bRunning) { return false; }

		if (Context->IsState(State_FindingPointEdgeIntersections))
		{
			auto PointEdge = [&](const int32 EdgeIndex)
			{
				const FIndexedEdge& Edge = GraphBuilder->Graph->Edges[EdgeIndex];
				if (!Edge.bValid) { return; }
				FindCollinearNodes(PointEdgeIntersections, EdgeIndex, CompoundPoints->GetOut());
			};

			if (!Context->Process(PointEdge, GraphBuilder->Graph->Edges.Num())) { return false; }

			PointEdgeIntersections->Insert(); // TODO : Async?
			CompoundPoints->CleanupKeys();    // Required for later blending as point count has changed

			Context->SetState(State_BlendingPointEdgeCrossings);
		}

		if (Context->IsState(State_BlendingPointEdgeCrossings))
		{
			auto Initialize = [&]()
			{
				if (bUseCustomPointEdgeBlending) { MetadataBlender = new PCGExDataBlending::FMetadataBlender(&CustomPointEdgeBlendingSettings); }
				else { MetadataBlender = new PCGExDataBlending::FMetadataBlender(&DefaultPointsBlendingSettings); }

				MetadataBlender->PrepareForData(*CompoundPoints, PCGExData::ESource::Out, true);
			};

			auto BlendPointEdgeMetadata = [&](const int32 Index)
			{
				// TODO
			};

			if (!Context->Process(Initialize, BlendPointEdgeMetadata, PointEdgeIntersections->Edges.Num())) { return false; }

			if (MetadataBlender) { MetadataBlender->Write(); }

			PCGEX_DELETE(PointEdgeIntersections)
			PCGEX_DELETE(MetadataBlender)

			if (bDoEdgeEdge) { FindEdgeEdgeIntersections(); }
			else { Context->SetState(State_WritingClusters); }
		}

		if (Context->IsState(State_FindingEdgeEdgeIntersections))
		{
			auto EdgeEdge = [&](const int32 EdgeIndex)
			{
				const FIndexedEdge& Edge = GraphBuilder->Graph->Edges[EdgeIndex];
				if (!Edge.bValid) { return; }
				FindOverlappingEdges(EdgeEdgeIntersections, EdgeIndex);
			};

			if (!Context->Process(EdgeEdge, GraphBuilder->Graph->Edges.Num())) { return false; }

			EdgeEdgeIntersections->Insert(); // TODO : Async?
			CompoundPoints->CleanupKeys();   // Required for later blending as point count has changed

			Context->SetState(State_BlendingEdgeEdgeCrossings);
		}

		if (Context->IsState(State_BlendingEdgeEdgeCrossings))
		{
			auto Initialize = [&]()
			{
				if (bUseCustomPointEdgeBlending) { MetadataBlender = new PCGExDataBlending::FMetadataBlender(&CustomEdgeEdgeBlendingSettings); }
				else { MetadataBlender = new PCGExDataBlending::FMetadataBlender(&DefaultPointsBlendingSettings); }

				MetadataBlender->PrepareForData(*CompoundPoints, PCGExData::ESource::Out, true);
			};

			auto BlendCrossingMetadata = [&](const int32 Index) { EdgeEdgeIntersections->BlendIntersection(Index, MetadataBlender); };

			if (!Context->Process(Initialize, BlendCrossingMetadata, EdgeEdgeIntersections->Crossings.Num())) { return false; }

			if (MetadataBlender) { MetadataBlender->Write(); }

			PCGEX_DELETE(EdgeEdgeIntersections)
			PCGEX_DELETE(MetadataBlender)

			Context->SetState(State_WritingClusters);
		}

		if (Context->IsState(State_WritingClusters))
		{
			GraphBuilder->CompileAsync(Context->GetAsyncManager(), &GraphMetadataSettings);
			Context->SetAsyncState(State_Compiling);
			return false;
		}

		if (Context->IsState(State_Compiling))
		{
			PCGEX_WAIT_ASYNC

			if (!GraphBuilder->bCompiledSuccessfully)
			{
				CompoundPoints->InitializeOutput(PCGExData::EInit::NoOutput);
				return true;
			}

			GraphBuilder->Write(Context);
			return true;
		}

		if (Context->IsState(State_MergingEdgeCompounds))
		{
			//TODO	
		}

		return true;
	}

	bool FCompoundProcessor::IsRunning() const { return bRunning; }

	void FCompoundProcessor::FindPointEdgeIntersections()
	{
		PointEdgeIntersections = new FPointEdgeIntersections(
			GraphBuilder->Graph, CompoundGraph, CompoundPoints, PointEdgeIntersectionSettings);

		Context->SetState(State_FindingPointEdgeIntersections);
	}

	void FCompoundProcessor::FindEdgeEdgeIntersections()
	{
		EdgeEdgeIntersections = new FEdgeEdgeIntersections(
			GraphBuilder->Graph, CompoundGraph, CompoundPoints, EdgeEdgeIntersectionSettings);

		Context->SetState(State_FindingEdgeEdgeIntersections);
	}
}
