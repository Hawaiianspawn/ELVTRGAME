#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

/**
 * Demichrome — the four locked global values (docs/art/aesthetic-direction.md, Direction A).
 *
 * These are sRGB hexes. FLinearColor(FColor) converts sRGB->linear, and Slate converts
 * back to sRGB at draw time, so the on-screen pixel is the exact hex. No fifth value is
 * permitted without an explicit owner exception (red is reserved for cost/decision events,
 * absent from menu/HUD).
 *
 * Value-role map (menu spec §4): Dark = ground/ink, Steel = borders/inactive/spent,
 * Bone = surfaces/body text, Pale = focus/healthy/the single glint.
 */
namespace Demichrome
{
	inline FLinearColor Dark()  { return FLinearColor(FColor(0x21, 0x1E, 0x20, 0xFF)); } // #211e20
	inline FLinearColor Steel() { return FLinearColor(FColor(0x55, 0x55, 0x68, 0xFF)); } // #555568
	inline FLinearColor Bone()  { return FLinearColor(FColor(0xA0, 0xA0, 0x8B, 0xFF)); } // #a0a08b
	inline FLinearColor Pale()  { return FLinearColor(FColor(0xE9, 0xEF, 0xEC, 0xFF)); } // #e9efec

	/** A solid-fill UMG brush in a Demichrome value (DrawAs Box over the default 1x1 white texture). */
	inline FSlateBrush SolidBrush(const FLinearColor& Value)
	{
		FSlateBrush Brush;
		Brush.TintColor = FSlateColor(Value);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		return Brush;
	}
}
