﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/Pathfinding/Heuristics/PCGExHeuristics.h"

#include "Graph/Pathfinding/Heuristics/PCGExHeuristicDistance.h"
#include "Graph/Pathfinding/Heuristics/PCGExHeuristicFeedback.h"
#include "Graph/Pathfinding/Heuristics/PCGExHeuristicOperation.h"

namespace PCGExHeuristics
{
	double FLocalFeedbackHandler::GetGlobalScore(const PCGExCluster::FNode& From, const PCGExCluster::FNode& Seed, const PCGExCluster::FNode& Goal) const
	{
		double GScore = 0;
		for (const UPCGExHeuristicFeedback* Feedback : Feedbacks) { GScore += Feedback->GetGlobalScore(From, Seed, Goal); }
		return GScore;
	}

	double FLocalFeedbackHandler::GetEdgeScore(const PCGExCluster::FNode& From, const PCGExCluster::FNode& To, const PCGExGraph::FIndexedEdge& Edge, const PCGExCluster::FNode& Seed, const PCGExCluster::FNode& Goal) const
	{
		double EScore = 0;
		for (const UPCGExHeuristicFeedback* Feedback : Feedbacks) { EScore += Feedback->GetEdgeScore(From, To, Edge, Seed, Goal); }
		return EScore;
	}

	void FLocalFeedbackHandler::FeedbackPointScore(const PCGExCluster::FNode& Node)
	{
		for (UPCGExHeuristicFeedback* Feedback : Feedbacks) { Feedback->FeedbackPointScore(Node); }
	}

	void FLocalFeedbackHandler::FeedbackScore(const PCGExCluster::FNode& Node, const PCGExGraph::FIndexedEdge& Edge)
	{
		for (UPCGExHeuristicFeedback* Feedback : Feedbacks) { Feedback->FeedbackScore(Node, Edge); }
	}

	THeuristicsHandler::THeuristicsHandler(FPCGExPointsProcessorContext* InContext, double InReferenceWeight):
		ReferenceWeight(InReferenceWeight)
	{
		const TArray<FPCGTaggedData>& Inputs = InContext->InputData.GetInputsByPin(PCGExPathfinding::SourceHeuristicsLabel);

		for (const FPCGTaggedData& InputState : Inputs)
		{
			if (UPCGHeuristicsFactoryBase* OperationFactory = Cast<UPCGHeuristicsFactoryBase>(InputState.Data))
			{
				UPCGExHeuristicOperation* Operation = nullptr;

				if (const UPCGHeuristicsFactoryFeedback* FeedbackFactory = Cast<UPCGHeuristicsFactoryFeedback>(Operation))
				{
					if (FeedbackFactory->IsGlobal())
					{
						Operation = OperationFactory->CreateOperation();
						UPCGExHeuristicFeedback* Feedback = Cast<UPCGExHeuristicFeedback>(Operation);
						Feedbacks.Add(Feedback);
					}
					else
					{
						LocalFeedbackFactories.Add(OperationFactory);
					}
				}
				else
				{
					Operation = OperationFactory->CreateOperation();
				}

				Operations.Add(Operation);
				Operation->WeightFactor = OperationFactory->WeightFactor;
				Operation->ReferenceWeight = ReferenceWeight;
				InContext->RegisterOperation<UPCGExHeuristicOperation>(Operation);
			}
		}

		if (Operations.IsEmpty())
		{
			PCGE_LOG_C(Warning, GraphAndLog, InContext, FTEXT("Missing valid heuristics. Will use Shortest Distance as default."));
			UPCGExHeuristicDistance* DefaultHeuristics = NewObject<UPCGExHeuristicDistance>();
			DefaultHeuristics->ReferenceWeight = ReferenceWeight;
			Operations.Add(DefaultHeuristics);
		}
	}

	THeuristicsHandler::THeuristicsHandler(UPCGExHeuristicOperation* InSingleOperation)
	{
		Operations.Add(InSingleOperation);
	}

	THeuristicsHandler::~THeuristicsHandler()
	{

		for (UPCGExHeuristicOperation* Operation : Operations)
		{
			Operation->Cleanup();
			PCGEX_DELETE_UOBJECT(Operation)
		}

		Operations.Empty();
		Feedbacks.Empty();
	}

	bool THeuristicsHandler::PrepareForCluster(FPCGExAsyncManager* AsyncManager, PCGExCluster::FCluster* InCluster)
	{
		InCluster->ComputeEdgeLengths(true);

		CurrentCluster = InCluster;
		for (UPCGExHeuristicOperation* Operation : Operations) { Operation->PrepareForCluster(InCluster); }

		return true;
	}

	void THeuristicsHandler::CompleteClusterPreparation()
	{
		TotalWeight = 0;
		for (const UPCGExHeuristicOperation* Op : Operations) { TotalWeight += Op->WeightFactor; }
	}

	double THeuristicsHandler::GetGlobalScore(
		const PCGExCluster::FNode& From,
		const PCGExCluster::FNode& Seed,
		const PCGExCluster::FNode& Goal,
		const FLocalFeedbackHandler* LocalFeedback) const
	{
		double GScore = 0;
		for (const UPCGExHeuristicOperation* Op : Operations) { GScore += Op->GetGlobalScore(From, Seed, Goal); }
		if (LocalFeedback) { return (GScore + LocalFeedback->GetGlobalScore(From, Seed, Goal)) / (TotalWeight + LocalFeedback->TotalWeight); }
		return GScore / TotalWeight;
	}

	double THeuristicsHandler::GetEdgeScore(
		const PCGExCluster::FNode& From,
		const PCGExCluster::FNode& To,
		const PCGExGraph::FIndexedEdge& Edge,
		const PCGExCluster::FNode& Seed,
		const PCGExCluster::FNode& Goal,
		const FLocalFeedbackHandler* LocalFeedback) const
	{
		double EScore = 0;
		for (const UPCGExHeuristicOperation* Op : Operations) { EScore += Op->GetEdgeScore(From, To, Edge, Seed, Goal); }
		if (LocalFeedback) { return (EScore + LocalFeedback->GetEdgeScore(From, To, Edge, Seed, Goal)) / (TotalWeight + LocalFeedback->TotalWeight); }
		return EScore / TotalWeight;
	}

	void THeuristicsHandler::FeedbackPointScore(const PCGExCluster::FNode& Node)
	{
		for (UPCGExHeuristicFeedback* Op : Feedbacks) { Op->FeedbackPointScore(Node); }
	}

	void THeuristicsHandler::FeedbackScore(const PCGExCluster::FNode& Node, const PCGExGraph::FIndexedEdge& Edge)
	{
		for (UPCGExHeuristicFeedback* Op : Feedbacks) { Op->FeedbackScore(Node, Edge); }
	}

	FLocalFeedbackHandler* THeuristicsHandler::MakeLocalFeedbackHandler(PCGExCluster::FCluster* InCluster)
	{
		if (!LocalFeedbackFactories.IsEmpty()) { return nullptr; }
		FLocalFeedbackHandler* NewLocalFeedbackHandler = new FLocalFeedbackHandler();

		for (const UPCGHeuristicsFactoryBase* Factory : LocalFeedbackFactories)
		{
			UPCGExHeuristicFeedback* Feedback = Cast<UPCGExHeuristicFeedback>(Factory->CreateOperation());
			Feedback->ReferenceWeight = ReferenceWeight * Factory->WeightFactor;

			NewLocalFeedbackHandler->Feedbacks.Add(Feedback);
			NewLocalFeedbackHandler->TotalWeight += Factory->WeightFactor;

			Feedback->PrepareForCluster(InCluster);
		}

		return NewLocalFeedbackHandler;
	}
}
