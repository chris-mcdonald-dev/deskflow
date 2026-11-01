/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EvdevKeyState.h"

#include "base/Log.h"

#ifdef __linux__

#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr std::size_t kKeyBitmapSize = (KEY_CNT + 7) / 8;

bool testBit(const std::uint8_t *bits, std::uint32_t bit)
{
  return (bits[bit / 8] & (1u << (bit % 8))) != 0;
}

bool isKeyboard(int fd)
{
  std::uint8_t keys[kKeyBitmapSize] = {};
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0) {
    return false;
  }
  return testBit(keys, KEY_A) || testBit(keys, KEY_LEFTCTRL);
}

} // namespace

EvdevKeyState::~EvdevKeyState()
{
  closeDevices();
}

void EvdevKeyState::closeDevices()
{
  for (const auto fd : m_fds) {
    close(fd);
  }
  m_fds.clear();
}

void EvdevKeyState::openDevices()
{
  closeDevices();

  glob_t paths = {};
  if (glob("/dev/input/event*", 0, nullptr, &paths) != 0) {
    return;
  }

  for (std::size_t i = 0; i < paths.gl_pathc; ++i) {
    const auto fd = open(paths.gl_pathv[i], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
      continue;
    }
    if (!isKeyboard(fd)) {
      close(fd);
      continue;
    }
    m_fds.push_back(fd);
  }
  globfree(&paths);

  LOG_DEBUG("evdev: %zu readable keyboard device(s)", m_fds.size());
}

bool EvdevKeyState::isAvailable()
{
  if (m_fds.empty()) {
    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastScan < std::chrono::seconds(10)) {
      return false;
    }
    m_lastScan = now;
    openDevices();
  }
  return !m_fds.empty();
}

const std::set<std::uint32_t> &EvdevKeyState::pressedKeys()
{
  m_pressed.clear();

  bool ok = !m_fds.empty();
  for (const auto fd : m_fds) {
    std::uint8_t keys[kKeyBitmapSize] = {};
    if (ioctl(fd, EVIOCGKEY(sizeof(keys)), keys) < 0) {
      ok = false;
      continue;
    }
    for (std::uint32_t code = 0; code < KEY_CNT; ++code) {
      if (testBit(keys, code)) {
        m_pressed.insert(code);
      }
    }
  }

  if (!ok) {
    closeDevices();
    m_pressed.clear();
  }
  return m_pressed;
}

#else // !__linux__

EvdevKeyState::~EvdevKeyState() = default;

void EvdevKeyState::closeDevices()
{
}

void EvdevKeyState::openDevices()
{
}

bool EvdevKeyState::isAvailable()
{
  return false;
}

const std::set<std::uint32_t> &EvdevKeyState::pressedKeys()
{
  return m_pressed;
}

#endif
