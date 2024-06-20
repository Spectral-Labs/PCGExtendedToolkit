﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/Edges/Extras/PCGExVtxExtraFactoryProvider.h"

#define LOCTEXT_NAMESPACE "PCGExWriteVtxExtras"
#define PCGEX_NAMESPACE PCGExWriteVtxExtras


void UPCGExVtxExtraOperation::CopySettingsFrom(const UPCGExOperation* Other)
{
	Super::CopySettingsFrom(Other);
	const UPCGExVtxExtraOperation* TypedOther = Cast<UPCGExVtxExtraOperation>(Other);
	if (TypedOther)
	{
	}
}

bool UPCGExVtxExtraOperation::PrepareForCluster(const FPCGContext* InContext, PCGExCluster::FCluster* InCluster)
{
	Cluster = InCluster;

	if (!PCGEx::IsValidName(OutputAttribute))
	{
		bIsValidOperation = false;
		PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Invalid user-defined attribute name for " "OutputAttribute"));
		return false;
	};

	return true;
}

bool UPCGExVtxExtraOperation::IsOperationValid() { return bIsValidOperation; }

void UPCGExVtxExtraOperation::ProcessNode(PCGExCluster::FNode& Node, const TArray<PCGExCluster::FAdjacencyData>& Adjacency)
{
}

void UPCGExVtxExtraOperation::Write()
{
}

void UPCGExVtxExtraOperation::Write(const TArrayView<int32> Indices)
{
}

void UPCGExVtxExtraOperation::Cleanup() { Super::Cleanup(); }

#if WITH_EDITOR
FString UPCGExVtxExtraProviderSettings::GetDisplayName() const { return TEXT(""); }
#endif

PCGExFactories::EType UPCGExVtxExtraFactoryBase::GetFactoryType() const { return PCGExFactories::EType::VtxExtra; }

UPCGExVtxExtraOperation* UPCGExVtxExtraFactoryBase::CreateOperation() const
{
	UPCGExVtxExtraOperation* NewOperation = NewObject<UPCGExVtxExtraOperation>();
	return NewOperation;
}

TArray<FPCGPinProperties> UPCGExVtxExtraProviderSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::InputPinProperties();
	//PCGEX_PIN_PARAMS(PCGEx::SourcePointFilters, "Filters used to check which node will be processed by the sampler or not.", Advanced, {})
	//PCGEX_PIN_PARAMS(PCGEx::SourceUseValueIfFilters, "Filters used to check if a node can be used as a value source or not.", Advanced, {})
	return PinProperties;
}

FName UPCGExVtxExtraProviderSettings::GetMainOutputLabel() const { return PCGExVtxExtra::OutputExtraLabel; }

UPCGExParamFactoryBase* UPCGExVtxExtraProviderSettings::CreateFactory(FPCGContext* InContext, UPCGExParamFactoryBase* InFactory) const
{
	UPCGExVtxExtraFactoryBase* SamplerFactory = Cast<UPCGExVtxExtraFactoryBase>(InFactory);
	SamplerFactory->Priority = Priority;
	return InFactory;
}


#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
