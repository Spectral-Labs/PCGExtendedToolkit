﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#include "Paths/PCGExPathProcessor.h"

#include "PCGExPointsMT.h"
#include "Graph/PCGExCluster.h"

#define LOCTEXT_NAMESPACE "PCGExPathProcessorElement"

FString UPCGExPathProcessorSettings::GetPointFilterTooltip() const { return TEXT("Path points processing filters"); }

PCGExData::EInit UPCGExPathProcessorSettings::GetMainOutputInitMode() const { return PCGExData::EInit::DuplicateInput; }

FName UPCGExPathProcessorSettings::GetMainInputLabel() const { return PCGExGraph::SourcePathsLabel; }
FName UPCGExPathProcessorSettings::GetMainOutputLabel() const { return PCGExGraph::OutputPathsLabel; }

FPCGExPathProcessorContext::~FPCGExPathProcessorContext()
{
	PCGEX_TERMINATE_ASYNC
}

PCGEX_INITIALIZE_CONTEXT(PathProcessor)

bool FPCGExPathProcessorElement::Boot(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElement::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(PathProcessor)

	return true;
}


#undef LOCTEXT_NAMESPACE
