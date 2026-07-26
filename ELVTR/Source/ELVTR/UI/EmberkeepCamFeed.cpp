#include "UI/EmberkeepCamFeed.h"
#include "UI/EmberkeepPalette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/CoreStyle.h"

void UEmberkeepCamFeed::Setup(UTextureRenderTarget2D* InRenderTarget, const FText& InLabel, bool bInBoxy)
{
	RenderTarget = InRenderTarget;
	CamLabel = InLabel;
	bBoxy = bInBoxy;
	Refresh();
}

void UEmberkeepCamFeed::SetHostSized(bool bInHostSized)
{
	bHostSized = bInHostSized;
	Refresh();
}

TSharedRef<SWidget> UEmberkeepCamFeed::RebuildWidget()
{
	if (!Frame)
	{
		SizeRoot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box"));
		WidgetTree->RootWidget = SizeRoot;

		Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		SizeRoot->SetContent(Frame);

		UOverlay* Inner = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Inner"));
		Frame->SetContent(Inner);

		FeedImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FeedImage"));
		if (UOverlaySlot* FeedSlot = Inner->AddChildToOverlay(FeedImage))
		{
			FeedSlot->SetHorizontalAlignment(HAlign_Fill);
			FeedSlot->SetVerticalAlignment(VAlign_Fill);
		}

		Tag = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Tag"));
		Tag->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Tag->SetPadding(FMargin(4.f, 1.f));
		LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
		LabelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 7));
		LabelText->SetColorAndOpacity(FSlateColor(Demichrome::Pale()));
		Tag->SetContent(LabelText);
		if (UOverlaySlot* TagSlot = Inner->AddChildToOverlay(Tag))
		{
			TagSlot->SetHorizontalAlignment(HAlign_Left);
			TagSlot->SetVerticalAlignment(VAlign_Top);
			TagSlot->SetPadding(FMargin(4.f));
		}
	}

	Refresh();
	return Super::RebuildWidget();
}

void UEmberkeepCamFeed::Refresh()
{
	if (!Frame)
	{
		return;
	}

	// Hero = thin Pale frame; unit = thicker Steel boxy frame.
	Frame->SetPadding(FMargin(bBoxy ? 4.f : 3.f));
	Frame->SetBrushColor(bBoxy ? Demichrome::Steel() : Demichrome::Pale());

	// Embedded in the HUD rectangle the host owns the size; standalone we're a square feed.
	if (SizeRoot)
	{
		if (bHostSized)
		{
			SizeRoot->ClearWidthOverride();
			SizeRoot->ClearHeightOverride();
		}
		else
		{
			SizeRoot->SetWidthOverride(FeedSize);
			SizeRoot->SetHeightOverride(FeedSize);
		}
	}

	if (FeedImage)
	{
		FSlateBrush Brush;
		Brush.ImageSize = FVector2D(FeedSize, FeedSize);
		if (RenderTarget)
		{
			Brush.SetResourceObject(RenderTarget);
			Brush.DrawAs = ESlateBrushDrawType::Image;
		}
		else
		{
			// No signal: a flat Dark box behind the frame.
			Brush.TintColor = FSlateColor(Demichrome::Dark());
			Brush.DrawAs = ESlateBrushDrawType::Box;
		}
		FeedImage->SetBrush(Brush);
	}

	if (LabelText)
	{
		LabelText->SetText(CamLabel);
	}
}
