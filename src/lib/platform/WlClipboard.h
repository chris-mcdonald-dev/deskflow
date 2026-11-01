/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"
#include "platform/EiClipboard.h"

#include <mutex>
#include <string>

namespace deskflow {

//! Wayland clipboard via the wl-clipboard helper binaries
/*!
The portal clipboard is not available on all compositors and the X11 clipboard
cannot publish into a Wayland selection, so this backend shells out to
wl-paste/wl-copy. Prototype: text only.
*/
class WlClipboard : public EiClipboard
{
public:
  explicit WlClipboard(ClipboardID id);
  ~WlClipboard() override = default;

  bool empty() override;
  void add(Format, const std::string &data) override;
  bool open(Time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format) const override;
  std::string get(Format) const override;

private:
  void pullFromWayland() const;
  void pushToWayland() const;

  mutable bool m_open = false;
  mutable bool m_owner = false;
  mutable Time m_time = 0;
  mutable Time m_timeOwned = 0;
  mutable std::string m_data[3];
  mutable std::mutex m_mutex;
};

} // namespace deskflow
