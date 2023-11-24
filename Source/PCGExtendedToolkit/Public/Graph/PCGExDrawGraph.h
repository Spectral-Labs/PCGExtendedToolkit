﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "IPCGExDebug.h"

#include "PCGExGraphProcessor.h"

#include "PCGExDrawGraph.generated.h"

/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Graph")
class PCGEXTENDEDTOOLKIT_API UPCGExDrawGraphSettings : public UPCGExGraphProcessorSettings, public IPCGExDebug
{
	GENERATED_BODY()

public:
	UPCGExDrawGraphSettings(const FObjectInitializer& ObjectInitializer);
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(DrawGraph, "Draw Graph", "Draw graph edges. Toggle debug OFF (D) before disabling this node (E)! Warning: this node will clear persistent debug lines before it!");
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Debug; }
	virtual FLinearColor GetNodeTitleColor() const override { return PCGEx::NodeColorDebug; }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	virtual PCGExIO::EInitMode GetPointOutputInitMode() const override;

public:
	//~Begin IPCGExDebug interface
	virtual bool IsDebugEnabled() const override { return bEnabled && bDebug; }
	//~End IPCGExDebug interface

	/** Draw edges.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bDrawGraph = true;

	/** Type of edge to draw.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExEdgeType"))
	uint8 EdgeType = static_cast<uint8>(EPCGExEdgeType::Unknown);

	/** Draw socket cones lines.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bDrawSocketCones = false;

private:
	friend class FPCGExDrawGraphElement;
};

class PCGEXTENDEDTOOLKIT_API FPCGExDrawGraphElement : public FPCGExGraphProcessorElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
};
