﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExCompare.h"
#include "UObject/Object.h"

#include "PCGExFactoryProvider.h"
#include "PCGExVtxExtraFactoryProvider.h"
#include "Graph/PCGExCluster.h"
#include "Graph/PCGExGraph.h"

#include "PCGExVtxExtraEdgeMatch.generated.h"

///

class UPCGExFilterFactoryBase;

namespace PCGExPointFilter
{
	class TManager;
}

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExEdgeMatchSettings
{
	GENERATED_BODY()

	/** Direction orientation */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExAdjacencyDirectionOrigin Origin = EPCGExAdjacencyDirectionOrigin::FromNode;
	
	/** Where to read the compared direction from. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExFetchType DirectionSource = EPCGExFetchType::Constant;

	/** Operand B for testing -- Will be translated to `double` under the hood. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="DirectionSource==EPCGExFetchType::Attribute", EditConditionHides))
	FPCGAttributePropertyInputSelector Direction;

	/** Direction for computing the dot product against the edge's. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="DirectionSource==EPCGExFetchType::Constant", EditConditionHides))
	FVector DirectionConstant = FVector::ForwardVector;

	/** Whether to transform the direction source by the vtx' transform */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool bTransformDirection = true;

	/** Dot comparison settings */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FPCGExDotComparisonSettings DotComparisonSettings;
	
	/** Matching edge. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, DisplayName="Output"))
	FPCGExEdgeOutputWithIndexSettings MatchingEdge = FPCGExEdgeOutputWithIndexSettings(TEXT("Matching"));

};

/**
 * 
 */
UCLASS()
class PCGEXTENDEDTOOLKIT_API UPCGExVtxExtraEdgeMatch : public UPCGExVtxExtraOperation
{
	GENERATED_BODY()

public:
	FPCGExEdgeMatchSettings Descriptor;
	TArray<UPCGExFilterFactoryBase*>* FilterFactories = nullptr;

	virtual void CopySettingsFrom(const UPCGExOperation* Other) override;
	virtual void ClusterReserve(const int32 NumClusters) override;
	virtual void PrepareForCluster(const FPCGContext* InContext, const int32 ClusterIdx, PCGExCluster::FCluster* Cluster, PCGExData::FPool* VtxDataCache, PCGExData::FPool* EdgeDataCache) override;
	virtual bool PrepareForVtx(const FPCGContext* InContext, PCGExData::FPointIO* InVtx, PCGExData::FPool* VtxDataCache) override;
	virtual void ProcessNode(const int32 ClusterIdx, const PCGExCluster::FCluster* Cluster, PCGExCluster::FNode& Node, const TArray<PCGExCluster::FAdjacencyData>& Adjacency) override;
	virtual void Write() override;
	virtual void Write(PCGExMT::FTaskManager* AsyncManager) override;
	virtual void Cleanup() override;

protected:
	bool bEdgeFilterInitialized = false;
	void InitEdgeFilters();

	mutable FRWLock FilterLock;

	PCGExData::FCache<FVector>* DirCache = nullptr;
	TArray<PCGExPointFilter::TManager*> FilterManagers;
};

UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Data")
class PCGEXTENDEDTOOLKIT_API UPCGExVtxExtraEdgeMatchFactory : public UPCGExVtxExtraFactoryBase
{
	GENERATED_BODY()

public:
	FPCGExEdgeMatchSettings Descriptor;
	TArray<UPCGExFilterFactoryBase*> FilterFactories;
	virtual UPCGExVtxExtraOperation* CreateOperation() const override;
};

UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|VtxExtra")
class PCGEXTENDEDTOOLKIT_API UPCGExVtxExtraEdgeMatchSettings : public UPCGExVtxExtraProviderSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS_CUSTOM_SUBTITLE(
		NeighborSamplerAttribute, "Vtx Extra : Edge Match", "Find the edge that matches the closest provided direction.",
		FName(GetDisplayName()))

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
#endif
	//~End UPCGSettings

public:
	virtual UPCGExParamFactoryBase* CreateFactory(FPCGContext* InContext, UPCGExParamFactoryBase* InFactory) const override;

#if WITH_EDITOR
	virtual FString GetDisplayName() const override;
#endif

	/** Direction Settings. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, ShowOnlyInnerProperties))
	FPCGExEdgeMatchSettings Descriptor;
};
