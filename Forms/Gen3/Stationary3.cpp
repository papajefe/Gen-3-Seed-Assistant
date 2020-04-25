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

#include "Stationary3.hpp"
#include "Forms/Util/IVCalculator.hpp"
#include "Forms/Controls/Filter.hpp"
#include "Forms/Controls/CheckList.hpp"
#include "ui_Stationary3.h"
#include <Core/Enum/Game.hpp>
#include <Core/Enum/Method.hpp>
#include <Core/Parents/PersonalInfo.hpp>
#include <Core/Util/IVChecker.hpp>
#include <Core/Gen3/Generators/StationaryGenerator3.hpp>
#include <Core/Gen3/ProfileLoader3.hpp>
#include <Core/Gen3/Searchers/StationarySearcher3.hpp>
#include <Core/Util/Translator.hpp>
#include <Forms/Gen3/Profile/ProfileManager3.hpp>
#include <Forms/Gen3/Tools/SeedTime3.hpp>
#include <Models/Gen3/StationaryModel3.hpp>
#include <QClipboard>
#include <QCompleter>
#include <QMessageBox>
#include <QSettings>
#include <QThread>
#include <QTimer>

Stationary3::Stationary3(QWidget *parent) : QWidget(parent), ui(new Ui::Stationary3)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

    updateProfiles();
    setupModels();

    qRegisterMetaType<QVector<Frame>>("QVector<Frame>");
}

Stationary3::~Stationary3()
{
    QSettings setting;
    setting.beginGroup("stationary3");
    setting.setValue("profile", ui->comboBoxProfiles->currentIndex());
    setting.setValue("geometry", this->saveGeometry());
    setting.endGroup();

    delete ui;
}

void Stationary3::updateProfiles()
{
    connect(ui->comboBoxProfiles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Stationary3::profilesIndexChanged);

    profiles = { Profile3() };
    for (const auto &profile : ProfileLoader3::getProfiles())
    {
        if (!(profile.getVersion() & Game::GC))
        {
            profiles.append(profile);
        }
    }

    ui->comboBoxProfiles->clear();
    for (const auto &profile : profiles)
    {
        ui->comboBoxProfiles->addItem(profile.getName());
    }

    QSettings setting;
    int val = setting.value("stationary3/profile", 0).toInt();
    if (val < ui->comboBoxProfiles->count())
    {
        ui->comboBoxProfiles->setCurrentIndex(val);
    }
}

void Stationary3::setupModels()
{
    generatorModel = new StationaryGeneratorModel3(ui->tableViewGenerator);
    searcherModel = new StationarySearcherModel3(ui->tableViewSearcher);
    generatorMenu = new QMenu(ui->tableViewGenerator);
    searcherMenu = new QMenu(ui->tableViewSearcher);
    generationIndexChanged(0);
    ui->comboBoxPokemon->setEditable(true);
    ui->comboBoxPokemon->completer()->setCompletionMode(QCompleter::PopupCompletion);
    connect(ui->comboBoxPokemon, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Stationary3::pokemonIndexChanged);
    connect(ui->comboBoxAltForm, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Stationary3::altformIndexChanged);
    ui->tableViewGenerator->setModel(generatorModel);
    ui->tableViewSearcher->setModel(searcherModel);
    ui->textBoxGeneratorInitialFrame->setValues(InputType::Frame32Bit);
    ui->comboBoxGeneratorMethod->setup({ Method::Method1, Method::Method1Reverse, Method::Method2, Method::Method4 });
    ui->comboBoxSearcherMethod->setup({ Method::Method1, Method::Method1Reverse, Method::Method2, Method::Method4 });
    ui->comboBoxNatures->addItems(Translator::getNatures());
    ui->filterGenerator->disableControls(Controls::EncounterSlots | Controls::Ability | Controls::UseDelay | Controls::DisableFilter | Controls::HiddenPowers | Controls::Shiny);
    ui->filterSearcher->disableControls(Controls::EncounterSlots | Controls::UseDelay | Controls::DisableFilter);

    QAction *outputTXTGenerator = generatorMenu->addAction(tr("Output Results to TXT"));
    QAction *outputCSVGenerator = generatorMenu->addAction(tr("Output Results to CSV"));
    connect(outputTXTGenerator, &QAction::triggered, this, [=] { ui->tableViewGenerator->outputModel(); });
    connect(outputCSVGenerator, &QAction::triggered, this, [=] { ui->tableViewGenerator->outputModel(true); });

    QAction *copySeedToClipboard = searcherMenu->addAction(tr("Copy Seed to Clipboard"));
    QAction *seedToTime = searcherMenu->addAction(tr("Generate times for seed"));
    QAction *outputTXTSearcher = searcherMenu->addAction(tr("Output Results to TXT"));
    QAction *outputCSVSearcher = searcherMenu->addAction(tr("Output Results to CSV"));
    connect(copySeedToClipboard, &QAction::triggered, this, &Stationary3::copySeedToClipboard);
    connect(seedToTime, &QAction::triggered, this, &Stationary3::seedToTime);
    connect(outputTXTSearcher, &QAction::triggered, this, [=] { ui->tableViewSearcher->outputModel(); });
    connect(outputCSVSearcher, &QAction::triggered, this, [=] { ui->tableViewSearcher->outputModel(true); });

    connect(ui->pushButtonGenerate, &QPushButton::clicked, this, &Stationary3::generate);
    connect(ui->pushButtonSearch, &QPushButton::clicked, this, &Stationary3::search);
    connect(ui->CalculateIVButton, &QPushButton::clicked, this, &Stationary3::calculatestats);
    connect(ui->tableViewGenerator, &QTableView::customContextMenuRequested, this, &Stationary3::tableViewGeneratorContextMenu);
    connect(ui->tableViewSearcher, &QTableView::customContextMenuRequested, this, &Stationary3::tableViewSearcherContextMenu);
    connect(ui->pushButtonProfileManager, &QPushButton::clicked, this, &Stationary3::profileManager);

    QSettings setting;
    if (setting.contains("stationary3/geometry"))
    {
        this->restoreGeometry(setting.value("stationary3/geometry").toByteArray());
    }
}

void Stationary3::updateProgress(const QVector<Frame> &frames, int progress)
{
    searcherModel->addItems(frames);
    ui->progressBar->setValue(progress);
}



void Stationary3::generate()
{
    generatorModel->clearModel();
    QVector<u32> seeds;
    QString userInput;
    userInput.clear();
    seeds.clear();
    userInput = ui->textUserSeed->toPlainText();
    QStringList userSeeds = ui->textUserSeed->toPlainText().split("\n");
    if (userInput.isEmpty())
    {
        for (u32 i = 0; i < 65536; i++)
        {
            seeds.append(i);
        }

    }
    else  {
        for (int i = 0; i < userSeeds.size(); i++)
        {
            seeds.append(userSeeds.at(i).toUInt(nullptr, 16));
        }

    }
    for (int i = 0; i < seeds.size(); i++){
    u32 seed = seeds.at(i);
    u32 initialFrame = ui->textBoxGeneratorInitialFrame->getUInt();
    u16 tid = currentProfile.getTID();
    u16 sid = currentProfile.getSID();
    u8 genderRatio = ui->filterGenerator->getGenderRatio();
    auto method = static_cast<Method>(ui->comboBoxGeneratorMethod->getCurrentInt());
    u32 offset = 0;
    if (ui->filterGenerator->useDelay())
    {
        offset = ui->filterGenerator->getDelay();
    }

    FrameFilter filter(ui->filterGenerator->getGender(), ui->filterGenerator->getAbility(), ui->filterGenerator->getShiny(),
                       ui->filterGenerator->getDisableFilters(), ui->filterGenerator->getMinIVs(), ui->filterGenerator->getMaxIVs(),
                       ui->filterGenerator->getNatures(), ui->filterGenerator->getHiddenPowers(), {});

    StationaryGenerator3 generator(initialFrame-10, 21, tid, sid, genderRatio, method, filter);
    generator.setOffset(offset);
    auto frames = generator.generate(seed);
    generatorModel->addItems(frames);
    }
}


void Stationary3::search()
{
    searcherModel->clearModel();

    ui->pushButtonSearch->setEnabled(false);
    ui->pushButtonCancel->setEnabled(true);

    QVector<u8> min = ui->filterSearcher->getMinIVs();
    QVector<u8> max = ui->filterSearcher->getMaxIVs();

    FrameFilter filter(ui->filterSearcher->getGender(), ui->filterSearcher->getAbility(), ui->filterSearcher->getShiny(), false, min, max,
                       ui->filterSearcher->getNatures(), ui->filterSearcher->getHiddenPowers(), {});

    u16 tid = currentProfile.getTID();
    u16 sid = currentProfile.getSID();
    u8 genderRatio = ui->filterSearcher->getGenderRatio();
    auto method = static_cast<Method>(ui->comboBoxSearcherMethod->getCurrentInt());

    auto *searcher = new StationarySearcher3(tid, sid, genderRatio, method, filter);

    int maxProgress = 1;
    for (u8 i = 0; i < 6; i++)
    {
        maxProgress *= max.at(i) - min.at(i) + 1;
    }
    ui->progressBar->setRange(0, maxProgress);

    auto *thread = QThread::create([=] { searcher->startSearch(min, max); });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(ui->pushButtonCancel, &QPushButton::clicked, [searcher] { searcher->cancelSearch(); });

    auto *timer = new QTimer();
    connect(timer, &QTimer::timeout, [=] { updateProgress(searcher->getResults(), searcher->getProgress()); });
    connect(thread, &QThread::finished, timer, &QTimer::stop);
    connect(thread, &QThread::finished, timer, &QTimer::deleteLater);
    connect(timer, &QTimer::destroyed, [=] {
        ui->pushButtonSearch->setEnabled(true);
        ui->pushButtonCancel->setEnabled(false);
        updateProgress(searcher->getResults(), searcher->getProgress());
        delete searcher;
    });

    thread->start();
    timer->start(1000);
}

void Stationary3::profilesIndexChanged(int index)
{
    if (index >= 0)
    {
        currentProfile = profiles.at(index);

        ui->labelProfileTIDValue->setText(QString::number(currentProfile.getTID()));
        ui->labelProfileSIDValue->setText(QString::number(currentProfile.getSID()));
        ui->labelProfileGameValue->setText(currentProfile.getVersionString());
    }
}

void Stationary3::tableViewGeneratorContextMenu(QPoint pos)
{
    if (generatorModel->rowCount() > 0)
    {

        generatorMenu->popup(ui->tableViewGenerator->viewport()->mapToGlobal(pos));
    }
}

void Stationary3::tableViewSearcherContextMenu(QPoint pos)
{
    if (searcherModel->rowCount() > 0)
    {
        searcherMenu->popup(ui->tableViewSearcher->viewport()->mapToGlobal(pos));
    }
}

void Stationary3::seedToTime()
{
    QModelIndex index = ui->tableViewSearcher->currentIndex();
    index = searcherModel->index(index.row(), 0);
    u32 seed = searcherModel->data(index).toString().toUInt(nullptr, 16);

    auto *seedToTime = new SeedTime3(seed);
    seedToTime->show();
    seedToTime->raise();
}

void Stationary3::copySeedToClipboard()
{
    QModelIndex index = ui->tableViewSearcher->currentIndex();
    index = searcherModel->index(index.row(), 0);

    QApplication::clipboard()->setText(searcherModel->data(index).toString());
}

void Stationary3::profileManager()
{
    auto *manager = new ProfileManager3();
    connect(manager, &ProfileManager3::updateProfiles, this, [=] { emit alertProfiles(3); });
    manager->show();
}

void Stationary3::displayIVs(QStringList &label, const QVector<u8> &ivs)
{
    QString result;

    if (ivs.isEmpty())
    {
        result = tr("Invalid");
    }
    else
    {
        bool flag = false;
        for (int i = 0; i < ivs.size(); i++)
        {
            if (i == 0)
            {
                result += QString::number(ivs.at(i));
            }
            else
            {
                if (ivs.at(i) == ivs.at(i - 1) + 1)
                {
                    flag = true;

                    //  Check to see if we need to cap here.
                    if (i == ivs.size() - 1)
                    {
                        result += QString("-%1").arg(ivs.at(i));
                    }
                }
                else
                {
                    if (flag)
                    {
                        flag = false;
                        result += QString("-%1").arg(ivs.at(i - 1));
                        result += QString(", %1").arg(ivs.at(i));
                    }
                    else
                    {
                        result += QString(", %1").arg(ivs.at(i));
                    }
                }
            }
        }
    }

    label.append(result);
}

void Stationary3::findIVs()
{
    QVector<QVector<u16>> stats;
    QVector<u8> levels;

    QStringList entries = ui->textEdit->toPlainText().split("\n");
    bool flag = true;
    for (const QString &entry : entries)
    {
        QStringList values = entry.split(" ");
        if (values.size() != 7)
        {
            flag = false;
            break;
        }

        levels.append(static_cast<u8>(values.at(0).toUInt(&flag)));
        if (!flag)
        {
            break;
        }

        QVector<u16> stat;
        for (u8 i = 1; i < 7; i++)
        {
            stat.append(static_cast<u16>(values.at(i).toUInt(&flag)));
            if (!flag)
            {
                break;
            }
        }
        stats.append(stat);

        if (!flag)
        {
            break;
        }
    }

    if (!flag)
    {
        QMessageBox error;
        error.setText(tr("Invalid input"));
        error.exec();
        return;
    }

    u8 nature = static_cast<u8>(ui->comboBoxNatures->currentIndex());
    u8 hiddenPower = -1;
    u8 characteristic = -1;
    auto base = personalInfo.at(ui->comboBoxPokemon->currentIndex() + 1);

    auto ivs = IVChecker::calculateIVRange(getPersonalInfo(base).getBaseStats(), stats, levels, nature, characteristic, hiddenPower);
    QStringList calcIVs;
    displayIVs(calcIVs, ivs.at(0));
    displayIVs(calcIVs, ivs.at(1));
    displayIVs(calcIVs, ivs.at(2));
    displayIVs(calcIVs, ivs.at(3));
    displayIVs(calcIVs, ivs.at(4));
    displayIVs(calcIVs, ivs.at(5));
    QStringList splitIVs;
    for (int i = 0; i < calcIVs.size(); i++)
    {
    splitIVs.append(calcIVs.at(i).split("\n"));
    }
    QRegExp IVSplitter("[-,]");
    QStringList hp = splitIVs.at(0).split(IVSplitter);
    ui->filterGenerator->changeHP(hp.first().toUInt(), hp.last().toUInt());
    QStringList Atk = splitIVs.at(1).split(IVSplitter);
    ui->filterGenerator->changeAtk(Atk.first().toUInt(), Atk.last().toUInt());
    QStringList Def = splitIVs.at(2).split(IVSplitter);
    ui->filterGenerator->changeDef(Def.first().toUInt(), Def.last().toUInt());
    QStringList SpA = splitIVs.at(3).split(IVSplitter);
    ui->filterGenerator->changeSpA(SpA.first().toUInt(), SpA.last().toUInt());
    QStringList SpD = splitIVs.at(4).split(IVSplitter);
    ui->filterGenerator->changeSpD(SpD.first().toUInt(), SpD.last().toUInt());
    QStringList Spe = splitIVs.at(5).split(IVSplitter);
    ui->filterGenerator->changeSpe(Spe.first().toUInt(), Spe.last().toUInt());

}


void Stationary3::pokemonIndexChanged(int index)
{
    if (index >= 0 && !personalInfo.isEmpty())
    {
        PersonalInfo base = personalInfo.at(index + 1);
        u8 formCount = base.getFormCount();

        ui->labelAltForm->setVisible(formCount > 1);
        ui->comboBoxAltForm->setVisible(formCount > 1);

        ui->comboBoxAltForm->clear();
        for (u8 i = 0; i < formCount; i++)
        {
            ui->comboBoxAltForm->addItem(QString::number(i));
        }
    }
}

void  Stationary3::altformIndexChanged(int index)
{
    if (index >= 0)
    {
        u16 specie = static_cast<u16>(ui->comboBoxPokemon->currentIndex());

        auto base = personalInfo.at(specie + 1);
        auto info = getPersonalInfo(base);
    }
}

void  Stationary3::generationIndexChanged(int index)
{
    if (index >= 0)
    {
        u16 max = 0;
        if (index == 0)
        {
            personalInfo = PersonalInfo::loadPersonal(3);
            max = 386;
        }
        else if (index == 1)
        {
            personalInfo = PersonalInfo::loadPersonal(4);
            max = 493;
        }
        else if (index == 2)
        {
            personalInfo = PersonalInfo::loadPersonal(5);
            max = 649;
        }

        QVector<u16> species;
        for (u16 i = 1; i <= max; i++)
        {
            species.append(i);
        }

        ui->comboBoxPokemon->clear();
        ui->comboBoxPokemon->addItems(Translator::getSpecies(species));
    }
}
PersonalInfo Stationary3::getPersonalInfo(const PersonalInfo &base)
{
    u8 form = static_cast<u8>(ui->comboBoxAltForm->currentIndex());
    u16 formIndex = base.getFormStatIndex();

    if (form == 0 || formIndex == 0)
    {
        return base;
    }

    return personalInfo.at(formIndex + form - 1);
}

void Stationary3::calculatestats()
{
ui->filterGenerator->resetNatures();
int calcNature;
calcNature = ui->comboBoxNatures->currentIndex();
ui->filterGenerator->setSearchNature(calcNature);
findIVs();
}
