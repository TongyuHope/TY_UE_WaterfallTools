// Copyright Epic Games, Inc. All Rights Reserved.

#include "TYWaterfallToolsEditorModeToolkit.h"
#include "TYWaterfallToolsEditorMode.h"

#include "Actors/TYWaterfallActor.h"
#include "Components/TYWaterfallSettingsComponent.h"
#include "IDetailsView.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TYWaterfallToolsEditorModeToolkit"

FTYWaterfallToolsEditorModeToolkit::FTYWaterfallToolsEditorModeToolkit()
{
}

void FTYWaterfallToolsEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
	ModeDetailsView->SetIsPropertyEditingEnabledDelegate(
		FIsPropertyEditingEnabled::CreateSP(
			this, &FTYWaterfallToolsEditorModeToolkit::CanEditSettings));

	ToolkitWidget = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(this, &FTYWaterfallToolsEditorModeToolkit::GetSelectionText)
			.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(this, &FTYWaterfallToolsEditorModeToolkit::GetStatusText)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 4.0f)
		[
			SNew(SProgressBar)
			.Percent(this, &FTYWaterfallToolsEditorModeToolkit::GetGenerationProgress)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("GeneratePaths", "Generate Paths"))
				.ToolTipText(LOCTEXT("GeneratePathsTooltip", "Generate deterministic waterfall paths."))
				.IsEnabled(this, &FTYWaterfallToolsEditorModeToolkit::CanRunGenerationCommand)
				.OnClicked(this, &FTYWaterfallToolsEditorModeToolkit::OnGeneratePathsClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateMesh", "Generate Mesh"))
				.ToolTipText(LOCTEXT("GenerateMeshTooltip", "Build Per Path, Cross and Splash geometry from the generated paths."))
				.IsEnabled(this, &FTYWaterfallToolsEditorModeToolkit::CanRunGenerationCommand)
				.OnClicked(this, &FTYWaterfallToolsEditorModeToolkit::OnGenerateMeshClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshNiagara", "Refresh FX"))
				.ToolTipText(LOCTEXT("RefreshNiagaraTooltip", "Refresh Top, Middle and Bottom Niagara data from the generated paths."))
				.IsEnabled(this, &FTYWaterfallToolsEditorModeToolkit::CanRunGenerationCommand)
				.OnClicked(this, &FTYWaterfallToolsEditorModeToolkit::OnRefreshNiagaraClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 4.0f, 8.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearAll", "Clear All"))
				.ToolTipText(LOCTEXT("ClearAllTooltip", "Remove generated paths, mesh and Niagara point data."))
				.IsEnabled(this, &FTYWaterfallToolsEditorModeToolkit::CanRunGenerationCommand)
				.OnClicked(this, &FTYWaterfallToolsEditorModeToolkit::OnClearClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.ToolTipText(LOCTEXT("CancelTooltip", "Cancel the active path generation task."))
				.IsEnabled(this, &FTYWaterfallToolsEditorModeToolkit::CanCancelGeneration)
				.OnClicked(this, &FTYWaterfallToolsEditorModeToolkit::OnCancelClicked)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8.0f, 4.0f)
		[
			ModeDetailsView.ToSharedRef()
		];
}

void FTYWaterfallToolsEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
}


FName FTYWaterfallToolsEditorModeToolkit::GetToolkitFName() const
{
	return FName("TYWaterfallToolsEditorMode");
}

FText FTYWaterfallToolsEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "TY Waterfall Tools");
}

TSharedPtr<SWidget> FTYWaterfallToolsEditorModeToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}

void FTYWaterfallToolsEditorModeToolkit::SetSelectedWaterfall(ATYWaterfallActor* InWaterfall)
{
	SelectedWaterfall = InWaterfall;
	if (ModeDetailsView.IsValid())
	{
		// The authoring panel intentionally exposes only waterfall settings. Showing
		// the whole actor duplicates Transform, Rendering, Collision and networking
		// categories that belong in the editor's standard Details panel.
		ModeDetailsView->SetObject(
			InWaterfall ? InWaterfall->GetWaterfallSettings() : nullptr);
	}
}

FText FTYWaterfallToolsEditorModeToolkit::GetSelectionText() const
{
	if (const ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		return FText::FromString(Waterfall->GetActorLabel());
	}
	return LOCTEXT("NoSelection", "Select a TY Waterfall Actor");
}

FText FTYWaterfallToolsEditorModeToolkit::GetStatusText() const
{
	const ATYWaterfallActor* Waterfall = SelectedWaterfall.Get();
	if (!Waterfall)
	{
		return LOCTEXT("NoSelectionStatus", "No waterfall selected");
	}
	if (Waterfall->IsGeneratingPaths())
	{
		return FText::Format(LOCTEXT("GeneratingStatus", "Generating paths: {0}"),
			FText::AsPercent(Waterfall->GetPathGenerationProgress()));
	}
	return LOCTEXT("ReadyStatus", "Ready");
}

TOptional<float> FTYWaterfallToolsEditorModeToolkit::GetGenerationProgress() const
{
	if (const ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		return Waterfall->IsGeneratingPaths()
			? Waterfall->GetPathGenerationProgress()
			: 0.0f;
	}
	return 0.0f;
}

bool FTYWaterfallToolsEditorModeToolkit::CanRunGenerationCommand() const
{
	const ATYWaterfallActor* Waterfall = SelectedWaterfall.Get();
	return Waterfall && !Waterfall->IsGeneratingPaths();
}

bool FTYWaterfallToolsEditorModeToolkit::CanCancelGeneration() const
{
	const ATYWaterfallActor* Waterfall = SelectedWaterfall.Get();
	return Waterfall && Waterfall->IsGeneratingPaths();
}

bool FTYWaterfallToolsEditorModeToolkit::CanEditSettings() const
{
	return CanRunGenerationCommand();
}

FReply FTYWaterfallToolsEditorModeToolkit::OnGeneratePathsClicked()
{
	if (ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		Waterfall->GeneratePaths();
	}
	return FReply::Handled();
}

FReply FTYWaterfallToolsEditorModeToolkit::OnGenerateMeshClicked()
{
	if (ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		Waterfall->GenerateMesh();
	}
	return FReply::Handled();
}

FReply FTYWaterfallToolsEditorModeToolkit::OnRefreshNiagaraClicked()
{
	if (ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		Waterfall->RefreshNiagaraEffects();
	}
	return FReply::Handled();
}

FReply FTYWaterfallToolsEditorModeToolkit::OnClearClicked()
{
	if (ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		Waterfall->ClearGeneratedPaths();
	}
	return FReply::Handled();
}

FReply FTYWaterfallToolsEditorModeToolkit::OnCancelClicked()
{
	if (ATYWaterfallActor* Waterfall = SelectedWaterfall.Get())
	{
		Waterfall->CancelPathGeneration();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
