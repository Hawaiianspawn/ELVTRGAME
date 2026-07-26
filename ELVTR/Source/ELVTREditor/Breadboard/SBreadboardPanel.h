#pragma once

#include "CoreMinimal.h"
#include "Breadboard/BreadboardModel.h"
#include "Widgets/SCompoundWidget.h"

class SHorizontalBox;
class SSearchBox;
class SVerticalBox;

/**
 * Per-row edit state. Held by shared ref so every row widget's lambdas can capture it and
 * outlive a list rebuild (filtering rebuilds widgets but must not lose typed values).
 */
struct FBreadboardRowState
{
	int32 RowIndex = INDEX_NONE;
	/** What the field currently shows — the value Save will write. */
	FString Edit;
};

/**
 * The breadboard: every tuning dial in Saved/SwarmExecOnPlay.txt as a live field, grouped the
 * way the file groups them. Editing a field sets the CVar immediately (so the running game
 * changes under your hands) and Save writes the values back to the file so they survive a
 * restart. That split is deliberate: live edits are lost at the next BeginPlay because the
 * render actor re-execs the file, so Save is what makes a value stick.
 */
class SBreadboardPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBreadboardPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Re-read the file, discarding unsaved field edits. */
	void ReloadFromFile();
	/** Rebuild the group/row widgets (after a reload, tab switch or filter change). */
	void RebuildRows();
	/** Rebuild the tab buttons from the tabs the file declared. */
	void RebuildTabs();

	/** True while a search is active — search spans every tab, so the tab bar goes inert. */
	bool IsSearching() const { return !Filter.IsEmpty(); }
	/** How many dials a tab holds, for the count on its button. */
	int32 CountRowsInTab(const FString& Tab) const;

	TSharedRef<SWidget> BuildRow(TSharedRef<FBreadboardRowState> State);
	TSharedRef<SWidget> BuildValueEditor(TSharedRef<FBreadboardRowState> State);

	/** Set a field and, when bApplyLive, push it straight to the running CVar. */
	void CommitRow(TSharedRef<FBreadboardRowState> State, const FString& NewValue, bool bApplyLive);

	FReply OnReloadClicked();
	FReply OnPullLiveClicked();
	FReply OnApplyAllClicked();
	FReply OnSaveClicked();

	const FBreadboardRow& RowFor(const TSharedRef<FBreadboardRowState>& State) const;
	bool PassesFilter(const FBreadboardRow& Row) const;
	bool HasUnsavedEdits() const;

	FBreadboardModel Model;
	/** Parallel to Model.Rows. */
	TArray<TSharedRef<FBreadboardRowState>> States;

	TSharedPtr<SVerticalBox> ListBox;
	TSharedPtr<SHorizontalBox> TabBar;
	TSharedPtr<SSearchBox> SearchBox;
	FString Filter;
	/** Empty = the "All" pseudo-tab. Survives a reload so re-reading the file keeps your place. */
	FString ActiveTab;
	FText StatusText;
};
