// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPickerEyedropperUtils.h"

#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "TextureResource.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"

namespace
{
	struct FViewportSampleContext
	{
		FViewport* Viewport = nullptr;
		TSharedPtr<SViewport> ViewportWidget;
		FIntPoint ViewportPixel = FIntPoint::ZeroValue;
		FIntPoint WidgetPixel = FIntPoint::ZeroValue;
		FIntPoint ViewportSize = FIntPoint::ZeroValue;
		FIntPoint WidgetSize = FIntPoint::ZeroValue;
	};

	FLinearColor ClampEyedropperColor01(FLinearColor Color)
	{
		Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
		Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
		Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
		Color.A = 1.0f;
		return Color;
	}

	FIntPoint ClampPixelToSize(const FIntPoint& Pixel, const FIntPoint& Size)
	{
		return FIntPoint(
			FMath::Clamp(Pixel.X, 0, FMath::Max(0, Size.X - 1)),
			FMath::Clamp(Pixel.Y, 0, FMath::Max(0, Size.Y - 1)));
	}

	bool ResolveViewportSampleContext(APlayerController* PlayerController, FViewportSampleContext& OutContext)
	{
		OutContext = FViewportSampleContext();
		if (!PlayerController) return false;

		const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		const UGameViewportClient* GameViewportClient = nullptr;
		if (LocalPlayer && LocalPlayer->ViewportClient)
		{
			GameViewportClient = LocalPlayer->ViewportClient.Get();
		}
		else
		{
			GameViewportClient = PlayerController->GetWorld()->GetGameViewport();
		}
		FViewport* Viewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
		if (!Viewport) return false;

		OutContext.Viewport = Viewport;
		OutContext.ViewportSize = Viewport->GetRenderTargetTextureSizeXY();
		if (OutContext.ViewportSize.X <= 0 || OutContext.ViewportSize.Y <= 0)
		{
			OutContext.ViewportSize = Viewport->GetSizeXY();
		}
		if (OutContext.ViewportSize.X <= 0 || OutContext.ViewportSize.Y <= 0) return false;

		if (FSlateApplication::IsInitialized() && GameViewportClient)
		{
			TSharedPtr<SViewport> ViewportWidget = GameViewportClient->GetGameViewportWidget();
			if (ViewportWidget.IsValid())
			{
				const FGeometry& Geometry = ViewportWidget->GetCachedGeometry();
				const FVector2D LocalMousePosition = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
				const FVector2D LocalSize = Geometry.GetLocalSize();
				if (LocalSize.X > 0.0f && LocalSize.Y > 0.0f &&
					LocalMousePosition.X >= 0.0f && LocalMousePosition.Y >= 0.0f &&
					LocalMousePosition.X < LocalSize.X && LocalMousePosition.Y < LocalSize.Y)
				{
					OutContext.ViewportWidget = ViewportWidget;
					OutContext.ViewportPixel = ClampPixelToSize(
						FIntPoint(
							FMath::FloorToInt((LocalMousePosition.X / LocalSize.X) * OutContext.ViewportSize.X),
							FMath::FloorToInt((LocalMousePosition.Y / LocalSize.Y) * OutContext.ViewportSize.Y)),
						OutContext.ViewportSize);

					OutContext.WidgetSize = FIntPoint(
						FMath::Max(1, FMath::RoundToInt(LocalSize.X * Geometry.Scale)),
						FMath::Max(1, FMath::RoundToInt(LocalSize.Y * Geometry.Scale)));
					OutContext.WidgetPixel = ClampPixelToSize(
						FIntPoint(
							FMath::FloorToInt(LocalMousePosition.X * Geometry.Scale),
							FMath::FloorToInt(LocalMousePosition.Y * Geometry.Scale)),
						OutContext.WidgetSize);
					return true;
				}
			}
		}

		const FIntPoint ViewportClientSize = Viewport->GetSizeXY();
		if (ViewportClientSize.X <= 0 || ViewportClientSize.Y <= 0) return false;

		FIntPoint MousePosition = FIntPoint::ZeroValue;
		Viewport->GetMousePos(MousePosition, true);
		if (MousePosition.X < 0 || MousePosition.Y < 0 ||
			MousePosition.X >= ViewportClientSize.X || MousePosition.Y >= ViewportClientSize.Y)
		{
			float MouseX = 0.0f;
			float MouseY = 0.0f;
			if (!PlayerController->GetMousePosition(MouseX, MouseY)) return false;
			MousePosition = FIntPoint(FMath::FloorToInt(MouseX), FMath::FloorToInt(MouseY));
		}

		if (MousePosition.X < 0 || MousePosition.Y < 0 ||
			MousePosition.X >= ViewportClientSize.X || MousePosition.Y >= ViewportClientSize.Y)
		{
			return false;
		}

		OutContext.ViewportPixel = ClampPixelToSize(
			FIntPoint(
				FMath::FloorToInt((static_cast<float>(MousePosition.X) / ViewportClientSize.X) * OutContext.ViewportSize.X),
				FMath::FloorToInt((static_cast<float>(MousePosition.Y) / ViewportClientSize.Y) * OutContext.ViewportSize.Y)),
			OutContext.ViewportSize);
		return true;
	}

	bool ReadViewportBackBufferPixel(const FViewportSampleContext& Context, FLinearColor& OutColor)
	{
		if (!Context.Viewport) return false;

		TArray<FColor> PixelData;
		PixelData.Reserve(1);

		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(true);
		const FIntRect PixelRect(
			Context.ViewportPixel.X,
			Context.ViewportPixel.Y,
			Context.ViewportPixel.X + 1,
			Context.ViewportPixel.Y + 1);
		if (!Context.Viewport->ReadPixels(PixelData, ReadFlags, PixelRect) || PixelData.Num() == 0) return false;

		OutColor = ClampEyedropperColor01(FLinearColor::FromSRGBColor(PixelData[0]));
		return true;
	}

	bool ReadSlateViewportPixel(const FViewportSampleContext& Context, FLinearColor& OutColor)
	{
		if (!FSlateApplication::IsInitialized() || !Context.ViewportWidget.IsValid()) return false;

		TArray<FColor> PixelData;
		FIntVector CapturedSize = FIntVector::ZeroValue;
		const FIntRect PixelRect(
			Context.WidgetPixel.X,
			Context.WidgetPixel.Y,
			Context.WidgetPixel.X + 1,
			Context.WidgetPixel.Y + 1);

		if (!FSlateApplication::Get().TakeScreenshot(
				Context.ViewportWidget.ToSharedRef(), PixelRect, PixelData, CapturedSize) ||
			PixelData.Num() == 0)
		{
			return false;
		}

		OutColor = ClampEyedropperColor01(FLinearColor::FromSRGBColor(PixelData[0]));
		return true;
	}

	bool SampleViewportColorUnderCursor(
		APlayerController* PlayerController, const FHitResult* HitResult,
		FRuntimeMeshPaintSampleResult& OutSampleResult)
	{
		FViewportSampleContext Context;
		if (!ResolveViewportSampleContext(PlayerController, Context)) return false;

		FLinearColor SampledColor = FLinearColor::Transparent;
		if (!ReadSlateViewportPixel(Context, SampledColor) && !ReadViewportBackBufferPixel(Context, SampledColor))
		{
			return false;
		}

		OutSampleResult = FRuntimeMeshPaintSampleResult();
		OutSampleResult.bSuccess = true;
		OutSampleResult.Color = SampledColor;
		if (HitResult) OutSampleResult.HitResult = *HitResult;
		return true;
	}

	bool IsMeshUnlitSampleMode(ERuntimeMeshPaintColorSampleMode SampleMode)
	{
		return SampleMode == ERuntimeMeshPaintColorSampleMode::MeshUnlitColor ||
			SampleMode == ERuntimeMeshPaintColorSampleMode::MeshUnlitColorThenViewport;
	}

	ERuntimeMeshPaintColorSampleMode NormalizeSampleMode(ERuntimeMeshPaintColorSampleMode SampleMode)
	{
		if (SampleMode == ERuntimeMeshPaintColorSampleMode::MeshUnlitColor ||
			SampleMode == ERuntimeMeshPaintColorSampleMode::ViewportLitColor ||
			SampleMode == ERuntimeMeshPaintColorSampleMode::MeshUnlitColorThenViewport)
		{
			return SampleMode;
		}

		return ERuntimeMeshPaintColorSampleMode::MeshUnlitColor;
	}

	bool SampleMeshUnlitColorUnderCursor(
		APlayerController* PlayerController, const FHitResult& HitResult,
		FRuntimeMeshPaintSampleResult& OutSampleResult)
	{
		FViewportSampleContext Context;
		if (!ResolveViewportSampleContext(PlayerController, Context)) return false;

		UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
		APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
		if (!World || !CameraManager) return false;

		const FMinimalViewInfo& CameraView = CameraManager->GetCameraCacheView();
		const int32 CaptureWidth = FMath::Clamp(Context.ViewportSize.X, 1, 4096);
		const int32 CaptureHeight = FMath::Clamp(Context.ViewportSize.Y, 1, 4096);

		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(PlayerController);
		if (!RenderTarget) return false;

		RenderTarget->RenderTargetFormat = RTF_RGBA8;
		RenderTarget->ClearColor = FLinearColor::Transparent;
		RenderTarget->bAutoGenerateMips = false;
		RenderTarget->InitAutoFormat(CaptureWidth, CaptureHeight);
		RenderTarget->UpdateResourceImmediate(true);

		USceneCaptureComponent2D* SceneCapture = NewObject<USceneCaptureComponent2D>(PlayerController);
		if (!SceneCapture) return false;

		SceneCapture->TextureTarget = RenderTarget;
		SceneCapture->CaptureSource = SCS_BaseColor;
		SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
		SceneCapture->bCaptureEveryFrame = false;
		SceneCapture->bCaptureOnMovement = false;
		SceneCapture->bAlwaysPersistRenderingState = false;
		SceneCapture->CompositeMode = SCCM_Overwrite;
		SceneCapture->ProjectionType = CameraView.ProjectionMode;
		SceneCapture->FOVAngle = CameraView.FOV;
		SceneCapture->OrthoWidth = CameraView.OrthoWidth;
		SceneCapture->PostProcessBlendWeight = 0.0f;
		SceneCapture->SetWorldLocationAndRotation(CameraView.Location, CameraView.Rotation);
		SceneCapture->RegisterComponentWithWorld(World);
		SceneCapture->CaptureScene();

		FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
		if (!RenderTargetResource)
		{
			SceneCapture->DestroyComponent();
			return false;
		}

		const FIntPoint CapturePixel = ClampPixelToSize(
			FIntPoint(
				FMath::FloorToInt((static_cast<float>(Context.ViewportPixel.X) / FMath::Max(1, Context.ViewportSize.X)) * CaptureWidth),
				FMath::FloorToInt((static_cast<float>(Context.ViewportPixel.Y) / FMath::Max(1, Context.ViewportSize.Y)) * CaptureHeight)),
			FIntPoint(CaptureWidth, CaptureHeight));
		const FIntRect PixelRect(CapturePixel.X, CapturePixel.Y, CapturePixel.X + 1, CapturePixel.Y + 1);

		TArray<FColor> PixelData;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		const bool bReadPixel = RenderTargetResource->ReadPixels(PixelData, ReadFlags, PixelRect) && PixelData.Num() > 0;
		SceneCapture->DestroyComponent();
		if (!bReadPixel) return false;

		OutSampleResult = FRuntimeMeshPaintSampleResult();
		OutSampleResult.bSuccess = true;
		OutSampleResult.Color = ClampEyedropperColor01(PixelData[0].ReinterpretAsLinear());
		OutSampleResult.HitResult = HitResult;
		return true;
	}
}

namespace ColorPickerEyedropper
{
	bool SampleUnderCursor(
		APlayerController* PlayerController, ECollisionChannel TraceChannel, bool bTraceComplex,
		UObject* FallbackPaintTarget, FRuntimeMeshPaintSampleResult& OutSampleResult,
		ERuntimeMeshPaintColorSampleMode SampleMode)
	{
		static_cast<void>(FallbackPaintTarget);
		OutSampleResult = FRuntimeMeshPaintSampleResult();
		SampleMode = NormalizeSampleMode(SampleMode);

		if (!PlayerController) return false;

		FHitResult HitResult;
		const ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(TraceChannel);
		const bool bHasHit = PlayerController->GetHitResultUnderCursorByChannel(TraceType, bTraceComplex, HitResult);
		if (IsMeshUnlitSampleMode(SampleMode))
		{
			if (bHasHit && SampleMeshUnlitColorUnderCursor(PlayerController, HitResult, OutSampleResult))
			{
				return true;
			}

			if (SampleMode == ERuntimeMeshPaintColorSampleMode::MeshUnlitColor)
			{
				return false;
			}

			return SampleViewportColorUnderCursor(PlayerController, bHasHit ? &HitResult : nullptr, OutSampleResult);
		}

		if (SampleMode == ERuntimeMeshPaintColorSampleMode::ViewportLitColor)
		{
			return SampleViewportColorUnderCursor(PlayerController, bHasHit ? &HitResult : nullptr, OutSampleResult);
		}

		return SampleViewportColorUnderCursor(PlayerController, bHasHit ? &HitResult : nullptr, OutSampleResult);
	}
}
