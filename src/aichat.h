/*
 * Copyright (C) 2026 LingmoOS Team.
 *
 * Author:     devalexandre <alexandre@dev2learn.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QUrl>

#include "assistantengine.h"

// Chat with the local model chosen in Lingmo Settings › IA (Ollama),
// streaming the answer into the Spotlight popup. When Lingmo Assistente is
// installed and the model can use tools, questions go through its engine
// (same tools, permissions and confirmations as the Assistente app);
// otherwise Spotlight talks to Ollama directly.
class AiChat : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString model READ model NOTIFY modelChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // Theme icon of the provider ("ollama"), or empty when the theme has none
    Q_PROPERTY(QString providerIcon READ providerIcon NOTIFY modelChanged)
    // A write waits for Permitir/Recusar: the popup must stay open
    Q_PROPERTY(bool pendingConfirmation READ pendingConfirmation NOTIFY pendingConfirmationChanged)
    // This conversation runs through the Assistente engine (tools, handoff)
    Q_PROPERTY(bool usingAssistant READ usingAssistant NOTIFY usingAssistantChanged)

public:
    enum Roles {
        RoleRole = Qt::UserRole + 1,  // user, assistant, tool, confirm
        TextRole,
        ThinkingRole,
        StatsRole,
        RunningRole,                  // tool: still running
        OkRole,                       // tool: succeeded
        RequestIdRole,                // confirm
        TitleRole,
        DetailsRole,
        ConfirmStateRole,             // confirm: pending, allowed, denied, handed
    };

    explicit AiChat(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const { return m_reply || m_showReply || m_engineBusy; }
    QString model() const { return m_model; }
    QString error() const { return m_error; }
    int count() const { return m_messages.size(); }
    QString providerIcon() const;
    bool pendingConfirmation() const;
    bool usingAssistant() const { return m_usingEngine; }

    Q_INVOKABLE void ask(const QString &question);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void copy(int row) const;
    // Answer a confirmation card
    Q_INVOKABLE void confirm(const QString &requestId, bool allow);
    // Continue this conversation (and a waiting confirmation) in the Assistente app
    Q_INVOKABLE void openInAssistant();

Q_SIGNALS:
    void busyChanged();
    void modelChanged();
    void errorChanged();
    void countChanged();
    void pendingConfirmationChanged();
    void usingAssistantChanged();

private:
    struct Message {
        QString role;  // "user", "assistant", "tool" or "confirm"
        QString text;  // tool: "Ran: lscpu"
        bool thinking = false;  // the model is reasoning before answering
        QString stats;          // "Respondeu em 2,4 s · 45 tokens/s" once done
        bool running = false;
        bool ok = true;
        QString function;       // tool
        QString args;
        QString requestId;      // confirm
        QString title;
        QString details;
        QString confirmState;
    };

    void loadSettings();
    void send();
    void readChunk();
    void finished();
    void setError(const QString &error);
    QString statsText(const QJsonObject &done) const;
    // Assistente engine path
    void dispatch();
    void checkTools();
    void askEngine(const QString &question);
    void onEngineEvent(const QVariantMap &event);
    void setEngineBusy(bool busy);
    void setUsingEngine(bool on);
    int openAnswer();
    void finishEngineAnswer(const QString &stats);
    QString toolLabel(const QString &function, const QString &args, bool running) const;
    void changed(int row, const QList<int> &roles);

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_buffer;
    QElapsedTimer m_elapsed;
    QList<Message> m_messages;
    QUrl m_server;
    QString m_provider;
    QString m_model;
    bool m_think = false;
    QString m_error;

    AssistantEngine m_engine;
    QPointer<QNetworkReply> m_showReply;  // /api/show: does the model support tools?
    QHash<QString, bool> m_toolModels;
    QString m_conversationId;
    bool m_engineBusy = false;
    bool m_usingEngine = false;
    bool m_lastPending = false;
};
