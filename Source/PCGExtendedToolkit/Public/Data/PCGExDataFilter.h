// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PCGExData.h"
#include "PCGExDataCaching.h"
#include "PCGExFactoryProvider.h"

#include "PCGExDataFilter.generated.h"

namespace PCGExDataFilter
{
	class TFilter;
}

namespace PCGExDataFilter
{
	enum class EType : uint8
	{
		Default = 0,
		ClusterNode,
		ClusterEdge,
	};
}

/**
 * 
 */
UCLASS(Abstract, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Data")
class PCGEXTENDEDTOOLKIT_API UPCGExFilterFactoryBase : public UPCGExParamFactoryBase
{
	GENERATED_BODY()

public:
	FORCEINLINE virtual PCGExFactories::EType GetFactoryType() const override;

	virtual void Init();

	int32 Priority = 0;
	virtual PCGExDataFilter::TFilter* CreateFilter() const;
};

namespace PCGExDataFilter
{
	PCGEX_ASYNC_STATE(State_FilteringPoints)

	const FName OutputFilterLabel = TEXT("Filter");
	const FName SourceFiltersLabel = TEXT("Filters");
	const FName OutputInsideFiltersLabel = TEXT("Inside");
	const FName OutputOutsideFiltersLabel = TEXT("Outside");

	class PCGEXTENDEDTOOLKIT_API TFilter
	{
	public:
		explicit TFilter(const UPCGExFilterFactoryBase* InFactory):
			Factory(InFactory)
		{
		}

		PCGExDataCaching::FPool* PointDataCache = nullptr;

		bool bCacheResults = true;
		const UPCGExFilterFactoryBase* Factory;
		TArray<bool> Results;

		int32 Index = 0;
		bool bValid = true;

		FORCEINLINE virtual EType GetFilterType() const;

		virtual void Capture(const FPCGContext* InContext, PCGExDataCaching::FPool* InPrimaryDataCache);

		virtual void PrepareForTesting();
		virtual void PrepareForTesting(const TArrayView<const int32>& PointIndices);

		FORCEINLINE virtual bool Test(const int32 PointIndex) const = 0;

		virtual ~TFilter()
		{
			Results.Empty();
		}
	};

	class PCGEXTENDEDTOOLKIT_API TFilterManager
	{
	public:
		explicit TFilterManager(PCGExDataCaching::FPool* InPrimaryDataCache);

		TArray<TFilter*> Handlers;

		bool bCacheResults = true;
		bool bValid = false;

		PCGExDataCaching::FPool* PointDataCache = nullptr;

		template <typename T_DEF>
		void Register(const FPCGContext* InContext, const TArray<T_DEF*>& InFactories)
		{
			RegisterAndCapture(InContext, InFactories, [&](TFilter* Handler) { Handler->Capture(InContext, PointDataCache); });
		}

		template <typename T_DEF, class CaptureFunc>
		void RegisterAndCapture(const FPCGContext* InContext, const TArray<T_DEF*>& InFactories, CaptureFunc&& InCaptureFn)
		{
			for (T_DEF* Factory : InFactories)
			{
				TFilter* Handler = Factory->CreateFilter();
				Handler->bCacheResults = bCacheResults;

				InCaptureFn(Handler);

				if (!Handler->bValid)
				{
					delete Handler;
					continue;
				}

				Handlers.Add(Handler);
			}

			bValid = !Handlers.IsEmpty();

			if (!bValid) { return; }

			// Sort mappings so higher priorities come last, as they have to potential to override values.
			Handlers.Sort([&](const TFilter& A, const TFilter& B) { return A.Factory->Priority < B.Factory->Priority; });

			// Update index & partials
			for (int i = 0; i < Handlers.Num(); i++)
			{
				Handlers[i]->Index = i;
				PostProcessHandler(Handlers[i]);
			}
		}

		virtual void PrepareForTesting();
		virtual void PrepareForTesting(const TArrayView<const int32>& PointIndices);

		virtual bool Test(const int32 PointIndex);

		virtual ~TFilterManager()
		{
			PCGEX_DELETE_TARRAY(Handlers)
		}

	protected:
		virtual void PostProcessHandler(TFilter* Handler);
	};

	class PCGEXTENDEDTOOLKIT_API TEarlyExitFilterManager final : public TFilterManager
	{
	public:
		explicit TEarlyExitFilterManager(PCGExDataCaching::FPool* InPrimaryDataCache);

		TArray<bool> Results;

		virtual bool Test(const int32 PointIndex) override;

		virtual void PrepareForTesting() override;
		virtual void PrepareForTesting(const TArrayView<const int32>& PointIndices) override;
	};
}
