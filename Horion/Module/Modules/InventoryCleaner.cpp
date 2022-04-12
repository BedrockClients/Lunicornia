#include "InventoryCleaner.h"
#include "../ModuleManager.h"

InventoryCleaner::InventoryCleaner() : IModule(0x0, Category::AUTO, "Automatically throws not needed stuff out of your inventory")
{
	registerBoolSetting("Tools", &this->keepTools, this->keepTools);
	registerBoolSetting("Armor", &this->keepArmor, this->keepArmor);
	registerBoolSetting("Food", &this->keepFood, this->keepFood);
	registerBoolSetting("Blocks", &this->keepBlocks, this->keepBlocks);
}


InventoryCleaner::~InventoryCleaner()
{
}

const char* InventoryCleaner::getModuleName()
{
	return ("InventoryCleaner");
}

void InventoryCleaner::onTick(C_GameMode* gm)
{
	if (g_Data.getLocalPlayer() == nullptr) return;
	if (g_Data.getLocalPlayer()->canOpenContainerScreen() || moduleMgr->getModule<ChestStealer>()->chestScreenController != nullptr) return;

	/*items.clear();
	uselessItems.clear();
	stackableSlot.clear();*/

	/*findStackableItems();   //  ---     causes game crashes
	if (!stackableSlot.empty()) {
		C_InventoryTransactionManager* manager = g_Data.getLocalPlayer()->getTransactionManager();
		manager->addInventoryAction(C_InventoryAction(stackableSlot.at(0), g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(stackableSlot.at(0)), nullptr, 632));
	}*/

	std::vector<int> dropSlots = findUselessItems();
	if (!dropSlots.empty()) {
		for (int i : dropSlots) {
			logF("Dropping slot: %i", i);
			g_Data.getLocalPlayer()->getSupplies()->inventory->dropSlot(i); 
		}
		logF("########");
	}
}

void InventoryCleaner::findStackableItems() {
	/*for (int i = 0; i < 36; i++) {
		C_ItemStack* itemStack = g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(i);
		if (itemStack->item != nullptr) {
			if ((*itemStack->item)->getMaxStackSize() > itemStack->count) {
				for (int i2 = 0; i2 < 36; i2++) {
					if (i2 == i) continue;
					C_ItemStack* itemStack2 = g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(i2);
					if ((*itemStack2->item)->getMaxStackSize() > itemStack->count) {
						if (*itemStack->item == *itemStack2->item) {
							if ((std::find(stackableSlot.begin(), stackableSlot.end(), i2) == stackableSlot.end())) stackableSlot.push_back(i2);
							if ((std::find(stackableSlot.begin(), stackableSlot.end(), i) == stackableSlot.end())) stackableSlot.push_back(i);
						}
					}
				}
			}
		}
	}*/
}

std::vector<int> InventoryCleaner::findUselessItems() {
	// Filter by options

	std::vector<int> uselessItems;
	std::vector<C_ItemStack*> items;

	{
		for (int i = 0; i < 36; i++) {
			C_ItemStack* itemStack = g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(i);
			if (itemStack->item != nullptr) {
				if (!stackIsUseful(itemStack)) {
					if (std::find(items.begin(), items.end(), itemStack) == items.end())
						uselessItems.push_back(i);
					else
						items.push_back(itemStack);
				}else if (std::find(items.begin(), items.end(), itemStack) == items.end())
					items.push_back(itemStack);
			}
		}
		
		for (int i = 0; i < 4; i++) {
			if (g_Data.getLocalPlayer()->getArmor(i)->item != nullptr)
				items.push_back(g_Data.getLocalPlayer()->getArmor(i));
		}
		
	}
	// Filter weapons
	if(items.size() > 0)
	{
		// Filter by attack damage
		std::sort(items.begin(), items.end(), [](const C_ItemStack* lhs, const C_ItemStack* rhs) {
				C_ItemStack* current = const_cast<C_ItemStack*>(lhs);
				C_ItemStack* other = const_cast<C_ItemStack*>(rhs);
				return current->getAttackingDamageWithEnchants() > other->getAttackingDamageWithEnchants();
		});

		bool hadTheBestItem = false;
		C_ItemStack* bestItem = items.at(0);
		for (int i = 0; i < 36; i++) {
			if (std::find(uselessItems.begin(), uselessItems.end(), i) != uselessItems.end())
				continue;
			C_ItemStack* itemStack = g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(i);
			if (itemStack->item != nullptr && itemStack->getAttackingDamageWithEnchants() > 1) {
				if (itemStack->getAttackingDamageWithEnchants() < bestItem->getAttackingDamageWithEnchants()) {
					uselessItems.push_back(i);
				} else {
					// Damage same as bestItem
					if (hadTheBestItem)
						uselessItems.push_back(i);
					else
						hadTheBestItem = true;
				}
			}
		}
	}
	// Filter armor
	{
		std::vector<C_ItemStack*> helmets;
		std::vector<C_ItemStack*> chestplates;
		std::vector<C_ItemStack*> leggins;
		std::vector<C_ItemStack*> boots;

		// Filter by armor value
		std::sort(items.begin(), items.end(), [](const C_ItemStack* lhs, const C_ItemStack* rhs)
		{
			C_ItemStack* current = const_cast<C_ItemStack*>(lhs);
			C_ItemStack* other = const_cast<C_ItemStack*>(rhs);
			return current->getArmorValueWithEnchants() > other->getArmorValueWithEnchants();
		});
		
		// Put armor items in their respective vectors
		for (C_ItemStack* itemsteck : items)
		{
			C_Item* item = itemsteck->getItem();
			if (item->isArmor()) {
				C_ArmorItem* armorItem = reinterpret_cast<C_ArmorItem*>(item);
				if (armorItem->isHelmet()) helmets.push_back(itemsteck);
				else if (armorItem->isChestplate()) chestplates.push_back(itemsteck);
				else if (armorItem->isLeggins()) leggins.push_back(itemsteck);
				else if (armorItem->isBoots()) boots.push_back(itemsteck);
			}
		}
		bool hadBest[4] = { 0, 0, 0, 0 };
		for (int i = 0; i < 4; i++) {
			C_ItemStack* itemsteck = g_Data.getLocalPlayer()->getArmor(i);
			C_Item** item = itemsteck->item;
			if (item != nullptr) {
				C_ArmorItem* armor = reinterpret_cast<C_ArmorItem*>(*item);
				float testArmorValue = 0;
				switch (armor->ArmorSlot) {
				case 0:
					if(helmets.size() > 0)
						testArmorValue = helmets.at(0)->getArmorValueWithEnchants();
					break;
				case 1:
					if (chestplates.size() > 0)
						testArmorValue = chestplates.at(0)->getArmorValueWithEnchants();
					break;
				case 2:
					if (leggins.size() > 0)
						testArmorValue = leggins.at(0)->getArmorValueWithEnchants();
					break;
				case 3:
					if (boots.size() > 0)
						testArmorValue = boots.at(0)->getArmorValueWithEnchants();
					break;
				}
				if (armor->getArmorValue() >= testArmorValue)
					hadBest[armor->ArmorSlot] = true;
			}
		}
		for (int i = 0; i < 36; i++) {
			if (std::find(uselessItems.begin(), uselessItems.end(), i) != uselessItems.end())
				continue; // item already useless
			C_ItemStack* itemStack = g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(i);
			if (itemStack->item != nullptr && (*itemStack->item)->isArmor()) {
				C_ArmorItem* armor = reinterpret_cast<C_ArmorItem*>(*itemStack->item);
				if (armor->isHelmet()) {
					if (hadBest[0] || itemStack->getArmorValueWithEnchants() < helmets.at(0)->getArmorValueWithEnchants()) {
						uselessItems.push_back(i);
					}else 
						hadBest[0] = true;
				}
				else if (armor->isChestplate()) {
					if (hadBest[1] || itemStack->getArmorValueWithEnchants() < chestplates.at(0)->getArmorValueWithEnchants())
						uselessItems.push_back(i);
					else
						hadBest[1] = true;
				}
				else if (armor->isLeggins()) {
					if (hadBest[2] || itemStack->getArmorValueWithEnchants() < leggins.at(0)->getArmorValueWithEnchants())
						uselessItems.push_back(i);
					else
						hadBest[2] = true;
				}
				else if (armor->isBoots()) {
					if (hadBest[3] || itemStack->getArmorValueWithEnchants() < boots.at(0)->getArmorValueWithEnchants())
						uselessItems.push_back(i);
					else
						hadBest[3] = true;
				}
			}
		}
	}

	return uselessItems;
}

bool InventoryCleaner::stackIsUseful(C_ItemStack* itemStack) {
	if (itemStack->item == nullptr) return true;
	if (keepArmor && (*itemStack->item)->isArmor()) return true; // Armor
	if (keepTools && (*itemStack->item)->isTool()) return true; // Tools
	if (keepFood && (*itemStack->item)->isFood()) return true; // Food
	if (keepBlocks && (*itemStack->item)->isBlock()) return true; // Block
	if (keepTools && (*itemStack->item)->itemId == 368) return true; // Ender Pearl
	return false;
}

bool InventoryCleaner::isLastItem(C_Item* item) {
	std::vector<C_Item*> items;
	for (int i = 0; i < 36; i++) {
		C_ItemStack* stack = g_Data.getLocalPlayer()->getSupplies()->inventory->getItemStack(i);
		if (stack->item != nullptr) items.push_back((*stack->item));
	}
	int count = 0;
	for (C_Item* a : items) {
		if (a == item) count++;
	}
	if (count > 1) return false;
	return true;
}