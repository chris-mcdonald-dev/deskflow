/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QString>

class QSettings;

class KeySequence
{
public:
  KeySequence() = default;

  QString toString() const;
  bool appendKey(int key, int modifiers);
  bool appendMouseButton(int button);
  bool isMouseButton() const;

  //! True if the sequence is made up of modifier keys only (e.g. just Ctrl)
  bool isModifierOnly() const;

  //! Build a sequence from its text form, e.g. \c Control or \c Control+a
  static KeySequence fromString(const QString &str);

  bool valid() const
  {
    return m_IsValid;
  }
  int modifiers() const
  {
    return m_Modifiers;
  }
  void saveSettings(QSettings &settings) const;
  void loadSettings(QSettings &settings);
  const QList<int> &sequence() const
  {
    return m_Sequence;
  }

  bool operator==(const KeySequence &ks) const = default;

private:
  void setValid(bool b)
  {
    m_IsValid = b;
  }
  void setModifiers(int i)
  {
    m_Modifiers = i;
  }
  QList<int> &sequence()
  {
    return m_Sequence;
  }

  QList<int> m_Sequence = {};
  int m_Modifiers = 0;
  bool m_IsValid = false;

  inline static const int kStrSize = 4;
  inline static const int kBase = 16;
  inline static const QChar kFillChar = QChar('0');

  static QString keyToString(int key);
  static int keyFromString(const QString &name);
  static int modifierFromString(const QString &name);
};
