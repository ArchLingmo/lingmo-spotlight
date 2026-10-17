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

#include "aichat.h"

#include <QClipboard>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkReply>
#include <QSettings>
#include <QUuid>

// Messages sent back as context: enough for a follow-up, small for a local model
static constexpr int ContextMessages = 20;
// A streamed line never gets near this; protects against a misbehaving server
static constexpr int MaxBuffer = 1024 * 1024;

AiChat::AiChat(QObject *parent)
    : QAbstractListModel(parent)
    , m_conversationId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    loadSettings();
    connect(&m_engine, &AssistantEngine::event, this, &AiChat::onEngineEvent);
    connect(&m_engine, &AssistantEngine::failed, this, [this] {
        if (!m_engineBusy)
            return;
        finishEngineAnswer({});
        setError(tr("The assistant engine stopped unexpectedly."));
        setEngineBusy(false);
    });
}

int AiChat::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_messages.size();
}

QVariant AiChat::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return {};
    const Message &m = m_messages.at(index.row());
    switch (role) {
    case RoleRole:
        return m.role;
    case TextRole:
        return m.text;
    case ThinkingRole:
        return m.thinking;
    case StatsRole:
        return m.stats;
    case RunningRole:
        return m.running;
    case OkRole:
        return m.ok;
    case RequestIdRole:
        return m.requestId;
    case TitleRole:
        return m.title;
    case DetailsRole:
        return m.details;
    case ConfirmStateRole:
        return m.confirmState;
    }
    return {};
}

QHash<int, QByteArray> AiChat::roleNames() const
{
    return {{RoleRole, "role"}, {TextRole, "text"}, {ThinkingRole, "thinking"}, {StatsRole, "stats"},
            {RunningRole, "running"}, {OkRole, "ok"}, {RequestIdRole, "requestId"}, {TitleRole, "title"},
            {DetailsRole, "details"}, {ConfirmStateRole, "confirmState"}};
}

QString AiChat::providerIcon() const
{
    return QIcon::hasThemeIcon(m_provider) ? m_provider : QString();
}

void AiChat::loadSettings()
{
    // Written by Lingmo Settings › IA. Older builds stored the keys under an [ai] group.
    QSettings settings(QStringLiteral("lingmoos"), QStringLiteral("ai"));
    const auto value = [&settings](const QString &key) {
        const QString plain = settings.value(key).toString();
        return plain.isEmpty() ? settings.value(QStringLiteral("ai/") + key).toString() : plain;
    };
    m_provider = value(QStringLiteral("provider"));
    if (m_provider.isEmpty())
        m_provider = QStringLiteral("ollama");
    // Settings › AI › "Think before answering"
    m_think = settings.value(QStringLiteral("think"), false).toBool();
    const QString model = value(QStringLiteral("model"));
    if (model != m_model) {
        m_model = model;
        emit modelChanged();
    }

    // Same variable the ollama CLI honours; local by default
    QString host = qEnvironmentVariable("OLLAMA_HOST", QStringLiteral("127.0.0.1:11434"));
    if (!host.contains(QLatin1String("://")))
        host.prepend(QLatin1String("http://"));
    m_server = QUrl(host);
}

void AiChat::ask(const QString &question)
{
    const QString text = question.trimmed();
    if (text.isEmpty() || busy())
        return;
    loadSettings();
    setError({});
    if (m_provider != QLatin1String("ollama")) {
        setError(tr("Spotlight talks to Ollama models. Choose Ollama in Settings › AI."));
        return;
    }
    if (m_model.isEmpty()) {
        setError(tr("No AI model chosen. Choose one in Settings › AI."));
        return;
    }

    beginInsertRows({}, m_messages.size(), m_messages.size() + 1);
    m_messages << Message{QStringLiteral("user"), text} << Message{QStringLiteral("assistant"), QString()};
    endInsertRows();
    emit countChanged();
    dispatch();
}

void AiChat::send()
{
    QJsonArray messages;
    messages << QJsonObject{
        {QStringLiteral("role"), QStringLiteral("system")},
        {QStringLiteral("content"),
         QStringLiteral("You are the assistant built into Lingmo OS (ArchLingmo), a Linux distribution based on "
                        "Arch Linux with the Lingmo desktop, built with Qt 6. Answer in the language of the question, "
                        "clearly and concisely. Use Markdown when it helps (lists, code). Don't make up facts: "
                        "say so when you don't know.")}};
    // Everything but the empty answer being written (and the Assistente's tool/confirmation rows)
    const int last = m_messages.size() - 1;
    for (int i = qMax(0, last - ContextMessages); i < last; ++i) {
        const Message &m = m_messages.at(i);
        if (m.role != QLatin1String("user") && m.role != QLatin1String("assistant"))
            continue;
        messages << QJsonObject{{QStringLiteral("role"), m.role}, {QStringLiteral("content"), m.text}};
    }
    QJsonObject body{{QStringLiteral("model"), m_model},
                     {QStringLiteral("messages"), messages},
                     {QStringLiteral("stream"), true}};
    // Off: reasoning models answer directly. On: left to the model, so models
    // without reasoning don't get a "think" they would reject
    if (!m_think)
        body.insert(QStringLiteral("think"), false);

    QUrl url = m_server;
    url.setPath(QStringLiteral("/api/chat"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    // No answer at all for this long means the server is stuck, not thinking
    request.setTransferTimeout(120 * 1000);

    m_buffer.clear();
    m_elapsed.start();
    m_reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::readyRead, this, &AiChat::readChunk);
    connect(m_reply, &QNetworkReply::finished, this, &AiChat::finished);
    emit busyChanged();
}

void AiChat::readChunk()
{
    if (!m_reply)
        return;
    m_buffer += m_reply->readAll();
    if (m_buffer.size() > MaxBuffer) {
        setError(tr("Invalid response from the AI server."));
        stop();
        return;
    }
    // Ollama streams one JSON object per line
    QString added;
    QString stats;
    bool thinking = false;
    int newline;
    while ((newline = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        if (line.isEmpty())
            continue;
        const QJsonObject object = QJsonDocument::fromJson(line).object();
        if (object.contains(QLatin1String("error"))) {
            const QString message = object.value(QLatin1String("error")).toString();
            setError(message.contains(QLatin1String("not found"))
                         ? tr("The model %1 isn't installed. Install it in Settings › AI.").arg(m_model)
                         : tr("The AI server refused the request."));
            continue;
        }
        const QJsonObject message = object.value(QLatin1String("message")).toObject();
        added += message.value(QLatin1String("content")).toString();
        // Reasoning still streams when a model ignores "think": false; it's progress, not the answer
        if (!message.value(QLatin1String("thinking")).toString().isEmpty())
            thinking = true;
        if (object.value(QLatin1String("done")).toBool())
            stats = statsText(object);
    }
    if (m_messages.isEmpty() || m_messages.last().role != QLatin1String("assistant"))
        return;
    Message &answer = m_messages.last();
    QList<int> roles;
    if (!added.isEmpty()) {
        answer.text += added;
        roles << TextRole;
    }
    const bool isThinking = thinking && answer.text.isEmpty();
    if (answer.thinking != isThinking) {
        answer.thinking = isThinking;
        roles << ThinkingRole;
    }
    if (!stats.isEmpty()) {
        answer.stats = stats;
        roles << StatsRole;
    }
    if (roles.isEmpty())
        return;
    const QModelIndex row = index(m_messages.size() - 1);
    emit dataChanged(row, row, roles);
}

QString AiChat::statsText(const QJsonObject &done) const
{
    // Wall time as the user felt it; speed from Ollama's own generation counters
    const QString seconds = QLocale().toString(m_elapsed.elapsed() / 1000.0, 'f', 1);
    const double evalCount = done.value(QLatin1String("eval_count")).toDouble();
    const double evalSeconds = done.value(QLatin1String("eval_duration")).toDouble() / 1e9;
    if (evalCount > 0 && evalSeconds > 0)
        return tr("Answered in %1 s · %2 tokens/s").arg(seconds).arg(qRound(evalCount / evalSeconds));
    return tr("Answered in %1 s").arg(seconds);
}

void AiChat::finished()
{
    if (!m_reply)
        return;
    QNetworkReply *reply = m_reply;
    readChunk();
    const QNetworkReply::NetworkError code = reply->error();
    if (code != QNetworkReply::NoError && code != QNetworkReply::OperationCanceledError && m_error.isEmpty()) {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404)
            setError(tr("The model %1 isn't installed. Install it in Settings › AI.").arg(m_model));
        else if (code == QNetworkReply::ConnectionRefusedError || code == QNetworkReply::HostNotFoundError)
            setError(tr("Ollama isn't running. Start it with “systemctl start ollama” or from Settings › AI."));
        else if (code == QNetworkReply::TimeoutError)
            setError(tr("The AI took too long to answer. Try a smaller model."));
        else
            setError(tr("Couldn't reach the AI."));
    }
    m_reply = nullptr;
    reply->deleteLater();

    // Drop an answer that never started, so it isn't sent back as context
    if (!m_messages.isEmpty() && m_messages.last().role == QLatin1String("assistant")
        && m_messages.last().text.isEmpty()) {
        beginRemoveRows({}, m_messages.size() - 1, m_messages.size() - 1);
        m_messages.removeLast();
        endRemoveRows();
        emit countChanged();
    }
    emit busyChanged();
}

void AiChat::stop()
{
    if (m_reply)
        m_reply->abort();
    if (m_showReply)
        m_showReply->abort();
    // A confirmation still waiting stays paused in the engine's database: the
    // Assistente app can still answer it
    if (m_engineBusy)
        m_engine.send(QJsonObject{{QStringLiteral("type"), QStringLiteral("stop")}});
}

void AiChat::clear()
{
    stop();
    beginResetModel();
    m_messages.clear();
    endResetModel();
    setError({});
    emit countChanged();
    m_conversationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    setUsingEngine(false);
    if (m_lastPending) {
        m_lastPending = false;
        emit pendingConfirmationChanged();
    }
}

void AiChat::copy(int row) const
{
    if (row >= 0 && row < m_messages.size())
        QGuiApplication::clipboard()->setText(m_messages.at(row).text);
}

void AiChat::setError(const QString &error)
{
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

// ------------------------------------------------------------------ Assistente engine

void AiChat::changed(int row, const QList<int> &roles)
{
    const QModelIndex i = index(row);
    emit dataChanged(i, i, roles);
    const bool pending = pendingConfirmation();
    if (pending != m_lastPending) {
        m_lastPending = pending;
        emit pendingConfirmationChanged();
    }
}

bool AiChat::pendingConfirmation() const
{
    for (const Message &m : m_messages)
        if (m.role == QLatin1String("confirm") && m.confirmState == QLatin1String("pending"))
            return true;
    return false;
}

void AiChat::setEngineBusy(bool busy)
{
    if (busy == m_engineBusy)
        return;
    m_engineBusy = busy;
    m_engine.setBusy(busy);
    emit busyChanged();
}

void AiChat::setUsingEngine(bool on)
{
    if (on == m_usingEngine)
        return;
    m_usingEngine = on;
    emit usingAssistantChanged();
}

void AiChat::dispatch()
{
    // Without the Assistente, or with a model that can't call tools: talk to Ollama directly
    if (!AssistantEngine::isInstalled()) {
        send();
        return;
    }
    const auto known = m_toolModels.constFind(m_model);
    if (known == m_toolModels.constEnd()) {
        checkTools();
        return;
    }
    if (!*known) {
        send();
        return;
    }
    QString question;
    for (int i = m_messages.size() - 1; i >= 0 && question.isEmpty(); --i)
        if (m_messages.at(i).role == QLatin1String("user"))
            question = m_messages.at(i).text;
    askEngine(question);
}

void AiChat::checkTools()
{
    QUrl url = m_server;
    url.setPath(QStringLiteral("/api/show"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(5000);
    m_showReply = m_network.post(request, QJsonDocument(QJsonObject{{QStringLiteral("model"), m_model}})
                                              .toJson(QJsonDocument::Compact));
    emit busyChanged();
    QNetworkReply *reply = m_showReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_showReply = nullptr;
        const QNetworkReply::NetworkError code = reply->error();
        emit busyChanged();
        if (code == QNetworkReply::OperationCanceledError) {
            finished();   // stopped: drop the empty answer
            return;
        }
        if (code == QNetworkReply::NoError) {
            const QJsonArray caps = QJsonDocument::fromJson(reply->readAll()).object()
                                        .value(QLatin1String("capabilities")).toArray();
            m_toolModels.insert(m_model, caps.contains(QStringLiteral("tools")));
            dispatch();
            return;
        }
        // Ollama unreachable or model missing: the direct path explains it
        send();
    });
}

void AiChat::askEngine(const QString &question)
{
    setUsingEngine(true);
    setEngineBusy(true);
    m_elapsed.start();
    m_engine.send(QJsonObject{{QStringLiteral("type"), QStringLiteral("chat")},
                              {QStringLiteral("conversation_id"), m_conversationId},
                              {QStringLiteral("message"), question}});
}

int AiChat::openAnswer()
{
    if (!m_messages.isEmpty() && m_messages.last().role == QLatin1String("assistant")
        && m_messages.last().stats.isEmpty())
        return m_messages.size() - 1;
    beginInsertRows({}, m_messages.size(), m_messages.size());
    m_messages << Message{QStringLiteral("assistant"), QString()};
    endInsertRows();
    emit countChanged();
    return m_messages.size() - 1;
}

void AiChat::finishEngineAnswer(const QString &stats)
{
    if (!m_messages.isEmpty() && m_messages.last().role == QLatin1String("assistant")
        && m_messages.last().text.trimmed().isEmpty()) {
        beginRemoveRows({}, m_messages.size() - 1, m_messages.size() - 1);
        m_messages.removeLast();
        endRemoveRows();
        emit countChanged();
    }
    for (int row = m_messages.size() - 1; row >= 0; --row) {
        Message &m = m_messages[row];
        if (m.role == QLatin1String("user"))
            break;
        if (m.role == QLatin1String("tool") && m.running) {
            m.running = false;
            m.text = toolLabel(m.function, m.args, false);
            changed(row, {RunningRole, TextRole});
        }
    }
    if (stats.isEmpty())
        return;
    for (int row = m_messages.size() - 1; row >= 0; --row) {
        if (m_messages.at(row).role == QLatin1String("user"))
            break;
        if (m_messages.at(row).role == QLatin1String("assistant")) {
            m_messages[row].stats = stats;
            changed(row, {StatsRole});
            return;
        }
    }
}

QString AiChat::toolLabel(const QString &function, const QString &args, bool running) const
{
    if (function == QLatin1String("run_command"))
        return running ? tr("Running %1…").arg(args) : tr("Ran: %1").arg(args);
    if (function == QLatin1String("system_info"))
        return running ? tr("Reading system information…") : tr("Read system information");
    if (function == QLatin1String("read_document") || function == QLatin1String("read_web_page"))
        return running ? tr("Reading %1…").arg(args) : tr("Read %1").arg(args);
    if (function == QLatin1String("summarize_documents"))
        return running ? tr("Summarizing %1…").arg(args) : tr("Summarized %1").arg(args);
    if (function == QLatin1String("list_folder"))
        return running ? tr("Listing files…") : tr("Listed files");
    if (function == QLatin1String("search_files"))
        return running ? tr("Searching files for “%1”…").arg(args) : tr("Searched files for “%1”").arg(args);
    if (function == QLatin1String("list_notes") || function == QLatin1String("read_note"))
        return running ? tr("Reading your notes…") : tr("Read your notes");
    if (function == QLatin1String("calendar_agenda"))
        return running ? tr("Looking at the calendar…") : tr("Looked at the calendar");
    if (function == QLatin1String("web_search"))
        return running ? tr("Searching the web for “%1”…").arg(args) : tr("Searched the web for “%1”").arg(args);
    return running ? tr("Saving “%1”…").arg(args) : tr("Saved “%1”").arg(args);
}

void AiChat::onEngineEvent(const QVariantMap &event)
{
    const QString type = event.value(QStringLiteral("type")).toString();
    const QString conv = event.value(QStringLiteral("conversation_id")).toString();
    if (type == QLatin1String("ready") || type == QLatin1String("config") || type == QLatin1String("conversations")
        || type == QLatin1String("history"))
        return;
    if (!conv.isEmpty() && conv != m_conversationId) {
        if (type == QLatin1String("done") || type == QLatin1String("error"))
            setEngineBusy(false);   // a conversation that was cleared meanwhile
        return;
    }

    if (type == QLatin1String("token")) {
        const QString text = event.value(QStringLiteral("text")).toString();
        if (text.trimmed().isEmpty() && (m_messages.isEmpty() || m_messages.last().role != QLatin1String("assistant")))
            return;
        const int row = openAnswer();
        Message &m = m_messages[row];
        m.text += text;
        QList<int> roles{TextRole};
        if (m.thinking) {
            m.thinking = false;
            roles << ThinkingRole;
        }
        changed(row, roles);
    } else if (type == QLatin1String("thinking")) {
        const int row = openAnswer();
        if (m_messages.at(row).text.isEmpty() && !m_messages.at(row).thinking) {
            m_messages[row].thinking = true;
            changed(row, {ThinkingRole});
        }
    } else if (type == QLatin1String("tool_start") || type == QLatin1String("confirmation_required")) {
        Message m;
        if (type == QLatin1String("tool_start")) {
            m.role = QStringLiteral("tool");
            m.function = event.value(QStringLiteral("function")).toString();
            m.args = event.value(QStringLiteral("args_summary")).toString();
            m.running = true;
            m.text = toolLabel(m.function, m.args, true);
        } else {
            m.role = QStringLiteral("confirm");
            m.requestId = event.value(QStringLiteral("request_id")).toString();
            m.title = event.value(QStringLiteral("title")).toString();
            m.details = event.value(QStringLiteral("details")).toString();
            m.confirmState = QStringLiteral("pending");
        }
        // Keep the waiting answer bubble at the bottom
        int row = m_messages.size();
        if (!m_messages.isEmpty() && m_messages.last().role == QLatin1String("assistant")
            && m_messages.last().text.isEmpty())
            row = m_messages.size() - 1;
        beginInsertRows({}, row, row);
        m_messages.insert(row, m);
        endInsertRows();
        emit countChanged();
        changed(row, {});
    } else if (type == QLatin1String("tool_end")) {
        const QString function = event.value(QStringLiteral("function")).toString();
        for (int row = m_messages.size() - 1; row >= 0; --row) {
            Message &m = m_messages[row];
            if (m.role == QLatin1String("tool") && m.function == function && m.running) {
                m.running = false;
                m.ok = event.value(QStringLiteral("ok")).toBool();
                m.text = toolLabel(m.function, m.args, false);
                changed(row, {RunningRole, OkRole, TextRole});
                return;
            }
        }
    } else if (type == QLatin1String("confirmation_resolved")) {
        const QString id = event.value(QStringLiteral("request_id")).toString();
        for (int row = 0; row < m_messages.size(); ++row) {
            if (m_messages.at(row).role == QLatin1String("confirm") && m_messages.at(row).requestId == id) {
                m_messages[row].confirmState = event.value(QStringLiteral("allow")).toBool()
                                                   ? QStringLiteral("allowed") : QStringLiteral("denied");
                changed(row, {ConfirmStateRole});
            }
        }
    } else if (type == QLatin1String("notice")) {
        // One line, like an error but not red: the model can't use tools
        setError(event.value(QStringLiteral("message")).toString());
    } else if (type == QLatin1String("error")) {
        if (!m_engineBusy && conv.isEmpty())
            return;
        finishEngineAnswer({});
        setError(event.value(QStringLiteral("message")).toString());
        setEngineBusy(false);
    } else if (type == QLatin1String("done")) {
        const QVariantMap stats = event.value(QStringLiteral("stats")).toMap();
        // Same wording as the direct path: wall time + generation speed
        const QString seconds = QLocale().toString(stats.value(QStringLiteral("seconds")).toDouble(), 'f', 1);
        const double speed = stats.value(QStringLiteral("tokens_per_s")).toDouble();
        const QString text = speed > 0 ? tr("Answered in %1 s · %2 tokens/s").arg(seconds).arg(qRound(speed))
                                       : tr("Answered in %1 s").arg(seconds);
        finishEngineAnswer(event.value(QStringLiteral("stopped")).toBool() ? QString() : text);
        setEngineBusy(false);
    }
}

void AiChat::confirm(const QString &requestId, bool allow)
{
    for (int row = 0; row < m_messages.size(); ++row) {
        if (m_messages.at(row).role == QLatin1String("confirm") && m_messages.at(row).requestId == requestId
            && m_messages.at(row).confirmState == QLatin1String("pending")) {
            m_messages[row].confirmState = allow ? QStringLiteral("allowed") : QStringLiteral("denied");
            changed(row, {ConfirmStateRole});
            m_engine.send(QJsonObject{{QStringLiteral("type"), QStringLiteral("confirm")},
                                      {QStringLiteral("request_id"), requestId},
                                      {QStringLiteral("allow"), allow}});
            return;
        }
    }
}

void AiChat::openInAssistant()
{
    if (!m_usingEngine)
        return;
    // Stop here first: a waiting confirmation stays paused in the shared
    // database and the app's engine picks it up
    for (int row = 0; row < m_messages.size(); ++row) {
        if (m_messages.at(row).role == QLatin1String("confirm")
            && m_messages.at(row).confirmState == QLatin1String("pending")) {
            m_messages[row].confirmState = QStringLiteral("handed");
            changed(row, {ConfirmStateRole});
        }
    }
    if (m_engineBusy)
        m_engine.send(QJsonObject{{QStringLiteral("type"), QStringLiteral("stop")}});
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("com.lingmo.Assistant"),
                                                       QStringLiteral("/Assistant"),
                                                       QStringLiteral("com.lingmo.Assistant"),
                                                       QStringLiteral("open"));
    call << m_conversationId;
    // Started by D-Bus activation when the app isn't running
    QDBusConnection::sessionBus().asyncCall(call, 10000);
}
