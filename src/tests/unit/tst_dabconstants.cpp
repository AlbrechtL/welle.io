/*
 *    Copyright (C) 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QtTest>

#include <stdexcept>
#include <string>
#include <vector>

#include "charsets.h"
#include "dab-constants.h"

class DabConstantsTests : public QObject
{
    Q_OBJECT

private slots:
    void dabParams_data();
    void dabParams();
    void dabParamsRejectsUnknownMode();

    void ebuLatinLabel();
    void shortLabel();
    void fig2LabelPreferredWhenComplete();
    void fig2LabelFallsBackToFig1WhenIncomplete();

    void eepBitrate_data();
    void eepBitrate();
};

void DabConstantsTests::dabParams_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("L");
    QTest::addColumn<int>("K");
    QTest::addColumn<int>("T_F");
    QTest::addColumn<int>("T_u");

    // ETSI EN 300 401, table 38
    QTest::newRow("Mode I")   << 1 << 76  << 1536 << 196608 << 2048;
    QTest::newRow("Mode II")  << 2 << 76  << 384  << 49152  << 512;
    QTest::newRow("Mode III") << 3 << 153 << 192  << 49152  << 256;
    QTest::newRow("Mode IV")  << 4 << 76  << 768  << 98304  << 1024;
}

void DabConstantsTests::dabParams()
{
    QFETCH(int, mode);
    QFETCH(int, L);
    QFETCH(int, K);
    QFETCH(int, T_F);
    QFETCH(int, T_u);

    DABParams p(mode);
    QCOMPARE((int)p.dabMode, mode);
    QCOMPARE((int)p.L, L);
    QCOMPARE((int)p.K, K);
    QCOMPARE((int)p.T_F, T_F);
    QCOMPARE((int)p.T_u, T_u);
    QCOMPARE((int)p.T_s, p.T_u + p.guardLength);
}

void DabConstantsTests::dabParamsRejectsUnknownMode()
{
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, DABParams(5));
}

void DabConstantsTests::ebuLatinLabel()
{
    DabLabel label;
    label.fig1_label = "M\x99nchen \x8d\x91\x97 \xa9";  // EBU Latin: ü, ß, ä, ö, €
    QCOMPARE(QString::fromStdString(label.utf8_label()), QString::fromUtf8("München ßäö €"));
}

void DabConstantsTests::shortLabel()
{
    DabLabel label;
    label.fig1_label = "OpenDigitalRadio";
    label.fig1_flag = 0x8850;  // characters 0, 4, 9, 11: O, D, a, R

    QCOMPARE(QString::fromStdString(label.fig1_shortlabel_utf8()), QString("ODaR"));
}

void DabConstantsTests::fig2LabelPreferredWhenComplete()
{
    DabLabel label;
    label.fig1_label = "Radio";
    label.extended_label_charset = CharacterSet::UnicodeUtf8;
    label.segment_count = 2;
    const std::string s0 = "Radio ";
    const std::string s1 = "Österreich";
    label.segments[0] = std::vector<uint8_t>(s0.begin(), s0.end());
    label.segments[1] = std::vector<uint8_t>(s1.begin(), s1.end());

    QCOMPARE(QString::fromStdString(label.utf8_label()), QString::fromUtf8("Radio Österreich"));
}

void DabConstantsTests::fig2LabelFallsBackToFig1WhenIncomplete()
{
    DabLabel label;
    label.fig1_label = "Radio";
    label.extended_label_charset = CharacterSet::UnicodeUtf8;
    label.segment_count = 2;
    const std::string s1 = "Österreich";
    label.segments[1] = std::vector<uint8_t>(s1.begin(), s1.end());

    QCOMPARE(QString::fromStdString(label.utf8_label()), QString("Radio"));
}

void DabConstantsTests::eepBitrate_data()
{
    QTest::addColumn<int>("lengthCU");
    QTest::addColumn<int>("level");
    QTest::addColumn<int>("bitrate");
    QTest::addColumn<QString>("protection");

    // Values from the service tables of the Vienna and Bratislava ensembles
    QTest::newRow("RockovaRepublika")  << 96  << 3 << 128 << "EEP 3-A";
    QTest::newRow("Radio Litera")      << 60  << 3 << 80  << "EEP 3-A";
    QTest::newRow("Radio Aktual")      << 128 << 2 << 128 << "EEP 2-A";
    QTest::newRow("ERF Plus")          << 24  << 3 << 32  << "EEP 3-A";
    QTest::newRow("Radio Slovensko")   << 102 << 3 << 136 << "EEP 3-A";
}

void DabConstantsTests::eepBitrate()
{
    QFETCH(int, lengthCU);
    QFETCH(int, level);
    QFETCH(int, bitrate);
    QFETCH(QString, protection);

    Subchannel sc;
    sc.subChId = 1;
    sc.length = lengthCU;
    sc.protectionSettings.shortForm = false;
    sc.protectionSettings.eepProfile = EEPProtectionProfile::EEP_A;
    sc.protectionSettings.eepLevel = static_cast<EEPProtectionLevel>(level);

    QCOMPARE(sc.bitrate(), bitrate);
    QCOMPARE(sc.numCU(), lengthCU);
    QCOMPARE(QString::fromStdString(sc.protection()), protection);
}

QTEST_APPLESS_MAIN(DabConstantsTests)

#include "tst_dabconstants.moc"
