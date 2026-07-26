#pragma once

#include "CoreMinimal.h"

/** How a dial should be edited, taken from the CVar's registered type (not from the file). */
enum class EBreadboardValueType : uint8
{
	Bool,
	Int,
	Float,
	String,
	/** The name isn't a registered CVar in this build — stale row, or the code renamed it. */
	Unregistered
};

/** One tuning dial: a `Name Value ; help` row parsed out of the exec file. */
struct FBreadboardRow
{
	FString Name;
	/** Value exactly as written in the file — the persisted setting. */
	FString FileValue;
	/** Trailing comment (help text), comment marker stripped. */
	FString Comment;
	/** Index into FBreadboardModel::FileLines, so Save can rewrite only this line's value. */
	int32 LineIndex = INDEX_NONE;
	/** Column the comment marker sat at, so a rewritten line keeps the file's alignment. */
	int32 CommentColumn = INDEX_NONE;
	/** The comment marker used on this line (';' or '#'), so it round-trips. */
	TCHAR CommentMarker = TEXT(';');

	/** Optional `[min..max]` slider hint parsed out of the comment. */
	bool bHasRange = false;
	double RangeMin = 0.0;
	double RangeMax = 0.0;

	EBreadboardValueType Type = EBreadboardValueType::Unregistered;
};

/** A `# ===` banner section of the exec file — LIGHTING, DITHER, COMBAT, and so on. */
struct FBreadboardGroup
{
	FString Title;
	/**
	 * Which top-level tab this section sits under, from a trailing `@tab <Name>` on the
	 * section's title line. Sections that declare none land in FBreadboardGroup::DefaultTab
	 * rather than disappearing — an untagged section must still be reachable.
	 */
	FString Tab;
	TArray<int32> RowIndices;

	/** Where sections with no `@tab` marker go. */
	static const TCHAR* DefaultTab() { return TEXT("Other"); }
};

/**
 * The breadboard's data source is `Saved/SwarmExecOnPlay.txt` itself, not a second table in C++.
 * That file is already the persisted tuning surface (owned by the /cvars skill, exec'd line by
 * line at BeginPlay), and it already carries the grouping, the help text and the owner-tuned
 * markers — so parsing it gives the panel its rows for free and guarantees the panel can never
 * drift from what the game actually loads.
 *
 * Save is a surgical rewrite: every line is kept verbatim except the value token of rows the
 * owner changed, so comments, banners, alignment and the commented-out ACTIONS block survive.
 */
class FBreadboardModel
{
public:
	/** ELVTR/Saved/SwarmExecOnPlay.txt — the file the render actor execs at BeginPlay. */
	static FString GetExecFilePath();

	/** Re-read the file from disk, rebuilding Rows/Groups. False if the file is missing. */
	bool Load();

	/** Write the given per-row values back into the file. Values are indexed by row. */
	bool Save(const TArray<FString>& RowValues, FString& OutError);

	/** Live CVar access. Returns false when the name isn't a registered variable. */
	static EBreadboardValueType ClassifyCVar(const FString& Name);
	static bool GetLiveValue(const FString& Name, FString& OutValue);
	static bool SetLiveValue(const FString& Name, const FString& Value);

	/** Trim a float to the shortest form that still reads as the same number. */
	static FString FormatNumber(double Value);

	/**
	 * Do two value strings mean the same setting? Numeric dials compare as numbers, so a field
	 * reading "2.0" against a live CVar reading "2" is not reported as drift — a false drift
	 * marker on an untouched dial would train the owner to ignore the real ones.
	 */
	static bool ValuesEqual(EBreadboardValueType Type, const FString& A, const FString& B);

	TArray<FBreadboardRow> Rows;
	TArray<FBreadboardGroup> Groups;
	/** Distinct group tabs in the order the file first declares them — the tab bar's order. */
	TArray<FString> Tabs;

private:
	/** Every line of the file, verbatim — the basis of the surgical rewrite in Save. */
	TArray<FString> FileLines;
	/** Line terminator the file on disk used, so Save doesn't flip the whole file to LF. */
	FString LineTerminator = TEXT("\r\n");
};
