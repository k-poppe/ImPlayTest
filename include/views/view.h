// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <vector>
#include <functional>
#include <nfd.hpp>

namespace ImPlay::Views {
class View {
 public:
  View() = default;
  virtual ~View() = default;

  virtual void draw() = 0;
  virtual void show() { m_open = true; }
  bool isOpen() { return m_open; }
  
  void openFile(std::vector<nfdu8filteritem_t> filters, std::function<void(nfdu8char_t *)> callback);
  void openFiles(std::vector<nfdu8filteritem_t> filters, std::function<void(nfdu8char_t *, int)> callback);
  void openFolder(std::function<void(nfdu8char_t *)> callback);

protected:
  bool m_open = false;
};
}  // namespace ImPlay::Views