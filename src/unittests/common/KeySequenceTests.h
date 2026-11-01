/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class KeySequenceTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void toString_controlShiftPlus_usesNamedPlus();
  void fromString_controlOnly_isModifierOnly();
  void fromString_controlPlusA_roundTrips();
  void fromString_keyNames_roundTrip();
};
