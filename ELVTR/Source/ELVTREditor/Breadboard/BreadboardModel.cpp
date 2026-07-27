#include "Breadboard/BreadboardModel.h"

#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace BreadboardParse
{
	/** A `# =========` rule line: the banner that opens and closes a section. */
	static bool IsBannerText(const FString& Text)
	{
		if (Text.Len() < 8)
		{
			return false;
		}
		for (int32 i = 0; i < Text.Len(); ++i)
		{
			if (Text[i] != TEXT('='))
			{
				return false;
			}
		}
		return true;
	}

	/** First ';' or '#' on the line — where the help text starts. INDEX_NONE if there is none. */
	static int32 FindCommentMarker(const FString& Line, TCHAR& OutMarker)
	{
		for (int32 i = 0; i < Line.Len(); ++i)
		{
			const TCHAR C = Line[i];
			if (C == TEXT(';') || C == TEXT('#'))
			{
				OutMarker = C;
				return i;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Split a trailing `@tab <Name>` off a section title, returning the tab and leaving
	 * InOutTitle as the display title.
	 *
	 * The tab lives in the exec file rather than in a title->tab table here for the same
	 * reason the groups do: the file is the tuning surface, and the /cvars skill rewrites it.
	 * A table in C++ would silently drop any section that skill adds later; a marker in the
	 * file cannot go stale, and retabbing a section is a one-word edit with no rebuild.
	 */
	static FString SplitTabMarker(FString& InOutTitle)
	{
		const int32 At = InOutTitle.Find(TEXT("@tab"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (At == INDEX_NONE)
		{
			return FString();
		}
		FString Tab = InOutTitle.RightChop(At + 4);
		Tab.TrimStartAndEndInline();
		if (Tab.IsEmpty())
		{
			return FString();   // a bare "@tab" is a typo, not a tab named ""
		}
		InOutTitle.LeftInline(At);
		InOutTitle.TrimStartAndEndInline();
		return Tab;
	}

	/**
	 * Parse a file-header `@tabs A, B, C` line into the tab-bar order.
	 *
	 * Without this the bar would be ordered by which section happens to come first, which is
	 * an accident of how the file reads top-to-bottom (the dither block sits early because it
	 * belongs beside lighting, not because Debug is the second-most-used tab). Declaring the
	 * order separately keeps the file's *reading* order and the bar's *browsing* order free to
	 * differ, and still keeps both in the file.
	 */
	static bool ParseTabOrder(const FString& Text, TArray<FString>& OutOrder)
	{
		if (!Text.StartsWith(TEXT("@tabs"), ESearchCase::IgnoreCase))
		{
			return false;
		}
		TArray<FString> Parts;
		Text.RightChop(5).ParseIntoArray(Parts, TEXT(","), /*InCullEmpty=*/true);
		for (FString& Part : Parts)
		{
			Part.TrimStartAndEndInline();
			if (!Part.IsEmpty())
			{
				OutOrder.AddUnique(Part);
			}
		}
		return true;
	}

	/** Pull a `[min..max]` slider hint out of the help text, if the author wrote one. */
	static void ParseRangeHint(const FString& Comment, FBreadboardRow& Row)
	{
		int32 Open = INDEX_NONE;
		int32 Close = INDEX_NONE;
		if (!Comment.FindChar(TEXT('['), Open))
		{
			return;
		}
		if (!Comment.FindChar(TEXT(']'), Close) || Close < Open)
		{
			return;
		}
		const FString Inner = Comment.Mid(Open + 1, Close - Open - 1);
		FString MinPart;
		FString MaxPart;
		if (!Inner.Split(TEXT(".."), &MinPart, &MaxPart))
		{
			return;
		}
		MinPart.TrimStartAndEndInline();
		MaxPart.TrimStartAndEndInline();
		if (MinPart.IsNumeric() && MaxPart.IsNumeric())
		{
			Row.bHasRange = true;
			Row.RangeMin = FCString::Atod(*MinPart);
			Row.RangeMax = FCString::Atod(*MaxPart);
		}
	}

	/**
	 * Pull a `{0=Ring, 1=Block, 2=Wedge}` dropdown hint out of the help text.
	 *
	 * Braces rather than brackets so a dial can carry both a range and a choice list without
	 * the two parsers eating each other's text.
	 */
	static void ParseChoiceHint(const FString& Comment, FBreadboardRow& Row)
	{
		int32 Open = INDEX_NONE;
		int32 Close = INDEX_NONE;
		if (!Comment.FindChar(TEXT('{'), Open) || !Comment.FindChar(TEXT('}'), Close) || Close < Open)
		{
			return;
		}

		TArray<FString> Parts;
		Comment.Mid(Open + 1, Close - Open - 1).ParseIntoArray(Parts, TEXT(","), /*InCullEmpty=*/true);
		for (const FString& Part : Parts)
		{
			FString Value;
			FString Label;
			if (!Part.Split(TEXT("="), &Value, &Label))
			{
				continue;   // not a `value=label` pair; ignore rather than guess
			}
			Value.TrimStartAndEndInline();
			Label.TrimStartAndEndInline();
			if (!Value.IsEmpty() && !Label.IsEmpty())
			{
				Row.Choices.Emplace(Value, Label);
			}
		}
	}
}

FString FBreadboardModel::GetExecFilePath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SwarmExecOnPlay.txt"));
}

bool FBreadboardModel::Load()
{
	Rows.Reset();
	Groups.Reset();
	FileLines.Reset();

	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *GetExecFilePath()))
	{
		return false;
	}

	// Keep the file's own terminator so saving doesn't rewrite every line in the diff.
	LineTerminator = Raw.Contains(TEXT("\r\n")) ? TEXT("\r\n") : TEXT("\n");

	Raw.ParseIntoArray(FileLines, TEXT("\n"), /*InCullEmpty=*/false);
	for (FString& Line : FileLines)
	{
		Line.RemoveFromEnd(TEXT("\r"));
	}

	// A section is "banner, then comment lines until the next banner"; the first of those
	// comment lines is the section title (the rest is prose the panel shows as a tooltip).
	bool bAfterBanner = false;
	int32 CurrentGroup = INDEX_NONE;
	TArray<FString> DeclaredTabOrder;

	for (int32 LineIndex = 0; LineIndex < FileLines.Num(); ++LineIndex)
	{
		const FString& Line = FileLines[LineIndex];
		FString Trimmed = Line;
		Trimmed.TrimStartAndEndInline();

		if (Trimmed.IsEmpty())
		{
			continue;
		}

		if (Trimmed.StartsWith(TEXT("#")))
		{
			FString Text = Trimmed.RightChop(1);
			Text.TrimStartAndEndInline();

			if (BreadboardParse::IsBannerText(Text))
			{
				bAfterBanner = true;
				continue;
			}
			if (BreadboardParse::ParseTabOrder(Text, DeclaredTabOrder))
			{
				continue;   // a header directive, never a section title
			}
			if (bAfterBanner)
			{
				FBreadboardGroup& Group = Groups.AddDefaulted_GetRef();
				Group.Tab = BreadboardParse::SplitTabMarker(Text);   // strips the marker off Text
				Group.Title = Text;
				CurrentGroup = Groups.Num() - 1;
				bAfterBanner = false;
			}
			continue;
		}

		if (Trimmed.StartsWith(TEXT(";")))
		{
			continue;   // a wrapped continuation of the previous row's help text
		}

		// A candidate dial line: `Name Value ; help`.
		bAfterBanner = false;

		TCHAR Marker = TEXT(';');
		const int32 CommentIndex = BreadboardParse::FindCommentMarker(Line, Marker);
		FString Code = (CommentIndex == INDEX_NONE) ? Line : Line.Left(CommentIndex);
		FString Comment;
		if (CommentIndex != INDEX_NONE)
		{
			Comment = Line.RightChop(CommentIndex + 1);
			Comment.TrimStartAndEndInline();
		}

		Code.TrimStartAndEndInline();
		FString Name;
		FString Value;
		if (!Code.Split(TEXT(" "), &Name, &Value))
		{
			continue;   // a bare command with no argument (an ACTION, not a dial)
		}
		Value.TrimStartAndEndInline();
		if (Name.IsEmpty() || Value.IsEmpty() || !Name.Contains(TEXT(".")))
		{
			continue;
		}

		FBreadboardRow Row;
		Row.Name = Name;
		Row.FileValue = Value;
		Row.Comment = Comment;
		Row.LineIndex = LineIndex;
		Row.CommentColumn = CommentIndex;
		Row.CommentMarker = Marker;
		Row.Type = ClassifyCVar(Name);
		BreadboardParse::ParseRangeHint(Comment, Row);
		BreadboardParse::ParseChoiceHint(Comment, Row);

		const int32 RowIndex = Rows.Add(MoveTemp(Row));

		if (CurrentGroup == INDEX_NONE)
		{
			FBreadboardGroup& Group = Groups.AddDefaulted_GetRef();
			Group.Title = TEXT("UNGROUPED");
			CurrentGroup = Groups.Num() - 1;
		}
		Groups[CurrentGroup].RowIndices.Add(RowIndex);
	}

	// Drop sections that turned out to hold no dials (pure prose banners).
	Groups.RemoveAll([](const FBreadboardGroup& Group) { return Group.RowIndices.Num() == 0; });

	// Collect the tab list AFTER the empty-section cull, so a tab whose only section held no
	// dials doesn't leave a permanently empty button on the bar.
	Tabs.Reset();

	// A declared tab only earns a slot on the bar if some section actually claims it, so an
	// @tabs line left listing a tab whose sections were deleted doesn't leave a dead button.
	for (const FString& Declared : DeclaredTabOrder)
	{
		const bool bClaimed = Groups.ContainsByPredicate(
			[&Declared](const FBreadboardGroup& Group) { return Group.Tab == Declared; });
		if (bClaimed)
		{
			Tabs.AddUnique(Declared);
		}
	}

	// Then anything the header didn't mention, in file order — a section tagged with a brand
	// new tab still shows up rather than vanishing because the header wasn't updated too.
	for (FBreadboardGroup& Group : Groups)
	{
		if (Group.Tab.IsEmpty())
		{
			Group.Tab = FBreadboardGroup::DefaultTab();
		}
		Tabs.AddUnique(Group.Tab);
	}

	return true;
}

bool FBreadboardModel::Save(const TArray<FString>& RowValues, FString& OutError)
{
	if (FileLines.Num() == 0)
	{
		OutError = TEXT("nothing loaded — hit Reload first");
		return false;
	}

	const FString Path = GetExecFilePath();
	if (FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(*Path))
	{
		OutError = FString::Printf(TEXT("%s is read-only"), *Path);
		return false;
	}

	TArray<FString> Out = FileLines;

	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (!RowValues.IsValidIndex(i))
		{
			continue;
		}
		const FBreadboardRow& Row = Rows[i];
		if (!Out.IsValidIndex(Row.LineIndex))
		{
			continue;
		}
		FString NewValue = RowValues[i];
		NewValue.TrimStartAndEndInline();
		if (NewValue.IsEmpty())
		{
			continue;   // an empty field would exec as a bare command and print the CVar instead
		}

		FString Line = Row.Name + TEXT(" ") + NewValue;
		if (Row.CommentColumn != INDEX_NONE)
		{
			// Re-pad to the file's comment column so the diff stays one token wide.
			Line += FString::ChrN(FMath::Max(1, Row.CommentColumn - Line.Len()), TEXT(' '));
			Line += FString::Chr(Row.CommentMarker);
			Line += TEXT(" ");
			Line += Row.Comment;
		}
		Out[Row.LineIndex] = Line;
	}

	const FString Text = FString::Join(Out, *LineTerminator);
	if (!FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("could not write %s"), *Path);
		return false;
	}

	// Disk is now the truth the panel shows as "saved".
	FileLines = MoveTemp(Out);
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (RowValues.IsValidIndex(i))
		{
			FString NewValue = RowValues[i];
			NewValue.TrimStartAndEndInline();
			if (!NewValue.IsEmpty())
			{
				Rows[i].FileValue = NewValue;
			}
		}
	}
	return true;
}

EBreadboardValueType FBreadboardModel::ClassifyCVar(const FString& Name)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Name, /*bTrackFrequentCalls=*/false);
	if (!CVar)
	{
		return EBreadboardValueType::Unregistered;
	}
	if (CVar->IsVariableBool())
	{
		return EBreadboardValueType::Bool;
	}
	if (CVar->IsVariableInt())
	{
		return EBreadboardValueType::Int;
	}
	if (CVar->IsVariableFloat())
	{
		return EBreadboardValueType::Float;
	}
	return EBreadboardValueType::String;
}

bool FBreadboardModel::GetLiveValue(const FString& Name, FString& OutValue)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Name, /*bTrackFrequentCalls=*/false);
	if (!CVar)
	{
		return false;
	}
	if (CVar->IsVariableBool())
	{
		OutValue = CVar->GetBool() ? TEXT("1") : TEXT("0");
	}
	else if (CVar->IsVariableInt())
	{
		OutValue = FString::FromInt(CVar->GetInt());
	}
	else if (CVar->IsVariableFloat())
	{
		OutValue = FormatNumber(CVar->GetFloat());
	}
	else
	{
		OutValue = CVar->GetString();
	}
	return true;
}

bool FBreadboardModel::SetLiveValue(const FString& Name, const FString& Value)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Name, /*bTrackFrequentCalls=*/false);
	if (!CVar)
	{
		return false;
	}
	// SetByConsole so the value outranks scalability/device-profile writes, exactly as
	// typing it in the console would.
	CVar->Set(*Value, ECVF_SetByConsole);
	return true;
}

bool FBreadboardModel::ValuesEqual(EBreadboardValueType Type, const FString& A, const FString& B)
{
	switch (Type)
	{
	case EBreadboardValueType::Bool:
		// Any non-zero number is on, so "1" and "true" and "2" all mean the same thing here.
		return (FCString::Atod(*A) != 0.0) == (FCString::Atod(*B) != 0.0);
	case EBreadboardValueType::Int:
		return FCString::Atoi64(*A) == FCString::Atoi64(*B);
	case EBreadboardValueType::Float:
		return FMath::IsNearlyEqual(FCString::Atod(*A), FCString::Atod(*B), UE_KINDA_SMALL_NUMBER);
	default:
		return A == B;
	}
}

FString FBreadboardModel::FormatNumber(double Value)
{
	FString Text = FString::Printf(TEXT("%.4f"), Value);
	if (Text.Contains(TEXT(".")))
	{
		int32 Last = Text.Len() - 1;
		while (Last > 0 && Text[Last] == TEXT('0'))
		{
			--Last;
		}
		if (Text[Last] == TEXT('.'))
		{
			--Last;
		}
		Text = Text.Left(Last + 1);
	}
	return Text;
}
