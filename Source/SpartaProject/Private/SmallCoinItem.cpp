#include "SmallCoinItem.h"

ASmallCoinItem::ASmallCoinItem()
{
	PointValue = 10;
	ItemType = "SmallCoin";
}

void ASmallCoinItem::Activateitem(AActor* Activator)
{
	Super::Activateitem(Activator);
}