#include "ProjectKC/Item/Component/KCItemOutlineComponent.h"

#include "Components/PrimitiveComponent.h"

UKCItemOutlineComponent::UKCItemOutlineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKCItemOutlineComponent::SetOutlineMesh(UPrimitiveComponent* InOutlineMesh)
{
	OutlineMesh = InOutlineMesh;
}

void UKCItemOutlineComponent::ApplyDefaultOutline()
{
	if (!OutlineMesh || !bUseOutline)
	{
		return;
	}

	OutlineMesh->SetRenderCustomDepth(true);
	OutlineMesh->SetCustomDepthStencilValue(DefaultOutlineStencilValue);
}

void UKCItemOutlineComponent::ApplyInteractionOutlineForTeam(int32 TeamId)
{
	if (!OutlineMesh || !bUseOutline)
	{
		ApplyDefaultOutline();
		return;
	}

	OutlineMesh->SetRenderCustomDepth(true);
	OutlineMesh->SetCustomDepthStencilValue(ResolveTeamOutlineStencilValue(TeamId));
}

void UKCItemOutlineComponent::DisableOutline()
{
	if (!OutlineMesh)
	{
		return;
	}

	OutlineMesh->SetRenderCustomDepth(false);
}

int32 UKCItemOutlineComponent::ResolveTeamOutlineStencilValue(int32 TeamId) const
{
	if (TeamId == 0)
	{
		return Team0OutlineStencilValue;
	}

	if (TeamId == 1)
	{
		return Team1OutlineStencilValue;
	}

	return DefaultOutlineStencilValue;
}