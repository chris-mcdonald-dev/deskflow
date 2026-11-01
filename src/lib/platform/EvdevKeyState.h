/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <set>
#include <vector>

//! Hardware key state from /dev/input, needs readable devices (udev uaccess)
class EvdevKeyState
{
public:
  EvdevKeyState() = default;
  ~EvdevKeyState();
  EvdevKeyState(const EvdevKeyState &) = delete;
  EvdevKeyState &operator=(const EvdevKeyState &) = delete;

  //! True if a readable keyboard device was found
  bool isAvailable();

  //! evdev key codes currently down
  const std::set<std::uint32_t> &pressedKeys();

private:
  void openDevices();
  void closeDevices();

  std::vector<int> m_fds;
  std::set<std::uint32_t> m_pressed;
  std::chrono::steady_clock::time_point m_lastScan;
};
