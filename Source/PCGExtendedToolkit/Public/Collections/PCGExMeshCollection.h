// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExAssetCollection.h"
#include "Engine/DataAsset.h"
#include "ISMPartition/ISMComponentDescriptor.h"
#include "MeshSelectors/PCGMeshSelectorBase.h"

#include "PCGExMeshCollection.generated.h"

class UPCGExMeshCollection;

USTRUCT(BlueprintType, DisplayName="[PCGEx] Mesh Collection Entry")
struct /*PCGEXTENDEDTOOLKIT_API*/ FPCGExMeshCollectionEntry : public FPCGExAssetCollectionEntry
{
	GENERATED_BODY()

	FPCGExMeshCollectionEntry()
	{
	}

	UPROPERTY(EditAnywhere, Category = Settings, meta=(EditCondition="!bIsSubCollection", EditConditionHides))
	FSoftISMComponentDescriptor Descriptor;

	UPROPERTY(EditAnywhere, Category = Settings, meta=(EditCondition="bIsSubCollection", EditConditionHides))
	TSoftObjectPtr<UPCGExMeshCollection> SubCollection;

	TObjectPtr<UPCGExMeshCollection> SubCollectionPtr;

	bool Matches(const FPCGMeshInstanceList& InstanceList) const
	{
		// TODO : This is way too weak
		return InstanceList.Descriptor.StaticMesh == Descriptor.StaticMesh;
	}

	bool SameAs(const FPCGExMeshCollectionEntry& Other) const
	{
		return
			SubCollectionPtr == Other.SubCollectionPtr &&
			Weight == Other.Weight &&
			Category == Other.Category &&
			Descriptor.StaticMesh == Other.Descriptor.StaticMesh;
	}

	virtual bool Validate(const UPCGExAssetCollection* ParentCollection) override;
	virtual void UpdateStaging(const UPCGExAssetCollection* OwningCollection, const bool bRecursive) override;
	virtual void SetAssetPath(const FSoftObjectPath& InPath) override;

protected:
	virtual void OnSubCollectionLoaded() override;
};

UCLASS(BlueprintType, DisplayName="[PCGEx] Mesh Collection")
class /*PCGEXTENDEDTOOLKIT_API*/ UPCGExMeshCollection : public UPCGExAssetCollection
{
	GENERATED_BODY()

	friend struct FPCGExMeshCollectionEntry;
	friend class UPCGExMeshSelectorBase;

public:
	virtual void RebuildStagingData(const bool bRecursive) override;

#if WITH_EDITOR
	virtual void EDITOR_RefreshDisplayNames() override;
#endif

	PCGEX_ASSET_COLLECTION_BOILERPLATE(UPCGExMeshCollection)
	
	virtual void GetAssetPaths(TSet<FSoftObjectPath>& OutPaths, const PCGExAssetCollection::ELoadingFlags Flags) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta=(TitleProperty="DisplayName"))
	TArray<FPCGExMeshCollectionEntry> Entries;
};
