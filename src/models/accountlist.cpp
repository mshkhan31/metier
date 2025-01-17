// Copyright (c) 2019-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "accountlist.hpp"  // IWYU pragma: associated

namespace metier::model
{
AccountList::AccountList(QAbstractItemModel* source) noexcept
    : ot_super(nullptr)
{
    setSourceModel(source);
    moveToThread(source->thread());
}
}  // namespace metier::model
