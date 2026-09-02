// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

namespace UE::MeshPaintingCore::ColorPicker
{
	inline FLinearColor DarkPanel()
	{
		return FLinearColor(0.035f, 0.038f, 0.043f, 1.0f);
	}

	inline FLinearColor DarkInput()
	{
		return FLinearColor(0.075f, 0.08f, 0.09f, 1.0f);
	}

	inline FLinearColor Border()
	{
		return FLinearColor(0.22f, 0.23f, 0.25f, 1.0f);
	}

	inline FLinearColor HoverBorder()
	{
		return FLinearColor(0.48f, 0.56f, 0.66f, 1.0f);
	}

	inline FLinearColor Accent()
	{
		return FLinearColor(0.18f, 0.48f, 0.95f, 1.0f);
	}

	inline FLinearColor Text()
	{
		return FLinearColor(0.86f, 0.88f, 0.9f, 1.0f);
	}

	inline FSlateBrush MakeColorBrush(const FLinearColor& Color, FVector2D ImageSize = FVector2D(1.0f, 1.0f))
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		Brush.ImageSize = ImageSize;
		return Brush;
	}

	inline FSlateBrush MakeImageBrush(UObject* ResourceObject, FVector2D ImageSize)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(ResourceObject);
		Brush.ImageSize = ImageSize;
		return Brush;
	}

	inline void DrawSolidRect(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& Geometry,
		const FVector2f& Position,
		const FVector2f& Size,
		const FLinearColor& Color,
		ESlateDrawEffect DrawEffects = ESlateDrawEffect::None)
	{
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			WhiteBrush,
			DrawEffects,
			Color);
	}

	inline void DrawRoundedFill(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& Geometry,
		const FVector2f& Position,
		const FVector2f& Size,
		const FLinearColor& Color,
		float Radius,
		ESlateDrawEffect DrawEffects = ESlateDrawEffect::None)
	{
		TArray<FSlateGradientStop> Stops;
		Stops.Emplace(FVector2f(0.0f, 0.0f), Color);
		Stops.Emplace(FVector2f(Size.X, 0.0f), Color);
		FSlateDrawElement::MakeGradient(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			Stops,
			Orient_Horizontal,
			DrawEffects,
			FVector4f(Radius));
	}

	inline void DrawRoundedBorderedFill(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& Geometry,
		const FVector2f& Position,
		const FVector2f& Size,
		const FLinearColor& FillColor,
		const FLinearColor& BorderColor,
		float BorderThickness,
		float Radius,
		ESlateDrawEffect DrawEffects = ESlateDrawEffect::None)
	{
		DrawRoundedFill(OutDrawElements, LayerId, Geometry, Position, Size, BorderColor, Radius, DrawEffects);

		const float Inset = FMath::Max(0.0f, BorderThickness);
		if (Size.X > Inset * 2.0f && Size.Y > Inset * 2.0f)
		{
			DrawRoundedFill(
				OutDrawElements,
				LayerId + 1,
				Geometry,
				Position + FVector2f(Inset, Inset),
				Size - FVector2f(Inset * 2.0f, Inset * 2.0f),
				FillColor,
				FMath::Max(0.0f, Radius - Inset),
				DrawEffects);
		}
	}

	inline void DrawCheckerboard(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& Geometry,
		const FVector2f& Position,
		const FVector2f& Size,
		float CellSize,
		ESlateDrawEffect DrawEffects = ESlateDrawEffect::None)
	{
		const float SafeCellSize = FMath::Max(2.0f, CellSize);
		const int32 Columns = FMath::CeilToInt(Size.X / SafeCellSize);
		const int32 Rows = FMath::CeilToInt(Size.Y / SafeCellSize);
		const FLinearColor Light(0.72f, 0.72f, 0.72f, 1.0f);
		const FLinearColor Dark(0.42f, 0.42f, 0.42f, 1.0f);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			for (int32 Column = 0; Column < Columns; ++Column)
			{
				const FVector2f CellPosition = Position + FVector2f(Column * SafeCellSize, Row * SafeCellSize);
				const FVector2f CellSizeActual(
					FMath::Min(SafeCellSize, Size.X - Column * SafeCellSize),
					FMath::Min(SafeCellSize, Size.Y - Row * SafeCellSize));
				DrawSolidRect(
					OutDrawElements,
					LayerId,
					Geometry,
					CellPosition,
					CellSizeActual,
					((Row + Column) % 2 == 0) ? Light : Dark,
					DrawEffects);
			}
		}
	}

	inline void DrawCircleLines(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& Geometry,
		const FVector2f& Center,
		float Radius,
		const FLinearColor& Color,
		float Thickness,
		int32 Segments = 32,
		ESlateDrawEffect DrawEffects = ESlateDrawEffect::None)
	{
		TArray<FVector2f> Points;
		Points.Reserve(Segments + 1);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = 2.0f * PI * Index / Segments;
			Points.Emplace(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(),
			MoveTemp(Points),
			DrawEffects,
			Color,
			true,
			Thickness);
	}

	inline FLinearColor ColorFromHSV(float Hue, float Saturation, float Value, float Alpha)
	{
		FLinearColor HSVColor(Hue, Saturation, Value, Alpha);
		FLinearColor RGBColor = HSVColor.HSVToLinearRGB();
		RGBColor.A = Alpha;
		return RGBColor;
	}
}
