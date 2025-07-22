#include "BigCoinItem.h"

ABigCoinItem::ABigCoinItem()
{
	PointValue = 50;
	ItemType = "BigCoin";
}

void ABigCoinItem::Activateitem(AActor* Activator)
{
	Super::Activateitem(Activator);
}

