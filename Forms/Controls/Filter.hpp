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

#ifndef FILTER_HPP
#define FILTER_HPP

#include <Core/Util/Global.hpp>
#include <Core/Gen3/EncounterArea3.hpp>
#include <QWidget>

enum Game : u16;

enum Controls : u16
{
    IVs = 1 << 0,
    Ability = 1 << 1,
    Gender = 1 << 2,
    GenderRatio = 1 << 3,
    EncounterSlots = 1 << 4,
    HiddenPowers = 1 << 5,
    Natures = 1 << 6,
    Shiny = 1 << 7,
    UseDelay = 1 << 8,
    DisableFilter = 1 << 9,
    FilterGame = 1 << 10,
    FilterEncounter = 1 << 11,
    FilterLocation = 1 << 12,
    FilterPokemon = 1 << 13,
    FilterLead = 1 << 14
};

namespace Ui
{
    class Filter;
}

class Filter  : public QWidget
{
    Q_OBJECT
public:
    explicit Filter(QWidget *parent = nullptr);
    ~Filter() override ;

    QVector<u8> getMinIVs() const;
    QVector<u8> getMaxIVs() const;
    u16 getTID() const;
    u16 getSID() const;
    u8 getAbility() const;
    u8 getGender() const;
    QVector<u32> getSeeds() const;
    u8 getGenderRatio() const;
    QVector<bool> getEncounterSlots();
    void setEncounterSlots(const QStringList &encounterSlots) const;
    void toggleEncounterSlots(const QVector<bool> &encounterSlots) const;
    QVector<EncounterArea3> encounterGenerator;
    void resetEncounterSlots() const;
    QVector<bool> getHiddenPowers();
    QVector<bool> getNatures();
    u8 getShiny() const;
    bool useDelay() const;
    u32 getDelay() const;
    Game getGame() const;
    int getEncounter() const;
    EncounterArea3 getLocIndex() const;
    QString getLeadText() const;
    int getIntLead()const;
    int getLeadIndex()const;
    bool getDisableFilters();
    void disableControls(u16 control);
    void setSearchNature(int &calcNature);
    void resetNatures();
    void changeHP(int min, int max);
    void changeAtk(int min, int max);
    void changeDef(int min, int max);
    void changeSpA(int min, int max);
    void changeSpD(int min, int max);
    void changeSpe(int min, int max);
    void enableControls();
    void updateLocationsGenerator();
    void generatorLead();
    void updatePokemonGenerator();
    void generatorEncounterIndexChanged(int index);
    void generatorLocationIndexChanged(int index);
    void generatorPokemonIndexChanged(int index);
    void updateEncounterList();

private:
    Ui::Filter *ui;

};

#endif // FILTER_HPP
