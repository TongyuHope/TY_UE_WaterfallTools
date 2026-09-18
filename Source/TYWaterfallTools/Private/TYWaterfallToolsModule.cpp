// Copyright Epic Games, Inc. All Rights Reserved.

#include "TYWaterfallToolsModule.h"
#include "TYWaterfallToolsEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "TYWaterfallToolsModule"

void FTYWaterfallToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FTYWaterfallToolsEditorModeCommands::Register();
}

void FTYWaterfallToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FTYWaterfallToolsEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTYWaterfallToolsModule, TYWaterfallTools)