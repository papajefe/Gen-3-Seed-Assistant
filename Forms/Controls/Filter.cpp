/*
 * This file is part of PokéFinder
 * Copyright (C) 2017-2020 by Admiral_Fish, bumba, and EzPzStreamz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "Filter.hpp"
#include "ui_Filter.h"
#include <Core/Util/Translator.hpp>
#include <Core/Enum/Encounter.hpp>
#include <Core/Gen3/Encounters3.hpp>
#include <Core/Enum/Game.hpp>
#include <Core/Enum/Lead.hpp>
#include <Core/Enum/Method.hpp>
#include <Core/Enum/Game.hpp>
#include <Core/Gen3/Generators/WildGenerator3.hpp>
#include <Core/Gen3/ProfileLoader3.hpp>
#include <Core/Gen3/Searchers/WildSearcher3.hpp>
#include <Core/Parents/Frames/WildFrame.hpp>
#include <Core/Util/Nature.hpp>
#include <Core/Util/Translator.hpp>
#include <Forms/Gen3/Profile/ProfileManager3.hpp>
#include <Forms/Gen3/Tools/SeedTime3.hpp>
#include <Models/Gen3/WildModel3.hpp>
#include <QClipboard>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QClipboard>
#include <QCompleter>
#include <QMessageBox>



Filter::Filter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Filter)
{

    ui->setupUi(this);
    generatorLead();
    updateLocationsGenerator();
    ui->comboBoxGame->setItemData(0, Game::Ruby);
    ui->comboBoxGame->setItemData(1, Game::Sapphire);
    ui->comboBoxGame->setItemData(2, Game::FireRed);
    ui->comboBoxGame->setItemData(3, Game::LeafGreen);
    ui->comboBoxGame->setItemData(4, Game::Emerald);
    ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
    ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
    ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
    ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
    ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
    ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);
    ui->comboBoxGeneratorEncounter->addItem(tr("Safari Zone"), Encounter::SafariZone);
    ui->comboBoxAbility->setup({ 255, 0, 1 });
    ui->comboBoxGender->setup({ 255, 0, 1 });
    ui->comboBoxGenderRatio->setup({ 255, 127, 191, 63, 31, 0, 254 });
    ui->checkListHiddenPower->setup(Translator::getHiddenPowers());
    ui->checkListNature->setup(Translator::getNatures());
    ui->comboBoxShiny->setup({ 255, 1, 2, 3 });
    ui->textBoxDelay->setValues(InputType::Frame32Bit);
    connect(ui->comboBoxGame, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Filter::updateEncounterList);
    connect(ui->pushButtonEncounterSlotAny, &QPushButton::clicked, ui->checkListEncounterSlot, &CheckList::resetChecks);
    connect(ui->pushButtonHiddenPowerAny, &QPushButton::clicked, ui->checkListHiddenPower, &CheckList::resetChecks);
    connect(ui->pushButtonNatureAny, &QPushButton::clicked, ui->checkListNature, &CheckList::resetChecks);
    connect(ui->pushButtonGeneratorLead, &QPushButton::clicked, this, &Filter::generatorLead);
    connect(ui->comboBoxGeneratorEncounter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Filter::generatorEncounterIndexChanged);
    connect(ui->comboBoxGeneratorLocation, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Filter::generatorLocationIndexChanged);
    connect(ui->comboBoxGeneratorPokemon, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &Filter::generatorPokemonIndexChanged);
    generatorEncounterIndexChanged(0);

}

Filter::~Filter()
{
    delete ui;
}

QVector<u8> Filter::getMinIVs() const
{
    return ui->ivFilter->getLower();
}

u16 Filter::getTID() const
{
    QString TID = ui->TID->text();
    u16 uTID = TID.toUShort(nullptr);
    return uTID;
}

u16 Filter::getSID() const
{
    QString SID = ui->SID->text();
    u16 uSID = SID.toUShort(nullptr);
    return uSID;
}

QVector<u8> Filter::getMaxIVs() const
{
    return ui->ivFilter->getUpper();
}

u8 Filter::getAbility() const
{
    return ui->comboBoxAbility->getCurrentByte();
}

u8 Filter::getGender() const
{
    return ui->comboBoxGender->getCurrentByte();
}

u8 Filter::getGenderRatio() const
{
    return ui->comboBoxGenderRatio->getCurrentByte();
}

QVector<bool> Filter::getEncounterSlots()
{
    return ui->checkListEncounterSlot->getChecked();
}

void Filter::setEncounterSlots(const QStringList &encounterSlots) const
{
    ui->checkListEncounterSlot->setup(encounterSlots);
}

void Filter::toggleEncounterSlots(const QVector<bool> &encounterSlots) const
{
    ui->checkListEncounterSlot->setChecks(encounterSlots);
}

void Filter::resetEncounterSlots() const
{
    ui->checkListEncounterSlot->resetChecks();
}

QVector<bool> Filter::getHiddenPowers()
{
    return ui->checkListHiddenPower->getChecked();
}

QVector<bool> Filter::getNatures()
{
    return ui->checkListNature->getChecked();
}

void Filter::resetNatures()
{
    ui->checkListNature->resetChecks();
}

u8 Filter::getShiny() const
{
    return ui->comboBoxShiny->getCurrentByte();
}

bool Filter::useDelay() const
{
    return ui->checkBoxUseDelay->isChecked();
}

u32 Filter::getDelay() const
{
    return ui->textBoxDelay->getUInt();
}

bool Filter::getDisableFilters()
{
    return ui->checkBoxDisableFilters->isChecked();
}

void Filter::setSearchNature(int &calcNature)
{
    ui->checkListNature->manualCheck(calcNature);
}

void Filter::disableControls(u16 control)
{
    if (control & Controls::IVs)
    {
        ui->ivFilter->setVisible(false);
    }

    if (control & Controls::Ability)
    {
        ui->labelAbility->setVisible(false);
        ui->comboBoxAbility->setVisible(false);
    }

    if (control & Controls::Gender)
    {
        ui->labelGender->setVisible(false);
        ui->comboBoxGender->setVisible(false);
    }

    if (control & Controls::GenderRatio)
    {
        ui->labelGenderRatio->setVisible(false);
        ui->comboBoxGenderRatio->setVisible(false);
    }

    if (control & Controls::EncounterSlots)
    {
        ui->labelEncounterSlot->setVisible(false);
        ui->checkListEncounterSlot->setVisible(false);
        ui->pushButtonEncounterSlotAny->setVisible(false);
    }

    if (control & Controls::HiddenPowers)
    {
        ui->labelHiddenPower->setVisible(false);
        ui->checkListHiddenPower->setVisible(false);
        ui->pushButtonHiddenPowerAny->setVisible(false);
    }

    if (control & Controls::Natures)
    {
        ui->labelNature->setVisible(false);
        ui->checkListNature->setVisible(false);
        ui->pushButtonNatureAny->setVisible(false);
    }

    if (control & Controls::Shiny)
    {
        ui->labelShiny->setVisible(false);
        ui->comboBoxShiny->setVisible(false);
    }

    if (control & Controls::UseDelay)
    {
        ui->checkBoxUseDelay->setVisible(false);
        ui->textBoxDelay->setVisible(false);
    }

    if (control & Controls::DisableFilter)
    {
        ui->checkBoxDisableFilters->setVisible(false);
    }

    if (control & Controls::FilterGame)
    {
        ui->comboBoxGame->setVisible(false);
        ui->labelGame->setVisible(false);
    }

    if (control & Controls::FilterEncounter)
    {
        ui->comboBoxGeneratorEncounter->setVisible(false);
        ui->labelEncounter->setVisible(false);
    }

    if (control & Controls::FilterLocation)
    {
        ui->comboBoxGeneratorLocation->setVisible(false);
        ui->labelLocation->setVisible(false);
    }

    if (control & Controls::FilterPokemon)
    {
        ui->comboBoxGeneratorPokemon->setVisible(false);
        ui->labelPokemon->setVisible(false);
    }

    if (control & Controls::FilterLead)
    {
        ui->comboBoxGeneratorLead->setVisible(false);
        ui->pushButtonGeneratorLead->setVisible(false);
    }
}

void Filter::changeHP(int min, int max)
{
    ui->ivFilter->changeHP(min, max);
}

void Filter::changeAtk(int min, int max)
{
    ui->ivFilter->changeAtk(min, max);
}

void Filter::changeDef(int min, int max)
{
    ui->ivFilter->changeDef(min, max);
}

void Filter::changeSpA(int min, int max)
{
   ui->ivFilter->changeSpA(min, max);
}

void Filter::changeSpD(int min, int max)
{
   ui->ivFilter->changeSpD(min, max);
}
void Filter::changeSpe(int min, int max)
{
    ui->ivFilter->changeSpe(min, max);
}
void Filter::enableControls()
{
        ui->ivFilter->setVisible(true);
        ui->labelAbility->setVisible(true);
        ui->comboBoxAbility->setVisible(true);
        ui->labelGender->setVisible(true);
        ui->comboBoxGender->setVisible(true);
        ui->labelGenderRatio->setVisible(true);
        ui->comboBoxGenderRatio->setVisible(true);
        ui->labelEncounterSlot->setVisible(true);
        ui->checkListEncounterSlot->setVisible(true);
        ui->pushButtonEncounterSlotAny->setVisible(true);
        ui->labelHiddenPower->setVisible(true);
        ui->checkListHiddenPower->setVisible(true);
        ui->pushButtonHiddenPowerAny->setVisible(true);
        ui->labelNature->setVisible(true);
        ui->checkListNature->setVisible(true);
        ui->pushButtonNatureAny->setVisible(true);
        ui->labelShiny->setVisible(true);
        ui->comboBoxShiny->setVisible(true);
        ui->checkBoxUseDelay->setVisible(true);
        ui->textBoxDelay->setVisible(true);
        ui->checkBoxDisableFilters->setVisible(true);
        ui->comboBoxGame->setVisible(true);
        ui->labelGame->setVisible(true);
        ui->comboBoxGeneratorEncounter->setVisible(true);
        ui->labelEncounter->setVisible(true);
        ui->comboBoxGeneratorLocation->setVisible(true);
        ui->labelLocation->setVisible(true);
        ui->comboBoxGeneratorPokemon->setVisible(true);
        ui->labelPokemon->setVisible(true);
        ui->comboBoxGeneratorLead->setVisible(true);
        ui->pushButtonGeneratorLead->setVisible(true);
}
Game Filter::getGame() const{
Game game = static_cast<Game>(ui->comboBoxGame->currentData().toInt());
return game;
}

int Filter::getEncounter() const{
int enc = ui->comboBoxGeneratorEncounter->currentData().toInt();
return enc;
}

QString Filter::getLeadText() const{
QString text = ui->pushButtonGeneratorLead->text();
return text;
}

int Filter::getIntLead()const{
int leadInt = ui->comboBoxGeneratorLead->currentData().toInt();
return leadInt;
}

int Filter::getLeadIndex()const{
int leadIndex = ui->comboBoxGeneratorLead->currentIndex();
return leadIndex;
}

auto Filter::getLocIndex() const{
    auto locIndex = encounterGenerator.at(ui->comboBoxGeneratorLocation->currentIndex());
    return locIndex;
    }

EncounterArea3 Filter::getLocData() const{
EncounterArea3 area = encounterGenerator[ui->comboBoxGeneratorLocation->currentIndex()];
return area;
}

void Filter::updateLocationsGenerator()
{
    auto encounter = static_cast<Encounter>(ui->comboBoxGeneratorEncounter->currentData().toInt());
    Game game = getGame();
    encounterGenerator = Encounters3::getEncounters(encounter, game);
    QVector<u8> locs;
    for (const auto &area : encounterGenerator)
    {
        locs.append(area.getLocation());
    }

    QStringList locations = Translator::getLocations(locs, game);

    ui->comboBoxGeneratorLocation->clear();
    ui->comboBoxGeneratorLocation->addItems(locations);
}
void Filter::updatePokemonGenerator()
{
    auto area = getLocIndex();
    QVector<u16> species = area.getUniqueSpecies();

    QStringList names = area.getSpecieNames();

    ui->comboBoxGeneratorPokemon->clear();
    ui->comboBoxGeneratorPokemon->addItem("-");
    for (int i = 0; i < species.size(); i++)
    {
        ui->comboBoxGeneratorPokemon->addItem(names.at(i), species.at(i));
    }
}

void Filter::generatorLead()
{
    ui->comboBoxGeneratorLead->clear();
    QString text = ui->pushButtonGeneratorLead->text();
    if (text == tr("Synchronize"))
    {
        ui->pushButtonGeneratorLead->setText(tr("Cute Charm"));

        ui->comboBoxGeneratorLead->addItem(tr("♂ Lead (50% ♀ Target)"), Lead::CuteCharm50F);
        ui->comboBoxGeneratorLead->addItem(tr("♂ Lead (75% ♀ Target)"), Lead::CuteCharm75F);
        ui->comboBoxGeneratorLead->addItem(tr("♂ Lead (25% ♀ Target)"), Lead::CuteCharm25F);
        ui->comboBoxGeneratorLead->addItem(tr("♂ Lead (12.5% ♀ Target)"), Lead::CuteCharm125F);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (50% ♂ Target)"), Lead::CuteCharm50M);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (75% ♂ Target)"), Lead::CuteCharm75M);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (25% ♂ Target)"), Lead::CuteCharm25M);
        ui->comboBoxGeneratorLead->addItem(tr("♀ Lead (87.5% ♂ Target)"), Lead::CuteCharm875M);
    }
    else
    {
        ui->pushButtonGeneratorLead->setText(tr("Synchronize"));

        ui->comboBoxGeneratorLead->addItem("None");
        ui->comboBoxGeneratorLead->addItems(Translator::getNatures());
    }
}


void Filter::generatorEncounterIndexChanged(int index)
{
    if (index >= 0)
    {

        QStringList t;
        Encounter encounter = static_cast<Encounter>(ui->comboBoxGeneratorEncounter->currentData().toInt());

        switch (encounter)
        {
        case Encounter::Grass:
        case Encounter::SafariZone:
            t = QStringList({ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11" });
            break;
        case Encounter::RockSmash:
        case Encounter::Surfing:
        case Encounter::SuperRod:
            t = QStringList({ "0", "1", "2", "3", "4" });
            break;
        case Encounter::OldRod:
            t = QStringList({ "0", "1" });
            break;
        case Encounter::GoodRod:
            t = QStringList({ "0", "1", "2" });
            break;
        default:
            break;
        }

        setEncounterSlots(t);
        updateLocationsGenerator();
    }
}


void Filter::generatorLocationIndexChanged(int index)
{
    if (index >= 0)
    {
        updatePokemonGenerator();
    }
}



void Filter::generatorPokemonIndexChanged(int index)
{
    if (index <= 0)
    {
        resetEncounterSlots();
    }
    else
    {
        u16 num = static_cast<u16>(ui->comboBoxGeneratorPokemon->currentData().toUInt());
        QVector<bool> flags = getLocIndex().getSlots(num);

        toggleEncounterSlots(flags);
    }
}

void Filter::updateEncounterList(){
    ui->comboBoxGeneratorEncounter->clear();
    Game game = getGame();
    switch (game)
    {
    case Game::Ruby:
        ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
        ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Safari Zone"), Encounter::SafariZone);
        updateLocationsGenerator();
        break;
    case Game::Sapphire:
        ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
        ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Safari Zone"), Encounter::SafariZone);
        updateLocationsGenerator();
        break;
    case Game::Emerald:
        ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
        ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Safari Zone"), Encounter::SafariZone);
        updateLocationsGenerator();
        break;
    case Game::FireRed:
        ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
        ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);
        updateLocationsGenerator();
        break;
    case Game::LeafGreen:
        ui->comboBoxGeneratorEncounter->addItem(tr("Grass"), Encounter::Grass);
        ui->comboBoxGeneratorEncounter->addItem(tr("Rock Smash"), Encounter::RockSmash);
        ui->comboBoxGeneratorEncounter->addItem(tr("Surfing"), Encounter::Surfing);
        ui->comboBoxGeneratorEncounter->addItem(tr("Old Rod"), Encounter::OldRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Good Rod"), Encounter::GoodRod);
        ui->comboBoxGeneratorEncounter->addItem(tr("Super Rod"), Encounter::SuperRod);
        updateLocationsGenerator();
        break;
    default:
        break;
    }
}
