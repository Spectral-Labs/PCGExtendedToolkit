﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Data/PCGExPointStates.h"

#include "Graph/PCGExCluster.h"

PCGExPointFilter::TFilter* UPCGExPointStateFactoryBase::CreateFilter() const
{
	return Super::CreateFilter();
}

void UPCGExPointStateFactoryBase::BeginDestroy()
{
	Super::BeginDestroy();
}

namespace PCGExPointStates
{
	FState::~FState()
	{
		PCGEX_DELETE(Manager)
	}

	bool FState::Init(const FPCGContext* InContext, PCGExData::FPool* InPointDataCache)
	{
		if (!TFilter::Init(InContext, InPointDataCache)) { return false; }

		Manager = new PCGExPointFilter::TManager(InPointDataCache);
		Manager->bCacheResults = true;
		return true;
	}

	bool FState::InitInternalManager(const FPCGContext* InContext, const TArray<UPCGExFilterFactoryBase*>& InFactories)
	{
		return Manager->Init(InContext, InFactories);
	}

	bool FState::Test(const int32 Index) const
	{
		const bool bResult = Manager->Test(Index);
		Manager->Results[Index] = bResult;
		return bResult;
	}

	void FState::ProcessFlags(const bool bSuccess, int64& InFlags) const
	{
		// TODO : Implement
	}

	FStateManager::FStateManager(TArray<int64>* InFlags, PCGExData::FPool* InPointDataCache)
		: TManager(InPointDataCache)
	{
		FlagsCache = InFlags;
	}

	FStateManager::~FStateManager()
	{
		States.Empty();
	}

	void FStateManager::PostInitFilter(const FPCGContext* InContext, PCGExPointFilter::TFilter* InFilter)
	{
		FState* State = static_cast<FState*>(InFilter);
		State->InitInternalManager(InContext, State->StateFactory->FilterFactories);

		TManager::PostInitFilter(InContext, InFilter);

		States.Add(State);
	}

	bool FStateManager::Test(const int32 Index)
	{
		int64& Flags = (*FlagsCache)[Index];
		for (const FState* State : States) { State->ProcessFlags(State->Test(Index), Flags); }
		return true;
	}
}


FName UPCGExPointStateFactoryProviderSettings::GetMainOutputLabel() const { return PCGExCluster::OutputNodeStateLabel; }

UPCGExParamFactoryBase* UPCGExPointStateFactoryProviderSettings::CreateFactory(FPCGContext* InContext, UPCGExParamFactoryBase* InFactory) const
{
	return nullptr;
}
