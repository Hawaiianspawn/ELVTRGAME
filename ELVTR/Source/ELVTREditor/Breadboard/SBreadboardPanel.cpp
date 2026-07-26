#include "Breadboard/SBreadboardPanel.h"

#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Breadboard"

namespace BreadboardUI
{
	static const float NameColumnWidth = 300.f;
	static const float EditorColumnWidth = 150.f;
	static const float LiveColumnWidth = 120.f;

	static const FLinearColor DirtyColor(0.95f, 0.72f, 0.25f);   // edited, not yet saved
	static const FLinearColor DriftColor(0.45f, 0.75f, 1.00f);   // live differs from the field
	static const FLinearColor MutedColor(0.55f, 0.55f, 0.55f);
	static const FLinearColor BadColor(0.90f, 0.35f, 0.35f);     // not a registered CVar
	static const FLinearColor TabActiveColor(0.30f, 0.55f, 0.90f); // the open tab's button

	/** Slider bounds. Uses a `[min..max]` hint from the help text when the file has one, else
	 *  brackets the file value so the handle lands somewhere useful without clamping typing. */
	static void SliderRange(const FBreadboardRow& Row, double& OutMin, double& OutMax)
	{
		if (Row.bHasRange && Row.RangeMax > Row.RangeMin)
		{
			OutMin = Row.RangeMin;
			OutMax = Row.RangeMax;
			return;
		}
		const double Value = FCString::Atod(*Row.FileValue);
		const double Magnitude = FMath::Max(1.0, FMath::Abs(Value) * 3.0);
		OutMin = (Value < 0.0) ? -Magnitude : 0.0;
		OutMax = Magnitude;
	}
}

void SBreadboardPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		// Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 6.f, 6.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text_Lambda([this]()
				{
					return HasUnsavedEdits()
						? LOCTEXT("SaveDirty", "Save to file *")
						: LOCTEXT("Save", "Save to file");
				})
				.ToolTipText(LOCTEXT("SaveTip", "Write every field back into Saved/SwarmExecOnPlay.txt.\nThis is what makes a value survive the next BeginPlay."))
				.OnClicked(this, &SBreadboardPanel::OnSaveClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ApplyAll", "Apply all"))
				.ToolTipText(LOCTEXT("ApplyAllTip", "Push every field to its live CVar (editing one field already does this)."))
				.OnClicked(this, &SBreadboardPanel::OnApplyAllClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PullLive", "Pull live"))
				.ToolTipText(LOCTEXT("PullLiveTip", "Copy the running game's current CVar values into the fields, so you can Save what you tuned from the console."))
				.OnClicked(this, &SBreadboardPanel::OnPullLiveClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Reload", "Reload"))
				.ToolTipText(LOCTEXT("ReloadTip", "Re-read the file from disk, discarding unsaved field edits."))
				.OnClicked(this, &SBreadboardPanel::OnReloadClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(LOCTEXT("FilterHint", "Filter dials…"))
				.OnTextChanged_Lambda([this](const FText& Text)
				{
					Filter = Text.ToString();
					RebuildRows();
				})
			]
		]

		// Tab bar — one button per @tab the file declares, plus All.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 2.f, 6.f, 2.f)
		[
			SAssignNew(TabBar, SHorizontalBox)
		]

		// Status line
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 0.f, 6.f, 4.f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(BreadboardUI::MutedColor)
			.Text_Lambda([this]() { return StatusText; })
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(ListBox, SVerticalBox)
			]
		]
	];

	ReloadFromFile();
}

void SBreadboardPanel::ReloadFromFile()
{
	States.Reset();

	if (!Model.Load())
	{
		StatusText = FText::Format(LOCTEXT("LoadFailed", "Could not read {0}"),
			FText::FromString(FBreadboardModel::GetExecFilePath()));
		RebuildRows();
		return;
	}

	States.Reserve(Model.Rows.Num());
	for (int32 i = 0; i < Model.Rows.Num(); ++i)
	{
		TSharedRef<FBreadboardRowState> State = MakeShared<FBreadboardRowState>();
		State->RowIndex = i;
		State->Edit = Model.Rows[i].FileValue;
		States.Add(State);
	}

	// Keep the open tab across a reload, but not if the file no longer declares it.
	if (!ActiveTab.IsEmpty() && !Model.Tabs.Contains(ActiveTab))
	{
		ActiveTab.Reset();
	}

	StatusText = FText::Format(LOCTEXT("Loaded", "{0} dials in {1} groups across {2} tabs — from {3}"),
		FText::AsNumber(Model.Rows.Num()),
		FText::AsNumber(Model.Groups.Num()),
		FText::AsNumber(Model.Tabs.Num()),
		FText::FromString(FBreadboardModel::GetExecFilePath()));

	RebuildTabs();
	RebuildRows();
}

int32 SBreadboardPanel::CountRowsInTab(const FString& Tab) const
{
	int32 Count = 0;
	for (const FBreadboardGroup& Group : Model.Groups)
	{
		if (Tab.IsEmpty() || Group.Tab == Tab)
		{
			Count += Group.RowIndices.Num();
		}
	}
	return Count;
}

void SBreadboardPanel::RebuildTabs()
{
	if (!TabBar.IsValid())
	{
		return;
	}
	TabBar->ClearChildren();

	// "All" first, then the file's tabs in declaration order.
	TArray<FString> Entries;
	Entries.Add(FString());
	Entries.Append(Model.Tabs);

	for (const FString& Tab : Entries)
	{
		const FString Label = Tab.IsEmpty() ? TEXT("All") : Tab;
		const int32 Count = CountRowsInTab(Tab);

		TabBar->AddSlot()
		.AutoWidth()
		.Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SButton)
			.Text(FText::Format(LOCTEXT("TabLabel", "{0} ({1})"),
				FText::FromString(Label), FText::AsNumber(Count)))
			.ToolTipText(Tab.IsEmpty()
				? LOCTEXT("TabAllTip", "Every dial in the file.")
				: FText::Format(LOCTEXT("TabTip",
					"Only sections marked '@tab {0}' in Saved/SwarmExecOnPlay.txt.\n"
					"Move a section between tabs by editing that marker — no rebuild."),
					FText::FromString(Tab)))
			// A search spans every tab, so the bar can't also be filtering — grey it out
			// rather than let it silently hide matches the search just found.
			.IsEnabled_Lambda([this]() { return !IsSearching(); })
			.ButtonColorAndOpacity_Lambda([this, Tab]()
			{
				return ActiveTab == Tab
					? FSlateColor(BreadboardUI::TabActiveColor)
					: FSlateColor(FLinearColor::White);
			})
			.OnClicked_Lambda([this, Tab]()
			{
				ActiveTab = Tab;
				RebuildRows();
				return FReply::Handled();
			})
		];
	}
}

const FBreadboardRow& SBreadboardPanel::RowFor(const TSharedRef<FBreadboardRowState>& State) const
{
	return Model.Rows[State->RowIndex];
}

bool SBreadboardPanel::PassesFilter(const FBreadboardRow& Row) const
{
	if (Filter.IsEmpty())
	{
		return true;
	}
	return Row.Name.Contains(Filter) || Row.Comment.Contains(Filter);
}

bool SBreadboardPanel::HasUnsavedEdits() const
{
	for (const TSharedRef<FBreadboardRowState>& State : States)
	{
		if (State->Edit != Model.Rows[State->RowIndex].FileValue)
		{
			return true;
		}
	}
	return false;
}

void SBreadboardPanel::RebuildRows()
{
	if (!ListBox.IsValid())
	{
		return;
	}
	ListBox->ClearChildren();

	int32 TotalShown = 0;

	for (const FBreadboardGroup& Group : Model.Groups)
	{
		// A search looks across every tab: finding a dial by name shouldn't depend on already
		// knowing which tab it lives in. Outside a search the tab is the filter.
		if (!IsSearching() && !ActiveTab.IsEmpty() && Group.Tab != ActiveTab)
		{
			continue;
		}

		TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
		int32 Shown = 0;

		for (const int32 RowIndex : Group.RowIndices)
		{
			if (!States.IsValidIndex(RowIndex) || !PassesFilter(Model.Rows[RowIndex]))
			{
				continue;
			}
			Body->AddSlot().AutoHeight()[BuildRow(States[RowIndex])];
			++Shown;
		}

		if (Shown == 0)
		{
			continue;   // whole group filtered out
		}
		TotalShown += Shown;

		// While searching, sections from other tabs are in the list — say which tab each came
		// from, so a hit doesn't look like it belongs to the tab that happens to be open.
		const FText AreaTitle = IsSearching()
			? FText::Format(LOCTEXT("GroupTitleSearch", "{0}   ·   {1}"),
				FText::FromString(Group.Title), FText::FromString(Group.Tab))
			: FText::FromString(Group.Title);

		TSharedRef<SExpandableArea> Area = SNew(SExpandableArea)
			.InitiallyCollapsed(false)
			.AreaTitle(AreaTitle)
			.Padding(FMargin(8.f, 4.f))
			.BodyContent()
			[
				Body
			];

		// Verified 2026-07-25: sections come up collapsed despite InitiallyCollapsed(false).
		// SExpandableArea gates its body on a rollout curve that is only jumped to the end by a
		// state *change*, so the construction-time flag alone can leave the body at scale 0.
		// Toggling forces that transition, and is correct from either starting state: the
		// collapse call no-ops when it is already collapsed, and the expand call always runs.
		Area->SetExpanded(false);
		Area->SetExpanded(true);

		ListBox->AddSlot()
		.AutoHeight()
		.Padding(6.f, 2.f)
		[
			Area
		];
	}

	if (TotalShown == 0)
	{
		ListBox->AddSlot()
		.AutoHeight()
		.Padding(10.f, 8.f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(BreadboardUI::MutedColor)
			.Text(IsSearching()
				? LOCTEXT("NoMatches", "No dial matches that search.")
				: LOCTEXT("EmptyTab", "This tab has no dials."))
		];
	}
}

TSharedRef<SWidget> SBreadboardPanel::BuildRow(TSharedRef<FBreadboardRowState> State)
{
	const FBreadboardRow& Row = RowFor(State);
	const bool bRegistered = Row.Type != EBreadboardValueType::Unregistered;

	const FText Tooltip = Row.Comment.IsEmpty()
		? FText::FromString(Row.Name)
		: FText::Format(LOCTEXT("RowTip", "{0}\n\n{1}"), FText::FromString(Row.Name), FText::FromString(Row.Comment));

	return SNew(SHorizontalBox)
		.ToolTipText(Tooltip)

		// Name — amber while the field differs from what's saved in the file.
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 1.f, 6.f, 1.f)
		[
			SNew(SBox)
			.WidthOverride(BreadboardUI::NameColumnWidth)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Row.Name))
				.ColorAndOpacity_Lambda([this, State, bRegistered]()
				{
					if (!bRegistered)
					{
						return FSlateColor(BreadboardUI::BadColor);
					}
					return State->Edit != RowFor(State).FileValue
						? FSlateColor(BreadboardUI::DirtyColor)
						: FSlateColor::UseForeground();
				})
			]
		]

		// The editable field
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 1.f, 8.f, 1.f)
		[
			SNew(SBox)
			.WidthOverride(BreadboardUI::EditorColumnWidth)
			.IsEnabled(bRegistered)
			[
				BuildValueEditor(State)
			]
		]

		// What the running game actually has right now
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(BreadboardUI::LiveColumnWidth)
			[
				SNew(STextBlock)
				.Text_Lambda([this, State]()
				{
					FString Live;
					if (!FBreadboardModel::GetLiveValue(RowFor(State).Name, Live))
					{
						return LOCTEXT("NotRegistered", "not registered");
					}
					return FText::Format(LOCTEXT("LiveValue", "live {0}"), FText::FromString(Live));
				})
				.ColorAndOpacity_Lambda([this, State]()
				{
					FString Live;
					if (!FBreadboardModel::GetLiveValue(RowFor(State).Name, Live))
					{
						return FSlateColor(BreadboardUI::BadColor);
					}
					const FBreadboardRow& Row = RowFor(State);
					return FBreadboardModel::ValuesEqual(Row.Type, Live, State->Edit)
						? FSlateColor(BreadboardUI::MutedColor)
						: FSlateColor(BreadboardUI::DriftColor);
				})
			]
		]

		// Revert this dial to the saved file value
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(4.f, 0.f))
			.ToolTipText(LOCTEXT("RevertTip", "Back to the value saved in the file"))
			.Visibility_Lambda([this, State]()
			{
				return State->Edit != RowFor(State).FileValue ? EVisibility::Visible : EVisibility::Hidden;
			})
			.OnClicked_Lambda([this, State]()
			{
				CommitRow(State, RowFor(State).FileValue, /*bApplyLive=*/true);
				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("PropertyWindow.DiffersFromDefault"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
}

TSharedRef<SWidget> SBreadboardPanel::BuildValueEditor(TSharedRef<FBreadboardRowState> State)
{
	const FBreadboardRow& Row = RowFor(State);

	double SliderMin = 0.0;
	double SliderMax = 1.0;
	BreadboardUI::SliderRange(Row, SliderMin, SliderMax);

	switch (Row.Type)
	{
	case EBreadboardValueType::Bool:
		return SNew(SCheckBox)
			.IsChecked_Lambda([State]()
			{
				return (State->Edit != TEXT("0") && !State->Edit.IsEmpty())
					? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, State](ECheckBoxState NewState)
			{
				CommitRow(State, NewState == ECheckBoxState::Checked ? TEXT("1") : TEXT("0"), true);
			});

	case EBreadboardValueType::Int:
		return SNew(SSpinBox<int32>)
			.Value_Lambda([State]() { return FCString::Atoi(*State->Edit); })
			.MinValue(TOptional<int32>())
			.MaxValue(TOptional<int32>())
			.MinSliderValue(FMath::FloorToInt32(SliderMin))
			.MaxSliderValue(FMath::Max(1, FMath::CeilToInt32(SliderMax)))
			.Delta(1)
			.OnValueChanged_Lambda([this, State](int32 NewValue)
			{
				CommitRow(State, FString::FromInt(NewValue), true);
			})
			.OnValueCommitted_Lambda([this, State](int32 NewValue, ETextCommit::Type)
			{
				CommitRow(State, FString::FromInt(NewValue), true);
			});

	case EBreadboardValueType::Float:
		return SNew(SSpinBox<float>)
			.Value_Lambda([State]() { return FCString::Atof(*State->Edit); })
			.MinValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MinSliderValue(static_cast<float>(SliderMin))
			.MaxSliderValue(static_cast<float>(SliderMax))
			.Delta(0.f)
			// Show what the file shows: a whole-number dial reads "900", not "900.0".
			.MinFractionalDigits(0)
			.MaxFractionalDigits(4)
			.OnValueChanged_Lambda([this, State](float NewValue)
			{
				CommitRow(State, FBreadboardModel::FormatNumber(NewValue), true);
			})
			.OnValueCommitted_Lambda([this, State](float NewValue, ETextCommit::Type)
			{
				CommitRow(State, FBreadboardModel::FormatNumber(NewValue), true);
			});

	default:
		return SNew(SEditableTextBox)
			.Text_Lambda([State]() { return FText::FromString(State->Edit); })
			.OnTextCommitted_Lambda([this, State](const FText& NewText, ETextCommit::Type)
			{
				CommitRow(State, NewText.ToString(), true);
			});
	}
}

void SBreadboardPanel::CommitRow(TSharedRef<FBreadboardRowState> State, const FString& NewValue, bool bApplyLive)
{
	State->Edit = NewValue;
	if (!bApplyLive)
	{
		return;
	}

	const FBreadboardRow& Row = RowFor(State);
	if (FBreadboardModel::SetLiveValue(Row.Name, NewValue))
	{
		StatusText = FText::Format(LOCTEXT("Applied", "{0} = {1}   (live; Save to keep it)"),
			FText::FromString(Row.Name), FText::FromString(NewValue));
	}
	else
	{
		StatusText = FText::Format(LOCTEXT("ApplyFailed", "{0} is not a registered CVar in this build"),
			FText::FromString(Row.Name));
	}
}

FReply SBreadboardPanel::OnReloadClicked()
{
	ReloadFromFile();
	return FReply::Handled();
}

FReply SBreadboardPanel::OnPullLiveClicked()
{
	int32 Pulled = 0;
	for (const TSharedRef<FBreadboardRowState>& State : States)
	{
		FString Live;
		if (FBreadboardModel::GetLiveValue(Model.Rows[State->RowIndex].Name, Live))
		{
			State->Edit = Live;
			++Pulled;
		}
	}
	StatusText = FText::Format(LOCTEXT("Pulled", "Pulled {0} live values into the fields"), FText::AsNumber(Pulled));
	return FReply::Handled();
}

FReply SBreadboardPanel::OnApplyAllClicked()
{
	int32 Applied = 0;
	for (const TSharedRef<FBreadboardRowState>& State : States)
	{
		if (FBreadboardModel::SetLiveValue(Model.Rows[State->RowIndex].Name, State->Edit))
		{
			++Applied;
		}
	}
	StatusText = FText::Format(LOCTEXT("AppliedAll", "Applied {0} dials to the running game"), FText::AsNumber(Applied));
	return FReply::Handled();
}

FReply SBreadboardPanel::OnSaveClicked()
{
	TArray<FString> Values;
	Values.Reserve(States.Num());
	for (const TSharedRef<FBreadboardRowState>& State : States)
	{
		Values.Add(State->Edit);
	}

	FString Error;
	if (Model.Save(Values, Error))
	{
		StatusText = FText::Format(LOCTEXT("Saved", "Saved {0} dials to {1}"),
			FText::AsNumber(Values.Num()), FText::FromString(FBreadboardModel::GetExecFilePath()));
	}
	else
	{
		StatusText = FText::Format(LOCTEXT("SaveFailed", "Save failed: {0}"), FText::FromString(Error));
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
