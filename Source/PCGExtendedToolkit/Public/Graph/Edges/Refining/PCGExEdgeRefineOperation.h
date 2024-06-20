﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExOperation.h"
#include "PCGExEdgeRefineOperation.generated.h"

namespace PCGExHeuristics
{
	class THeuristicsHandler;
}

namespace PCGExGraph
{
	class FGraph;
}

namespace PCGExCluster
{
	struct FCluster;
}

/**
 * 
 */
UCLASS(Abstract)
class PCGEXTENDEDTOOLKIT_API UPCGExEdgeRefineOperation : public UPCGExOperation
{
	GENERATED_BODY()

public:
	virtual bool RequiresHeuristics();
	virtual void Process(PCGExCluster::FCluster* InCluster, PCGExHeuristics::THeuristicsHandler* InHeuristics = nullptr);
	virtual void Cleanup() override;
};
