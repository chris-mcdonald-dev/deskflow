/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboard.h"

#include "base/Log.h"

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>

#ifdef __linux__

#include <sys/wait.h>
#include <unistd.h>

namespace {

//! Run a command, capturing stdout
std::string runCapture(const char *const argv[])
{
  int fds[2];
  if (pipe(fds) != 0) {
    return {};
  }

  const pid_t pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    return {};
  }

  if (pid == 0) {
    close(fds[0]);
    dup2(fds[1], STDOUT_FILENO);
    close(fds[1]);
    execvp(argv[0], const_cast<char *const *>(argv));
    _exit(127);
  }

  close(fds[1]);
  std::string out;
  char buffer[4096];
  bool timedOut = false;
  for (;;) {
    struct pollfd pfd = {fds[0], POLLIN, 0};
    const int ready = poll(&pfd, 1, 1000);
    if (ready < 0) {
      break;
    }
    if (ready == 0) {
      timedOut = true;
      break;
    }
    const ssize_t n = read(fds[0], buffer, sizeof(buffer));
    if (n <= 0) {
      break;
    }
    out.append(buffer, static_cast<size_t>(n));
  }
  close(fds[0]);

  if (timedOut) {
    LOG_WARN("timed out reading the Wayland clipboard");
    kill(pid, SIGKILL);
  }

  int status = 0;
  waitpid(pid, &status, 0);
  if (timedOut || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    return {};
  }
  return out;
}

//! Run a command with the given stdin
bool runFeed(const char *const argv[], const std::string &input)
{
  int fds[2];
  if (pipe(fds) != 0) {
    return false;
  }

  const pid_t pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    return false;
  }

  if (pid == 0) {
    close(fds[1]);
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    execvp(argv[0], const_cast<char *const *>(argv));
    _exit(127);
  }

  close(fds[0]);
  size_t written = 0;
  while (written < input.size()) {
    const ssize_t n = write(fds[1], input.data() + written, input.size() - written);
    if (n <= 0) {
      break;
    }
    written += static_cast<size_t>(n);
  }
  close(fds[1]);

  int status = 0;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

//! True if the helper program is on PATH
bool helperAvailable(const char *name)
{
  const char *path = getenv("PATH");
  if (path == nullptr) {
    return false;
  }

  const std::string dirs{path};
  size_t start = 0;
  while (start <= dirs.size()) {
    const size_t end = dirs.find(':', start);
    const auto dir = dirs.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!dir.empty() && access((dir + "/" + name).c_str(), X_OK) == 0) {
      return true;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return false;
}

} // namespace

namespace deskflow {

WlClipboard::WlClipboard(ClipboardID id) : EiClipboard{id}
{
  if (!helperAvailable("wl-paste") || !helperAvailable("wl-copy")) {
    LOG_WARN("wl-clipboard is not installed, clipboard sharing will not work on Wayland");
  }
}

bool WlClipboard::open(Time time) const
{
  std::scoped_lock lock{m_mutex};
  if (m_open) {
    return true;
  }

  m_time = time;
  m_open = true;
  if (!m_owner) {
    pullFromWayland();
  }
  return true;
}

void WlClipboard::close() const
{
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    return;
  }
  m_open = false;
  if (m_owner) {
    pushToWayland();
    // the selection is no longer ours, so the next open() must read it again
    m_owner = false;
  }
}

bool WlClipboard::empty()
{
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    LOG_WARN("cannot empty clipboard, not open");
    return false;
  }
  for (auto &data : m_data) {
    data.clear();
  }
  m_timeOwned = m_time;
  m_owner = true;
  return true;
}

void WlClipboard::add(Format format, const std::string &data)
{
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    LOG_WARN("cannot add to clipboard, not open");
    return;
  }
  if (!m_owner) {
    LOG_WARN("cannot add to clipboard, no owner");
    return;
  }
  m_data[static_cast<int>(format)] = data;
}

IClipboard::Time WlClipboard::getTime() const
{
  std::scoped_lock lock{m_mutex};
  return m_time;
}

bool WlClipboard::has(Format format) const
{
  std::scoped_lock lock{m_mutex};
  return !m_data[static_cast<int>(format)].empty();
}

std::string WlClipboard::get(Format format) const
{
  std::scoped_lock lock{m_mutex};
  return m_data[static_cast<int>(format)];
}

void WlClipboard::pullFromWayland() const
{
  static const char *const textArgv[] = {"wl-paste", "--no-newline", "--type", "text/plain;charset=utf-8", nullptr};
  static const char *const htmlArgv[] = {"wl-paste", "--no-newline", "--type", "text/html", nullptr};

  m_data[static_cast<int>(Format::Text)] = runCapture(textArgv);
  m_data[static_cast<int>(Format::HTML)] = runCapture(htmlArgv);

  LOG_VERBOSE(
      "wayland clipboard read: text=%zu bytes html=%zu bytes", m_data[static_cast<int>(Format::Text)].size(),
      m_data[static_cast<int>(Format::HTML)].size()
  );
}

void WlClipboard::pushToWayland() const
{
  const auto &text = m_data[static_cast<int>(Format::Text)];
  if (text.empty()) {
    return;
  }

  static const char *const argv[] = {"wl-copy", "--type", "text/plain;charset=utf-8"};
  const bool ok = runFeed(argv, text);
  LOG_DEBUG("wayland clipboard publish: %zu bytes, %s", text.size(), ok ? "ok" : "failed");
}

} // namespace deskflow

#else // !__linux__

namespace deskflow {

WlClipboard::WlClipboard(ClipboardID id) : EiClipboard{id}
{
}

bool WlClipboard::open(Time) const
{
  return false;
}
void WlClipboard::close() const
{
}
bool WlClipboard::empty()
{
  return false;
}
void WlClipboard::add(Format, const std::string &)
{
}
IClipboard::Time WlClipboard::getTime() const
{
  return 0;
}
bool WlClipboard::has(Format) const
{
  return false;
}
std::string WlClipboard::get(Format) const
{
  return {};
}
void WlClipboard::pullFromWayland() const
{
}
void WlClipboard::pushToWayland() const
{
}

} // namespace deskflow

#endif
