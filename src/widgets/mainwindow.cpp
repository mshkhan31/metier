// Copyright (c) 2019-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "mainwindow.hpp"  // IWYU pragma: associated

#include <opentxs/opentxs.hpp>
#include <QAction>
#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QList>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QToolBox>
#include <QVariant>

#include "mainwindow/imp.hpp"
#include "otwrap.hpp"
#include "ui_mainwindow.h"
#include "util/focuser.hpp"
#include "widgets/blockchainchooser.hpp"
#include "widgets/licenses.hpp"
#include "widgets/mainwindow/chaintoolboxmanager.hpp"
#include "widgets/showseed.hpp"

namespace metier::widget
{
using OTModel = opentxs::ui::AccountListQt;

MainWindow::MainWindow(QObject* parent, OTWrap& ot) noexcept
    : QMainWindow(nullptr)
    , imp_p_(std::make_unique<Imp>(this, ot))
    , imp_(*imp_p_)
{
    moveToThread(parent->thread());
    qRegisterMetaType<QVector<int>>();
    setWindowTitle(QString::fromLocal8Bit(METIER_APPSTREAM_NAME));
    auto* quit = imp_.ui_->action_file_quit;
    auto* bc = imp_.ui_->action_settings_blockchain;
    auto* words = imp_.ui_->action_settings_recovery_phrase;
    auto* bcdone = imp_.blockchains_->Ok();
    auto* license = imp_.ui_->action_help_opensource;
    auto* toolbox = imp_.ui_->moneyToolbox;
    auto* prog = imp_.ui_->syncProgress;
    connect(&ot, &OTWrap::nymReady, this, &MainWindow::initModels);
    connect(&ot, &OTWrap::readyForMainWindow, this, &MainWindow::updateToolbox);
    connect(&ot, &OTWrap::chainsChanged, this, &MainWindow::updateToolbox);
    connect(quit, &QAction::triggered, this, &MainWindow::exit);
    connect(bc, &QAction::triggered, this, &MainWindow::showBlockchainChooser);
    connect(words, &QAction::triggered, this, &MainWindow::showRecoveryWords);
    connect(bcdone, &QPushButton::clicked, &ot, &OTWrap::checkAccounts);
    connect(license, &QAction::triggered, this, &MainWindow::showLicenseViewer);
    connect(toolbox, &QToolBox::currentChanged, this, &MainWindow::changeChain);
    connect(this, &MainWindow::progMaxUpdated, prog, &QProgressBar::setMaximum);
    connect(this, &MainWindow::progValueUpdated, prog, &QProgressBar::setValue);
    showBlockchainStatistics();
}

auto MainWindow::accountListUpdated(
    const QItemSelection& current,
    [[maybe_unused]] const QItemSelection& previous) -> void
{
    const auto indices = current.indexes();

    if (0 == indices.size()) {
        showBlockchainStatistics();
    } else {
        auto& accountList = *imp_.ui_->accountList;
        const auto& index = indices.first();
        const auto& model = *accountList.model();
        const auto accountID =
            model.data(index, OTModel::Roles::AccountIDRole).toString();
        showAccountActivity(accountID);
    }

    imp_.updateProgress();
}

auto MainWindow::canMessage(bool value) -> void
{
    auto& edit = *imp_.ui_->messageEdit;
    auto& send = *imp_.ui_->sendMessage;
    edit.setEnabled(value);
    send.setEnabled(value);
}

auto MainWindow::clearActivityThread() -> void
{
    auto& view = *imp_.ui_->activityThreadView;
    auto& edit = *imp_.ui_->messageEdit;
    auto& send = *imp_.ui_->sendMessage;

    if (auto* model = view.model(); nullptr != model) { model->disconnect(); }

    edit.disconnect();
    send.disconnect();
    edit.setEnabled(false);
    send.setEnabled(false);
    edit.setPlainText({});
}

auto MainWindow::contactListUpdated(
    const QItemSelection& current,
    [[maybe_unused]] const QItemSelection& previous) -> void
{
    const auto indices = current.indexes();

    if (0 == indices.size()) {
        clearActivityThread();
    } else {
        auto& contactList = *imp_.ui_->contactListView;
        const auto& index = indices.first();
        const auto& model = *contactList.model();
        const auto accountID =
            model.data(index, ot::ui::ContactListQt::IDRole).toString();
        showActivityThread(accountID);
    }
}

auto MainWindow::exit() -> void { QCoreApplication::exit(); }

auto MainWindow::initModels() -> void { imp_.init_models(this); }

auto MainWindow::sendMessage() -> void
{
    using Model = opentxs::ui::ActivityThreadQt;
    auto& view = *imp_.ui_->activityThreadView;
    auto& edit = *imp_.ui_->messageEdit;

    if (auto* model = view.model(); nullptr != model) {
        auto* thread = dynamic_cast<Model*>(model);

        if (nullptr == thread) { return; }

        thread->setDraft(edit.toPlainText());
        thread->sendDraft();
        edit.setPlainText(thread->draft());
    }
}

auto MainWindow::setProgressMax(int max) -> void { emit progMaxUpdated(max); }

auto MainWindow::setProgressValue(int value) -> void
{
    emit progValueUpdated(value);
}

auto MainWindow::showAddContact() -> void { imp_.showAddContact(); }

auto MainWindow::showAccountActivity(int chain) -> void
{
    auto& accountActivity = *imp_.ui_->accountActivity;
    accountActivity.setModel(imp_.ot_.accountActivityModel(chain));
}

auto MainWindow::showAccountActivity(QString account) -> void
{
    auto& accountActivity = *imp_.ui_->accountActivity;
    accountActivity.setModel(imp_.ot_.accountActivityModel(account));
}

auto MainWindow::showActivityThread(QString contact) -> void
{
    using Model = opentxs::ui::ActivityThreadQt;
    auto& view = *imp_.ui_->activityThreadView;
    auto& edit = *imp_.ui_->messageEdit;
    auto& send = *imp_.ui_->sendMessage;
    edit.disconnect();
    send.disconnect();

    if (auto* model = view.model(); nullptr != model) {
        model->disconnect();
        this->disconnect(model);
        dynamic_cast<Model*>(model)->setDraft(edit.toPlainText());
    }

    auto* model = imp_.ot_.activityThreadModel(contact);

    if (nullptr == model) {
        clearActivityThread();

        return;
    }

    const auto canMessage = model->canMessage();
    connect(model, &Model::canMessageUpdate, this, &MainWindow::canMessage);
    connect(&send, &QAbstractButton::clicked, this, &MainWindow::sendMessage);
    view.setModel(model);
    edit.setEnabled(canMessage);
    send.setEnabled(canMessage);
    edit.setPlainText(model->draft());
}

auto MainWindow::showBlockchainChooser() -> void
{
    util::Focuser(imp_.blockchains_.get()).show();
}

auto MainWindow::showBlockchainStatistics() -> void
{
    auto& accountActivity = *imp_.ui_->accountActivity;
    accountActivity.setModel(imp_.ot_.blockchainStatisticsModel());
}

auto MainWindow::showLicenseViewer() -> void
{
    util::Focuser(imp_.licenses_.get()).show();
}

auto MainWindow::showRecoveryWords() -> void
{
    auto dialog = std::make_unique<ShowSeed>(this, imp_.ot_);
    auto postcondition = ScopeGuard{[&]() { dialog.release(); }};
    util::Focuser(dialog.get()).show();
}

auto MainWindow::changeChain() -> void
{
    const auto chain = imp_.chain_toolbox_.currentChain();

    if (ot::blockchain::Type::Unknown == chain) {
        showBlockchainStatistics();
    } else {
        showAccountActivity(static_cast<int>(chain));
    }

    imp_.updateProgress();
}

auto MainWindow::updateName(QString value) -> void { imp_.updateName(value); }

auto MainWindow::updatePaymentCode(QString value) -> void
{
    imp_.updatePaymentCode(value);
}

auto MainWindow::updateToolbox() -> void
{
    imp_.chain_toolbox_.reconcile(imp_.ot_.enabledBlockchains());
}

MainWindow::~MainWindow() = default;
}  // namespace metier::widget
