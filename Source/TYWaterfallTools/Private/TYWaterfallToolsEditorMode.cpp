// Copyright Epic Games, Inc. All Rights Reserved.

#include "TYWaterfallToolsEditorMode.h"
#include "TYWaterfallToolsEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "TYWaterfallToolsEditorModeCommands.h"
#include "Modules/ModuleManager.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/TYWaterfallToolsSimpleTool.h"
#include "Tools/TYWaterfallToolsInteractiveTool.h"

// step 2: register a ToolBuilder in FTYWaterfallToolsEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "TYWaterfallToolsEditorMode"

const FEditorModeID UTYWaterfallToolsEditorMode::EM_TYWaterfallToolsEditorModeId = TEXT("EM_TYWaterfallToolsEditorMode");

FString UTYWaterfallToolsEditorMode::SimpleToolName = TEXT("TYWaterfallTools_ActorInfoTool");
FString UTYWaterfallToolsEditorMode::InteractiveToolName = TEXT("TYWaterfallTools_MeasureDistanceTool");


UTYWaterfallToolsEditorMode::UTYWaterfallToolsEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UTYWaterfallToolsEditorMode::EM_TYWaterfallToolsEditorModeId,
		LOCTEXT("ModeName", "TYWaterfallTools"),
		FSlateIcon(),
		true);
}


UTYWaterfallToolsEditorMode::~UTYWaterfallToolsEditorMode()
{
}


void UTYWaterfallToolsEditorMode::ActorSelectionChangeNotify()
{
}

void UTYWaterfallToolsEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FTYWaterfallToolsEditorModeCommands& SampleToolCommands = FTYWaterfallToolsEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UTYWaterfallToolsSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UTYWaterfallToolsInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UTYWaterfallToolsEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FTYWaterfallToolsEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UTYWaterfallToolsEditorMode::GetModeCommands() const
{
	return FTYWaterfallToolsEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
