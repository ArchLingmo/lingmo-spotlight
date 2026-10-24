#pragma once

#include <QObject>
#include <QVariantMap>

#include <LingmoIntents/intentmatcher.h>
#include <LingmoIntents/intentregistry.h>

#include "resultsmodel.h"

// App actions (Lingmo Intents) in Spotlight: typed text is matched against
// the example phrases of the installed manifests ("criar alarme 7h" ->
// "Relógio › Criar alarme às 07:00" with the time filled in). Picking a row
// shows what will run; actions that send or change something wait for an
// explicit "Run" (the app checks it again: context.confirmed).
class IntentProvider : public QObject
{
    Q_OBJECT
    // The action waiting in the popup: {title, app, icon, lines, confirm, missing, sends, running, error}
    Q_PROPERTY(QVariantMap pending READ pending NOTIFY pendingChanged)
    Q_PROPERTY(bool active READ active NOTIFY pendingChanged)

public:
    explicit IntentProvider(QObject *parent = nullptr);

    QList<SearchResult> search(const QString &text);
    // From SearchEngine::activate(): keep the picked match; true when it can run right away
    bool pick(const QString &payload);

    QVariantMap pending() const { return m_pending; }
    bool active() const { return !m_pending.isEmpty(); }

    Q_INVOKABLE void run();
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void pendingChanged();
    // ok: the action ran; message: its summary or why it failed
    void finished(bool ok, const QString &message);

private:
    LingmoIntents::IntentRegistry m_registry;
    QList<LingmoIntents::IntentMatch> m_matches;
    LingmoIntents::IntentMatch m_picked;
    QVariantMap m_pending;
};
