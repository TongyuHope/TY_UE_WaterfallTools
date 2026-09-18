// Copyright Epic Games, Inc. All Rights Reserved.

#include "TYWaterfallToolsEditorModeToolkit.h"
#include "TYWaterfallToolsEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "TYWaterfallToolsEditorModeToolkit"

FTYWaterfallToolsEditorModeToolkit::FTYWaterfallToolsEditorModeToolkit()
{
}

void FTYWaterfallToolsEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FTYWaterfallToolsEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FTYWaterfallToolsEditorModeToolkit::GetToolkitFName() const
{
	return FName("TYWaterfallToolsEditorMode");
}

FText FTYWaterfallToolsEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "TYWaterfallToolsEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
