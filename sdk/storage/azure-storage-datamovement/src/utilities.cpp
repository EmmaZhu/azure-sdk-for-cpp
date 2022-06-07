// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "azure/storage/datamovement/utilities.hpp"

#include <azure/core/platform.hpp>

#if defined(AZ_PLATFORM_WINDOWS)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <climits>
#include <unistd.h>
#endif

#include <algorithm>
#include <stdexcept>
#include <vector>

#if defined(AZ_PLATFORM_WINDOWS)
namespace Azure { namespace Storage { namespace _internal {
  std::wstring Utf8ToWide(const std::string& narrow);
  std::string Utf8ToNarrow(const std::wstring& wide);
}}} // namespace Azure::Storage::_internal
#endif

namespace Azure { namespace Storage { namespace _internal {

  constexpr static const char* FileUrlScheme = "file://";

  std::string GetFileUrl(const std::string& relativePath)
  {
#if defined(AZ_PLATFORM_WINDOWS)
    const std::wstring relativePathW = Storage::_internal::Utf8ToWide(relativePath);
    wchar_t absPathW[MAX_PATH];
    DWORD absPathWLength = GetFullPathNameW(relativePathW.data(), MAX_PATH, absPathW, nullptr);
    if (absPathWLength == 0)
    {
      throw std::runtime_error("Failed to get absolute path.");
    }
    std::replace(absPathW, absPathW + absPathWLength, L'\\', L'/');
    return FileUrlScheme + Storage::_internal::Utf8ToNarrow(absPathW);
#else
    if (relativePath.empty())
    {
      throw std::runtime_error("Failed to get absolute path.");
    }

    std::string absPath;
    if (relativePath[0] == '/')
    {
      absPath = relativePath;
    }
    else
    {
      absPath = std::string(PATH_MAX + 1, '\0');
      if (!getcwd(&absPath[0], absPath.length()))
      {
        throw std::runtime_error("Cannot get current working directory.");
      }
      absPath = std::string(absPath.data());
      absPath += "/" + relativePath;
    }

    std::vector<std::string> parts;
    std::string part;
    for (auto c : absPath + "/")
    {
      if (c == '/')
      {
        if (part.empty() || part == ".")
        {
        }
        else if (part == "..")
        {
          if (!parts.empty())
          {
            parts.pop_back();
          }
        }
        else
        {
          parts.push_back(std::move(part));
        }
        part.clear();
      }
      else
      {
        part += c;
      }
    }
    absPath.clear();
    for (const auto& p : parts)
    {
      absPath += "/" + p;
    }

    return FileUrlScheme + absPath;
#endif
  }

}}} // namespace Azure::Storage::_internal