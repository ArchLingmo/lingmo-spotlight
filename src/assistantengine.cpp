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

#include "assistantengine.h"

#include <QFileInfo>
#include <QJsonDocument>

static constexpr int MaxBuffer = 8 * 1024 * 1024;
// The engine loads a Python runtime: don't keep it around forever
static constexpr int IdleMs = 10 * 60 * 1000;

AssistantEngine::AssistantEngine(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    m_idle.setSingleShot(true);
    m_idle.setInterval(IdleMs);
    connect(&m_idle, &QTimer::timeout, this, [this] {
        if (!m_busy)
            shutdown();
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &AssistantEngine::readOutput);
    connect(&m_process, &QProcess::started, this, [this] {
        for (const QByteArray &line : std::as_const(m_queue))
            m_process.write(line);
        m_queue.clear();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_queue.clear();
            Q_EMIT failed();
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        m_buffer.clear();
        if (!m_stopping)
            Q_EMIT failed();
        m_stopping = false;
    });
}

AssistantEngine::~AssistantEngine()
{
    shutdown();
    if (m_process.state() != QProcess::NotRunning && !m_process.waitForFinished(1500))
        m_process.kill();
}

QString AssistantEngine::path()
{
    const QString override = qEnvironmentVariable("LINGMO_ASSISTANT_ENGINE");
    return override.isEmpty() ? QStringLiteral("/usr/lib/lingmo-assistant/lingmo-assistant-engine") : override;
}

bool AssistantEngine::isInstalled()
{
    const QFileInfo info(path());
    return info.isFile() && info.isExecutable();
}

void AssistantEngine::shutdown()
{
    if (m_process.state() == QProcess::NotRunning)
        return;
    m_stopping = true;
    m_process.closeWriteChannel();   // the engine exits on end of input
}

void AssistantEngine::setBusy(bool busy)
{
    m_busy = busy;
    m_idle.start();
}

void AssistantEngine::send(const QJsonObject &request)
{
    m_idle.start();
    const QByteArray line = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (m_process.state() == QProcess::Running) {
        m_process.write(line);
        return;
    }
    m_queue << line;
    if (m_process.state() == QProcess::NotRunning) {
        m_buffer.clear();
        m_stopping = false;
        m_process.start(path(), {});
    }
}

void AssistantEngine::readOutput()
{
    m_buffer += m_process.readAllStandardOutput();
    if (m_buffer.size() > MaxBuffer) {
        m_buffer.clear();
        return;
    }
    int newline;
    while ((newline = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        if (line.isEmpty())
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject())
            Q_EMIT event(doc.object().toVariantMap());
    }
}
