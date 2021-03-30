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

#ifndef STATIONARY3_H
#define STATIONARY3_H
#include <QLabel>
#include <QTextEdit>

#include <Core/Gen3/Profile3.hpp>
#include <QMenu>

class Frame;
class StationaryGeneratorModel3;
class WildGeneratorModel3;
class PersonalInfo;

namespace Ui
{
    class Stationary3;
}

class Stationary3 : public QWidget
{
    Q_OBJECT
signals:





public:
    explicit Stationary3(QWidget *parent = nullptr);
    ~Stationary3() override;

private:
    Ui::Stationary3 *ui;
    QVector<PersonalInfo> personalInfo;
    StationaryGeneratorModel3 *stationaryModel = nullptr;
    WildGeneratorModel3 *wildModel = nullptr;
    QMenu *generatorMenu = nullptr;
    PersonalInfo getPersonalInfo(const PersonalInfo &base);
    void setupModels();

private slots:
    void displayIVs(QStringList &label, const QVector<u8> &ivs);
    void findIVs();
    void calculatestats();
    void generate();
    void generateWild();
    void tableViewGeneratorContextMenu(QPoint pos);
    void pokemonIndexChanged(int index);
    void altformIndexChanged(int index);
    void methodIndexChanged(int index);
    void generationIndexChanged(int index);

};

#endif // STATIONARY3_H
