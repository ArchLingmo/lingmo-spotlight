#include "intentprovider.h"

#include <LingmoIntents/intentclient.h>
#include <LingmoIntents/intentgate.h>
#include <LingmoIntents/intentschema.h>

#include <QJsonObject>

using namespace LingmoIntents;

IntentProvider::IntentProvider(QObject *parent)
    : QObject(parent)
    , m_registry(this)
{
}

QList<SearchResult> IntentProvider::search(const QString &text)
{
    m_matches.clear();
    const IntentGate gate = IntentGate::load();
    QList<IntentInfo> allowed;
    for (const IntentInfo &i : m_registry.intents())
        if (gate.intentEnabled(i))      // Settings › IA › "Ações dos apps"
            allowed << i;
    QList<SearchResult> out;
    const QList<IntentMatch> matches = matchIntents(allowed, text, QString(), 3);
    for (const IntentMatch &m : matches) {
        // Partial phrases ("criar ala") only when nothing better shows up
        if (m.score < 60)
            continue;
        SearchResult r;
        r.kind = SearchResult::Intent;
        r.title = m.title;
        QStringList what;
        if (!m.missing.isEmpty()) {
            QStringList names;
            const QJsonObject props = m.intent.parameters.value(QStringLiteral("properties")).toObject();
            for (const QString &p : m.missing)
                names << localized(props.value(p).toObject().value(QStringLiteral("title")));
            what << tr("Missing: %1").arg(names.join(QStringLiteral(", ")));
        }
        what << (m.intent.needsConfirmation() ? tr("Asks before running") : m.intent.descriptionText());
        r.subtitle = what.join(QStringLiteral(" · "));
        r.icon = m.intent.icon;
        r.category = tr("Actions");
        r.payload = QStringLiteral("intent:%1").arg(m_matches.size());
        r.score = m.score;
        m_matches << m;
        out << r;
    }
    return out;
}

static QString valueText(const QVariant &v)
{
    if (v.typeId() == QMetaType::Bool)
        return v.toBool() ? QObject::tr("yes") : QObject::tr("no");
    if (v.typeId() == QMetaType::QVariantList || v.typeId() == QMetaType::QStringList)
        return v.toStringList().join(QStringLiteral(", "));
    return v.toString();
}

bool IntentProvider::pick(const QString &payload)
{
    const int index = payload.mid(int(qstrlen("intent:"))).toInt();
    if (index < 0 || index >= m_matches.size())
        return false;
    m_picked = m_matches.at(index);
    const IntentInfo &intent = m_picked.intent;
    QStringList errors;
    const QVariantMap params = IntentSchema::validate(intent.parameters, m_picked.params, &errors);
    const QJsonObject props = intent.parameters.value(QStringLiteral("properties")).toObject();
    QVariantList lines;
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (valueText(it.value()).isEmpty())
            continue;
        lines << QVariantMap{{QStringLiteral("label"), localized(props.value(it.key()).toObject().value(QStringLiteral("title")))},
                             {QStringLiteral("value"), valueText(it.value())}};
    }
    QStringList missing;
    for (const QString &p : m_picked.missing)
        missing << localized(props.value(p).toObject().value(QStringLiteral("title")));
    m_pending = {{QStringLiteral("title"), m_picked.title},
                 {QStringLiteral("app"), intent.appNameText()},
                 {QStringLiteral("icon"), intent.icon},
                 {QStringLiteral("description"), intent.descriptionText()},
                 {QStringLiteral("lines"), lines},
                 {QStringLiteral("missing"), missing.join(QStringLiteral(", "))},
                 {QStringLiteral("confirm"), intent.needsConfirmation()},
                 {QStringLiteral("sends"), intent.capabilities.contains(QStringLiteral("send"))},
                 {QStringLiteral("needsUI"), intent.needsUI},
                 {QStringLiteral("running"), false},
                 {QStringLiteral("error"), QString()}};
    Q_EMIT pendingChanged();
    return missing.isEmpty() && !intent.needsConfirmation();
}

void IntentProvider::run()
{
    if (m_pending.isEmpty() || m_pending.value(QStringLiteral("running")).toBool()
        || !m_pending.value(QStringLiteral("missing")).toString().isEmpty())
        return;
    m_pending.insert(QStringLiteral("running"), true);
    m_pending.insert(QStringLiteral("error"), QString());
    Q_EMIT pendingChanged();
    // The person picked the row and pressed Run: that is the confirmation
    const QVariantMap context{{QStringLiteral("caller"), QStringLiteral("spotlight")},
                              {QStringLiteral("confirmed"), true}};
    IntentResult check;
    const QVariantMap params = IntentClient::prepare(m_picked.intent, m_picked.params, context, &check);
    auto done = [this](const IntentResult &r) {
        if (m_pending.isEmpty())
            return;
        if (r.isOk()) {
            const QString title = m_pending.value(QStringLiteral("title")).toString();
            m_pending.clear();
            Q_EMIT pendingChanged();
            Q_EMIT finished(true, title);
            return;
        }
        m_pending.insert(QStringLiteral("running"), false);
        m_pending.insert(QStringLiteral("error"), r.message);
        Q_EMIT pendingChanged();
        Q_EMIT finished(false, r.message);
    };
    if (!check.isOk()) {
        done(check);
        return;
    }
    IntentClient::executeAsync(m_picked.intent, params, context, this, done);
}

void IntentProvider::cancel()
{
    if (m_pending.isEmpty())
        return;
    m_pending.clear();
    Q_EMIT pendingChanged();
}
