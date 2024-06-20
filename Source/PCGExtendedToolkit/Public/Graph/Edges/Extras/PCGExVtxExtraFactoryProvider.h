﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "PCGExFactoryProvider.h"
#include "Graph/PCGExCluster.h"
#include "Graph/PCGExGraph.h"
#include "PCGExOperation.h"

#include "PCGExVtxExtraFactoryProvider.generated.h"

#define PCGEX_VTX_EXTRA_CREATE \
	NewOperation->Descriptor = Descriptor;

namespace PCGExVtxExtra
{
	const FName SourceExtrasLabel = TEXT("Extras");
	const FName OutputExtraLabel = TEXT("Extra");
}

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExSingleEdgeOutputSettings
{
	GENERATED_BODY()

	FPCGExSingleEdgeOutputSettings()
	{
	}

	explicit FPCGExSingleEdgeOutputSettings(const FString& Name, bool InSupportIdx = true)
		: DirectionAttribute(FName(FText::Format(FText::FromString(TEXT("{0}Dir")), FText::FromString(Name)).ToString())),
		  LengthAttribute(FName(FText::Format(FText::FromString(TEXT("{0}Len")), FText::FromString(Name)).ToString())),
		  bSupportIdx(InSupportIdx)
	{
	}

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bWriteDirection = false;

	/** Name of the attribute to output the direction to. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bWriteDirection"))
	FName DirectionAttribute = "Direction";
	PCGEx::TFAttributeWriter<FVector>* DirWriter = nullptr;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, InlineEditConditionToggle))
	bool bWriteLength = false;

	/** Name of the attribute to output the length to. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bWriteLength"))
	FName LengthAttribute = "Length";
	PCGEx::TFAttributeWriter<double>* LengthWriter = nullptr;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bSupportIdx", InlineEditConditionToggle))
	bool bWriteEdgeIndex = false;

	/** TBD */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bWriteEdgeIndex && bSupportIdx"))
	FName EdgeIndexAttribute = "EdgeIndex";
	PCGEx::TFAttributeWriter<int32>* EIdxWriter = nullptr;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bSupportIdx", InlineEditConditionToggle))
	bool bWriteVtxIndex = false;

	/** TBD */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bWriteVtxIndex && bSupportIdx"))
	FName VtxIndexAttribute = "VtxIndex";
	PCGEx::TFAttributeWriter<int32>* VIdxWriter = nullptr;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition="false", EditConditionHides, HideInDetailPanel))
	bool bSupportIdx = true;
	
	bool Validate(const FPCGContext* InContext) const
	{
		if (bWriteDirection) { PCGEX_VALIDATE_NAME_C(InContext, DirectionAttribute); }
		if (bWriteLength) { PCGEX_VALIDATE_NAME_C(InContext, LengthAttribute); }

		if (bSupportIdx)
		{
			if (bWriteEdgeIndex) { PCGEX_VALIDATE_NAME_C(InContext, EdgeIndexAttribute); }
			if (bWriteVtxIndex) { PCGEX_VALIDATE_NAME_C(InContext, VtxIndexAttribute); }
		}
		return true;
	}

	void Init(const PCGExCluster::FCluster* InCluster)
	{
		if (bWriteDirection)
		{
			DirWriter = new PCGEx::TFAttributeWriter<FVector>(DirectionAttribute);
			DirWriter->BindAndSetNumUninitialized(*InCluster->PointsIO);
		}

		if (bWriteLength)
		{
			LengthWriter = new PCGEx::TFAttributeWriter<double>(LengthAttribute);
			LengthWriter->BindAndSetNumUninitialized(*InCluster->PointsIO);
		}

		if (bWriteEdgeIndex)
		{
			EIdxWriter = new PCGEx::TFAttributeWriter<int32>(EdgeIndexAttribute);
			EIdxWriter->BindAndSetNumUninitialized(*InCluster->PointsIO);
		}

		if (bWriteVtxIndex)
		{
			VIdxWriter = new PCGEx::TFAttributeWriter<int32>(VtxIndexAttribute);
			VIdxWriter->BindAndSetNumUninitialized(*InCluster->PointsIO);
		}
	}

	void Write(const int32 EntryIndex, const double InLength, const FVector& InDir, const int32 EIndex, const int32 VIndex)
	{
		if (DirWriter) { DirWriter->Values[EntryIndex] = InDir; }
		if (LengthWriter) { LengthWriter->Values[EntryIndex] = InLength; }
		if (EIdxWriter) { EIdxWriter->Values[EntryIndex] = EIndex; }
		if (VIdxWriter) { VIdxWriter->Values[EntryIndex] = VIndex; }
	}

	void Write() const
	{
		if (DirWriter) { DirWriter->Write(); }
		if (LengthWriter) { LengthWriter->Write(); }
		if (EIdxWriter) { EIdxWriter->Write(); }
		if (VIdxWriter) { VIdxWriter->Write(); }
	}

	void Write(const TArrayView<int32> Indices) const
	{
		if (DirWriter) { DirWriter->Write(Indices); }
		if (LengthWriter) { LengthWriter->Write(Indices); }
		if (EIdxWriter) { EIdxWriter->Write(Indices); }
		if (VIdxWriter) { VIdxWriter->Write(Indices); }
	}

	void Cleanup()
	{
		PCGEX_DELETE(DirWriter)
		PCGEX_DELETE(LengthWriter)
		PCGEX_DELETE(EIdxWriter)
		PCGEX_DELETE(VIdxWriter)
	}
};

/**
 * 
 */
UCLASS()
class PCGEXTENDEDTOOLKIT_API UPCGExVtxExtraOperation : public UPCGExOperation
{
	GENERATED_BODY()

public:
	FName OutputAttribute;

	virtual void CopySettingsFrom(const UPCGExOperation* Other) override;

	virtual bool PrepareForCluster(const FPCGContext* InContext, PCGExCluster::FCluster* InCluster);
	virtual bool IsOperationValid();

	FORCEINLINE virtual void ProcessNode(PCGExCluster::FNode& Node, const TArray<PCGExCluster::FAdjacencyData>& Adjacency);

	virtual void Write() override;
	virtual void Write(const TArrayView<int32> Indices) override;

	virtual void Cleanup() override;

protected:
	bool bIsValidOperation = true;
	PCGExCluster::FCluster* Cluster = nullptr;
};

UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Data")
class PCGEXTENDEDTOOLKIT_API UPCGExVtxExtraFactoryBase : public UPCGExNodeStateFactory
{
	GENERATED_BODY()

public:
	virtual PCGExFactories::EType GetFactoryType() const override;
	virtual UPCGExVtxExtraOperation* CreateOperation() const;
};

UCLASS(Abstract, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|VtxExtra")
class PCGEXTENDEDTOOLKIT_API UPCGExVtxExtraProviderSettings : public UPCGExFactoryProviderSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS_CUSTOM_SUBTITLE(
		VtxExtraAttribute, "Vtx Extra : Abstract", "Abstract vtx extra settings.",
		PCGEX_FACTORY_NAME_PRIORITY)
	virtual FLinearColor GetNodeTitleColor() const override { return GetDefault<UPCGExGlobalSettings>()->NodeColorSamplerNeighbor; }

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
#endif
	//~End UPCGSettings

public:
	virtual FName GetMainOutputLabel() const override;
	virtual UPCGExParamFactoryBase* CreateFactory(FPCGContext* InContext, UPCGExParamFactoryBase* InFactory) const override;

#if WITH_EDITOR
	virtual FString GetDisplayName() const override;
#endif

	/** Priority for sampling order. Higher values are processed last. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayPriority=-1))
	int32 Priority = 0;
};
