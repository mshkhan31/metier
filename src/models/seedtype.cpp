// Copyright (c) 2019-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "seedtype.hpp"  // IWYU pragma: associated

#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace metier::model
{
SeedType::SeedType(QObject* parent, Data&& data) noexcept
    : data_(std::move(data))
    , max_(std::accumulate(
          data_.begin(),
          data_.end(),
          0,
          [](const int prev, const auto& next) -> int {
              return prev + static_cast<int>(next.first.size());
          }))
{
    moveToThread(parent->thread());
}

auto SeedType::data(const QModelIndex& index, int role) const -> QVariant
{
    if (false == index.isValid()) { return {}; }

    const auto row = static_cast<std::size_t>(index.row());

    switch (role) {
        case Qt::DisplayRole: {
            try {
                return QString{data_.at(row).first.c_str()};
            } catch (...) {
            }
        } break;
        case Qt::UserRole: {
            try {
                return data_.at(row).second;
            } catch (...) {
            }
        } break;
        default: {
        }
    }

    return {};
}

auto SeedType::rowCount([[maybe_unused]] const QModelIndex& parent) const -> int
{
    return static_cast<int>(data_.size());
}
}  // namespace metier::model
