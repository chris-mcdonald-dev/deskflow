/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeySequenceTests.h"

#include "common/KeySequence.h"

void KeySequenceTests::toString_controlShiftPlus_usesNamedPlus()
{
  KeySequence sequence;

  sequence.appendKey(Qt::Key_Control, Qt::ControlModifier);
  sequence.appendKey(Qt::Key_Shift, Qt::ControlModifier | Qt::ShiftModifier);
  QVERIFY(sequence.appendKey(Qt::Key_Plus, Qt::ControlModifier | Qt::ShiftModifier));

  QCOMPARE(sequence.toString(), QStringLiteral("Control+Shift+Plus"));
}

void KeySequenceTests::fromString_controlOnly_isModifierOnly()
{
  const auto sequence = KeySequence::fromString(QStringLiteral("Control"));

  QCOMPARE(sequence.sequence(), QList<int>({static_cast<int>(Qt::ControlModifier)}));
  QVERIFY(sequence.isModifierOnly());
  QVERIFY(!sequence.valid());
  QCOMPARE(sequence.toString(), QStringLiteral("Control"));
}

void KeySequenceTests::fromString_controlPlusA_roundTrips()
{
  const auto sequence = KeySequence::fromString(QStringLiteral("Control+a"));

  QCOMPARE(sequence.toString(), QStringLiteral("Control+a"));
  QVERIFY(sequence.valid());
  QVERIFY(!sequence.isModifierOnly());
}

void KeySequenceTests::fromString_keyNames_roundTrip()
{
  QCOMPARE(KeySequence::fromString(QStringLiteral("Space")).toString(), QStringLiteral("Space"));
  QCOMPARE(KeySequence::fromString(QStringLiteral("Plus")).toString(), QStringLiteral("Plus"));
  QCOMPARE(KeySequence::fromString(QStringLiteral("F5")).toString(), QStringLiteral("F5"));

  const auto invalid = KeySequence::fromString(QStringLiteral("NotAKey"));
  QVERIFY(invalid.sequence().isEmpty());
}

QTEST_MAIN(KeySequenceTests)