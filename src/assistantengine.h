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

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>

// Lingmo Assistente's engine (lingmo-assistant-engine), run with QProcess
// (argv, no shell) and spoken to in its NDJSON protocol: one JSON object per
// line each way. Started on first use, closed after a while without questions.
class AssistantEngine : public QObject
{
    Q_OBJECT

public:
    explicit AssistantEngine(QObject *parent = nullptr);
    ~AssistantEngine() override;

    // /usr/lib/lingmo-assistant/lingmo-assistant-engine, or $LINGMO_ASSISTANT_ENGINE
    static QString path();
    static bool isInstalled();

    void send(const QJsonObject &request);
    // Keep the process while an answer or a confirmation is in progress
    void setBusy(bool busy);

Q_SIGNALS:
    void event(const QVariantMap &event);
    void failed();

private:
    void readOutput();
    void shutdown();

    QProcess m_process;
    QByteArray m_buffer;
    QList<QByteArray> m_queue;
    QTimer m_idle;
    bool m_busy = false;
    bool m_stopping = false;
};
