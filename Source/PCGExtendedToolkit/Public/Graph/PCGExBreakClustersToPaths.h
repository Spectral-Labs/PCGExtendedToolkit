﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "Geometry/PCGExGeo.h"
#include "Graph/PCGExEdgesProcessor.h"

#include "PCGExBreakClustersToPaths.generated.h"

UENUM(BlueprintType, meta=(DisplayName="[PCGEx] Break Cluster Operation Target"))
enum class EPCGExBreakClusterOperationTarget : uint8
{
	Paths UMETA(DisplayName = "Paths", ToolTip="Operate on edge chains which form paths with no crossings. \n e.g, nodes with only two neighbors."),
	Edges UMETA(DisplayName = "Edges", ToolTip="Operate on each edge individually (very expensive)"),
};

UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Edges")
class PCGEXTENDEDTOOLKIT_API UPCGExBreakClustersToPathsSettings : public UPCGExEdgesProcessorSettings
{
	GENERATED_BODY()

public:
	UPCGExBreakClustersToPathsSettings(const FObjectInitializer& ObjectInitializer);

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(BreakClustersToPaths, "Pathfinding : Find Contours", "Attempts to find a closed contour of connected edges around seed points.");
	virtual FLinearColor GetNodeTitleColor() const override { return GetDefault<UPCGExEditorSettings>()->NodeColorPathfinding; }
#endif

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	//~Begin UPCGExPointsProcessorSettings interface
public:
	virtual PCGExData::EInit GetMainOutputInitMode() const override;
	//~End UPCGExPointsProcessorSettings interface

	virtual PCGExData::EInit GetEdgeOutputInitMode() const override;

	/** Operation target mode */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExBreakClusterOperationTarget OperateOn;

private:
	friend class FPCGExBreakClustersToPathsElement;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExBreakClustersToPathsContext : public FPCGExEdgesProcessorContext
{
	friend class FPCGExBreakClustersToPathsElement;

	virtual ~FPCGExBreakClustersToPathsContext() override;

	TArray<UPCGExFilterFactoryBase*> PointFilterFactories;
	PCGExDataFilter::TEarlyExitFilterManager* PointFilterManager = nullptr;

	PCGExData::FPointIOCollection* Paths = nullptr;
	
	TArray<PCGExCluster::FNodeChain*> Chains;
	
};

class PCGEXTENDEDTOOLKIT_API FPCGExBreakClustersToPathsElement : public FPCGExEdgesProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
};

class PCGEXTENDEDTOOLKIT_API FPCGExBreakClusterTask : public FPCGExNonAbandonableTask
{
public:
	FPCGExBreakClusterTask(FPCGExAsyncManager* InManager, const int32 InTaskIndex, PCGExData::FPointIO* InPointIO,
	                      PCGExCluster::FCluster* InCluster) :
		FPCGExNonAbandonableTask(InManager, InTaskIndex, InPointIO),
		Cluster(InCluster)
	{
	}

	PCGExCluster::FCluster* Cluster = nullptr;

	virtual bool ExecuteTask() override;
};
